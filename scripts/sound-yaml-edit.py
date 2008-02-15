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
"""Attempt to encapsulate rulesets

team sets can have
 player + any of
 [enemy, food, enemy-bullet, player-bullet, moving-enviroment, static-environment, magic?, ally?, enemy2?]
 (~40k or 362k)

9!/(8!) + 9!/2(7!) + 9!/6(6!) + 9!/24(5!) + 9!/5!(4!) + + 9!/6!(3!)

but some combinations are perhaps impossible/implausible
order is fixed.

"""
import sys, os, random
sys.path.append('/home/douglas/tetuhi')


from cgi import parse_qs, parse_qsl
from urlparse import urlsplit
from pprint import pformat
from traceback import format_exc, print_exc
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer

import yaml
try:
    from yaml import CLoader as Loader
    from yaml import CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper



from vg import config

class Schema:
    def __init__(self, fn):
        f = open(fn)
        self.data = yaml.load(f, Loader=Loader)
        f.close()
        self.__dict__.update(self.data)



RELOAD = False

data, data_map, schema = None, None, None


def yaml_load(filename, reload=RELOAD):
    global data, data_map, schema
    if data is None and not reload:
        #only do this once
        try:
            f = open(filename)
            data = yaml.load(f, Loader=Loader)
            data_map = dict((x['name'], x) for x in data)
            f.close()
        except Exception,e:
            print e
            data = []
            data_map = {}

    if schema is None:
        schema = Schema(config.SCHEMA_FILE)

def yaml_save(filename):
    f = open(filename, 'w')
    yaml.dump(data, f, Dumper=Dumper)
    f.close()




class Handler(BaseHTTPRequestHandler):

    def _head(self):
        self.send_response(200)
        self.wfile.write("Content-type: text/html\n\n")

    def _list(self):
        self.wfile.write('<html>\n<ul>')
        for k in schema.actors:
            self.wfile.write('<li><a href="/%s">%s</a></li>' %(k, k))
        self.wfile.write('</ul></html>')


    def do_GET(self):
        self._head()
        bits = urlsplit(self.path)
        set = bits[2][1:]
        args = parse_qs(bits[3])
        try:
            if set not in data_map:
                self._list()
            else:
                if args:
                    self._save_args(actor, args)
                self._show(actor)
        except Exception, e:
            write = self.wfile.write
            write('<plaintext>')
            write(format_exc())


    def _save_args(self, actor, args):
        d = data_map[actor]
        print pformat(args)
        yaml_save()


    def _show(self, actor):
        write = self.wfile.write


def run(server_class=HTTPServer, handler_class=Handler):
    yaml_load(config.SOUND_YAML)
    server_address = ('', 8000)
    httpd = server_class(server_address, handler_class)
    httpd.serve_forever()



def randomise():
    import random
    yaml_load(config.SOUND_YAML)
    root = '/home/douglas/sounds/tetuhi/categorised/'
    for d in os.listdir(root):
        files = os.listdir(os.path.join(root, d))
        random.shuffle(files)
        free = len(files)
        used = 0
        i = 0
        while used < free and free > 32:
            name = '%s_%s' % (d, i)
            #print data_map, type(data_map), name
            s = {'name': name,
                 'sounds':{'hit':     files[used],
                           'turning': [None] + files[used + 1: used + 8],
                           'blocked': files[used + 8 : used + 16]
                           }
                 }
            data.append(s)

            used += 16
            i += 1
    yaml_save(config.SOUND_YAML)







randomise()
#run()
