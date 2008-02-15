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
import sys
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
    def __init__(self):
        f = open(config.SCHEMA_FILE)
        self.data = yaml.load(f, Loader=Loader)
        print pformat(self.data)

        f.close()
        self.actors = self.data['actors']
        self.attributes = self.data['attributes']
        self.mind_strings = self.data['mind_string']
        self.suitability_map = self.data['suitability_map']
        self.character_list = self.data['character_list']

        self.rules_map =  self.data['rules_map']
        self.aims_map =  self.data['aims_map']
        self.attention_map = self.data['attention_map']


schema = Schema()

RELOAD = False
DATA, DATA_MAP = None, None

class Handler(BaseHTTPRequestHandler):

    def _load(self):
        global DATA, DATA_MAP
        if DATA is None and not RELOAD:
            #only do this once
            f = open(config.RULES_FILE)
            self.data = yaml.load(f, Loader=Loader)
            self.data_map = dict((x['name'], x) for x in self.data)
            f.close()
            DATA = self.data
            DATA_MAP = self.data_map
        else:
            self.data = DATA
            self.data_map = DATA_MAP

    def _save(self):
        f = open(config.RULES_FILE, 'w')
        yaml.dump(self.data, f, Dumper=Dumper)
        f.close()

    def _head(self):
        self.send_response(200)
        self.wfile.write("Content-type: text/html\n\n")

    def _list(self):
        self.wfile.write('<html>\n<ul>')
        for k in schema.actors:
            self.wfile.write('<li><a href="/%s">%s</a></li>' %(k, k))
        self.wfile.write('</ul></html>')


    def do_GET(self):
        self._load()
        self._head()
        bits = urlsplit(self.path)
        actor = bits[2][1:]
        args = parse_qs(bits[3])
        try:
            if actor not in self.data_map:
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
        d = self.data_map[actor]
        print pformat(args)
        for k, v in schema.attributes.items():
            print k, v
            #some have composite names, some are direct
            if v == 'suitability_map':
                for s in schema.suitability_map:
                    key = '%s++%s' % (k,s)
                    if key in args:
                        d[k][s] = float(args[key][0])

            elif v == 'rules_map':
                if not k in d:
                    d[k] = {}
                for s in getattr(schema, v):
                    d[k][s] = []
                for a in schema.actors:
                    key = '%s++%s' % (k, a)
                    if key in args:
                        for s in args[key]:
                            if s in getattr(schema, v):
                                d[k][s].append(a)

            elif v == 'aims_map':
                if not k in d:
                    d[k] = {}
                for a in schema.actors:
                    key = '%s++%s' % (k, a)
                    if key in args:
                        print args[key]
                        s = args[key][0]
                        if s in getattr(schema, v):
                            d[k][a] = s

            elif v == 'attention_map':
                print 'doing attention map'
                if not k in d:
                    d[k] = {}
                for a in schema.actors:
                    key = '%s++%s' % (k, a)
                    if key in args:
                        print args[key]
                        s = args[key][0]
                        d[k][a] = eval(s)
            else:
                arglist = args.get(k, ['None'])
                arg1 = arglist[0]
                if v in ('number', 'list'):
                    d[k] = eval(arg1)
                elif v == 'string':
                    d[k] = arg1
                elif v == 'mind_string' and arg1 in schema.mind_strings:
                    d[k] = arg1
                elif v == 'character_list':
                    d[k] = [s for s in schema.character_list if s in arglist]




        print pformat(d)
        self._save()


    def _show(self, actor):
        write = self.wfile.write
        d = self.data_map[actor]
        write('<html><h1>%s</h1>\n<form><a href="/">home</a>' % actor)
        for a in schema.actors:
            write(' | <a href="/%s">%s</a> ' % (a,a))
        write('<br/><input type="submit" />')
        for k, v in schema.attributes.items():
            print "doing %s" %k
            if v != 'hidden_string':
                write('<h2>%s</h2>' % k)
            dk = d.get(k, '')
            if d['mind'] == 'wall' and dk == '':
                continue
            #simple ones
            if v in ('string', 'number', 'list'):
                write('<input name="%s" value="%s" size="%s" />' % (k, dk, 8 + 50 * (v != 'number')))
            #mind
            elif v == 'mind_string':
                for s in schema.mind_strings:
                    write('<input type="radio" name="%s" value="%s" %s/> %s<br/>' %
                          (k, s, ['', 'checked '][s == dk], s))
            #suitability
            elif v == 'suitability_map':
                for s in schema.suitability_map:
                    write('%s <input name="%s++%s" value="%s" size="5" /><br/>' % (s, k, s, dk[s] or 0))
            #character list
            elif v == 'character_list':
                for s in schema.character_list:
                    write('<input type="checkbox" name="%s" value="%s" %s/> %s<br/>' %
                          (k, s, ['', 'checked '][s in dk], s))
            #rules
            elif v == 'rules_map':
                write('<table><td></td><td><i>none</i></td>')
                for s in getattr(schema, v):
                    write('<th style="padding:0px 9px">%s</th>' % s)
                for a in schema.actors:
                    write('<tr><td><b>%s</b></td><td> </td>\n' %
                          (a,))
                    for s in getattr(schema, v):
                        write('<td><input type="checkbox" name="%s++%s" value="%s" %s/>%s</td>\n' %
                              (k, a, s, ['', 'checked '][s in dk and a in dk[s]], ''))
                    write('</tr>\n')
                write('</table>\n')
            #aims
            elif v == 'aims_map':
                print pformat((d, k, v, dk))
                write('<table><td></td><td>weight</td>')
                for s in getattr(schema, v):
                    write('<th style="padding:0px 9px">%s</th>' % s)
                for a in schema.actors:
                    colour = '#'
                    d2 = self.data_map[a]
                    colour += '08'[a in d['rules']['die']]
                    colour += '08'[actor in d2['rules']['die']]
                    colour += '0'

                    write('<tr><td><b style="color:%s">%s</b><small>(%s)</small></td><td>'
                          '<input name="attention++%s" value="%s" size="3" /></td>\n'
                          % (colour, a, d2[k].get(actor, '-'), a, d['attention'].get(a, 1)))
                    for s in getattr(schema, v):
                        write('<td>')
                        write('<input type="radio" name="%s++%s" value="%s" %s/>\n' %
                              (k, a, s, ['', 'checked '][s == dk.get(a)]))
                        #print (k, a, s, dk.get(a), s == dk.get(a))
                        write('</td>\n')
                    write('</tr>\n')
                write('</table>\n')



        write('<br/><input type="submit" /></form></html>')


def run(server_class=HTTPServer,
        handler_class=Handler):
    server_address = ('', 8000)
    httpd = server_class(server_address, handler_class)
    httpd.serve_forever()



def rearrange():
    f = open(config.RULES_FILE)
    data = yaml.load(f)
    data_map = dict((x['name'], x) for x in data)
    for d in data:
        aims_map = d.pop('aims', {})
        actormap = d['aims_actormap'] = {}
        attnmap = d['attentionmap'] = dict((x, 1) for x in schema.actors)

        for a in schema.actors:
            actormap[a] = 'ignore'
            for k,v in aims_map.iteritems():
                if a in v:
                    actormap[a] = k
                    break


    f = open('/tmp/tmp.yaml', 'w')
    yaml.dump(data, f, Dumper=Dumper)
    f.close()



def _print_details():
    f = open(config.RULES_FILE)
    aims = set()
    rules = set()
    for x in yaml.load(f):
        print x['name']
        continue
        print pformat(x)
        for y in x.get('aims', []):
            aims.add(y)
        for y in x.get('rules', []):
            rules.add(y)
    print 'aims', aims
    print 'rules', rules

if __name__ == '__main__':
    run()
    #rearrange()
