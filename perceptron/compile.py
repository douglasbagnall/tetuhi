#!/usr/bin/python
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
#
# see the README file in the perceptron directory for information
# about possible future licensing.

import os, sys, time


def compile_mod(name):
    if not nocompile:
        print "compiling..."
        os.system("gcc -c -fPIC -O3  -I/usr/include/python2.3 "
                  "-Wall -ffast-math -I/home/douglas/texts/lib "
                  "-march=i686 "
                  #"-Wno-unused -Wno-uninitialized "
                  "-fno-strict-aliasing "
                  "%s.c -o /tmp/%s.o" % (name, name))
        

        os.system("gcc -shared /tmp/%s.o -o %s.so" % (name, name))



def print_results(set, net):
    for a,b in set:
        print "%s %s   %s" %(a, b, ', '.join(['%2.2f' % x for x in net.opinion(a)]))


def test_net(dimensions, set, momentum=0.3, learn_rate=0.2, scale=1, r=(-0.5,0.5), count=10000,threshold=0.00001):
    import nnpy
    net = nnpy.Network(dimensions, momentum=momentum,
                       learn_rate=learn_rate, scale=scale, weight_range=r)
    print "in test_net, refcount is %s " % sys.getrefcount(net)
    #net.randomise(r)
    net.learn_rate = 0.23
    net.momentum = 0.321
    print "set learn_rate and momentum " 
    t = time.time()
    print "got time" 
    net.train_set(set, count, threshold, 0)
    print "took %s seconds" % (time.time() - t)
    #print_results(set, net)

    w = net.get_weights()
    print "copied net via python:"
    net2 = nnpy.Network(dimensions)
    net2.set_weights(w)
    #print_results(set, net2)
    return net

    

def import_and_test():
    import nnpy
    print dir(nnpy)

    for a in dir(nnpy):
        print getattr(nnpy, a)
        try:
            print getattr(nnpy, a)()
        except TypeError:
            pass


    set1 = [([1, 1], [1]),
            ([1, 0], [0]),
            ([0, 1], [0]),
            ([0, 0], [1])]


    n = test_net([2, 10, 1], set1)
    for a in dir(n):
        print "n.%s: " % a
        print getattr(n, a)
        try:
            print getattr(n, a).__doc__
        except AttributeError:
            pass


    print "trying 4 layer net"
    test_net([2,4,4,1], set1)

    set2 = []
    for x in range(16):
        ins = (x >> 3, x >> 2 & 1, x >> 1 & 1, x & 1)
        outs = (sum(ins)>>1 &1, sum(ins)&1)
        set2.append((ins, outs))


    #test_net([4,9,9,2], set2)

    net = test_net([4, 11, 2], set2)
    print "refcount is %s " % sys.getrefcount(net)
    
    net.save_weights('/tmp/test.net')
    net2 = nnpy.Network([4, 11, 2])#, 0.3, 0.3, 1.0)
    print "loading weights"
    net2.load_weights('/tmp/test.net')    
    #print_results(set2, net2)

    print "trying 2 layer net"
    net = nnpy.Network((3, 2))
    
    print "trying 1 layer net -- should fail"
    try:
        net = nnpy.Network((2,))
    except ValueError, e:
        print e


def memtest():
    from resource import getrusage, RUSAGE_SELF
    #x =[]
    for z in range(50):
        t = time.time()
        for y in range(100000):
            net = nnpy.Network([5, 13, 3], weight_range=(-1,1))
            net.opinion((0,0,0,0,0))
            #x.append(net)
        print getrusage(RUSAGE_SELF)[2:8], time.time() - t
            
if __name__ =='__main__':
    name = "nnpy"
    nocompile = "test" in sys.argv[1:]
    noimport = "compile" in sys.argv[1:]
    if not nocompile:
        compile_mod(name)
    if not noimport:
        import_and_test()
    if 'memtest' in sys.argv[1:]:
        import nnpy
        memtest()

    print "ends"
