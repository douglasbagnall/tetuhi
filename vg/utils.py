# This file is part of Te Tuhi Video Game System.
#
# Copyright (C) 2008 Douglas Bagnall
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
"""Common routines"""

import cPickle, tempfile, os, fcntl, sys, random, traceback
import time

from vg import config

def log(*messages):
    for m in messages:
        print >>sys.stdout, m
    sys.stdout.flush()


def id_generator(start=1, wrap=2000000000, prefix='', suffix='', pattern ='%s%s%s'):
    """Generator that can count. By default returns stringified
    Numbers from '1' to '2000000000' before wrapping back to '1'."""
    while 1:
        next_id = start
        while next_id < wrap:
            yield pattern % (prefix, next_id, suffix)
            next_id += 1

def make_dir(*args):
    d = os.path.join(*args)
    if not os.path.exists(d):
        os.makedirs(d)
    return d


def pickle(obj, fn=None):
    #for x in dir(obj):
    #    print "%20s:  %50s" % (x, getattr(obj, x))
    print "pickling %s" % (obj,)
    if fn is None:
        fn = tempfile.mkstemp('.pickle')[1]
    f = open(fn, 'w')
    cPickle.dump(obj, f, cPickle.HIGHEST_PROTOCOL)
    f.close()
    return fn


def unpickle(fn):
    f = open(fn)
    o = cPickle.load(f)
    f.close()
    return o


def fork_safely(logfile=config.CHILD_LOG, mode='a',logmode='PID'):
    """Wrap a fork in an attempt not to upset X.  From the parents
    point of view, it behaves just like os.fork().  For the child, it
    closes or redirects various file handles, before returning 0 as
    expected."""
    pid = os.fork()
    if pid:
        return pid
    # in child
    if logmode == 'PID':
        logfile = config.LOG_FORMAT % os.getpid()

    if not config.ALLOW_ERRORS_TO_KILL_X:
        fextra = open(logfile + '.x_rubbish', 'a+')
        fdextra = fextra.fileno()
        #shut out a whole lot of files in the (reasonable) hope that
        #one of them is X
        #import fcntl

        #XXX try shutting down input? sound?
        for x in range(3, 19):
            #XXX use os.fpathconf or statvfs to find out about the thing and replicate it better.
            #v = fcntl.fcntl(x, fcntl.F_GETFL)
            os.dup2(fdextra, x)
            pass
        log("dup2'd fds")
        #os.system('lsof > /tmp/files.txt')
        #import pygame
        #pygame.quit()
        #log("quit!")
    #os._exit(0)


    fnull = open(os.devnull, 'r')
    flog = open(logfile, mode)
    fdno = flog.fileno()
    #print fdno, sys.stdin.fileno(), sys.stdout.fileno(), sys.stderr.fileno()
    os.dup2(fnull.fileno(), sys.stdin.fileno())
    os.dup2(fdno, sys.stdout.fileno())
    os.dup2(fdno, sys.stderr.fileno())
    # redirect at python level (also keeps file objects from garbage collection)
    #sys.stdout = flog
    #sys.stderr = flog
    #sys.stdin = fnull


    return 0


def process_in_fork(function, display, processes, timeout):
    """make rules, in background processes"""
    children = []
    for i in range(processes):
        print "handling child %s" %i
        child = fork_safely()
        #without this jump all the processes will give the same answer
        random.jumpahead(11)
        if not child:
            #in child
            try:
                print "in child %s, to run %s" % (i, function)
                #os._exit(0)
                function()
                print "in child %s, after %s" % (i, function)
            except:
                print "in child %s, with exception" % (i)
                #exception will kill X window (and thus main process),
                #so catch everything
                traceback.print_exc()
                sys.stderr.flush()
            print "in child %s, wleaving" % (i,)
            os._exit(0)
        children.append(child)

    # now, twiddle thumbs
    timeout += time.time()
    results = []
    while children and time.time() < timeout:
        if display is not None:
            display()
        pid, status = os.waitpid(-1, os.WNOHANG)
        if pid in children:
            children.remove(pid)
            print "got pid %s, status %s" % (pid, status)
            if not status:
                results.append(pid)
    #final chance -- no display/delay
    for i in range(len(children)):
        pid, status = os.waitpid(-1, os.WNOHANG)
        children.remove(pid)
        print "got (late) pid %s, status %s" % (pid, status)
        if not status:
            results.append(pid)


    if children:
        print "getting violent with children %s" % children
        for pid in children: #kill slowcoaches, if any
            os.kill(pid, 9)
    return results





def photograph_in_fork(display=None):
    """Take a photograph, making up returning its filename"""
    format = "%Y-%m-%d_%H-%M-%S/original." + config.CAPTURE_IMAGE_TYPE
    filename = os.path.join(config.PHOTO_DIRECTORY, time.strftime(format))

    def photograph():
        os.system(config.CAMERA_SCRIPT + ' ' + filename)

    process_in_fork(photograph, display, 1, config.CAMERA_TIMEOUT)
    return filename


def yaml_load(filename):
    """Load and interpret a YAML file, using whichever library is
    found. The following packages are tried:

     pysyck
     pyyaml (with C extensions if available)

    """
    f = open(filename)
    try:
        import syck
        s = f.read()
        y = syck.load(s)

    except:
        import yaml
        try:
            from yaml import CLoader as Loader
        except ImportError:
            from yaml import Loader

        y = yaml.load(f, Loader=Loader)

    f.close()
    return y
