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

import sys, os, random

sys.path.append('/home/douglas/tetuhi')

from vg import config


import yaml
try:
    from yaml import CLoader as Loader
    from yaml import CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper



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
    return data

def yaml_clear():
    global data, data_map, schema
    data = []
    data_map = {}            
    if schema is None:
        schema = Schema(config.SCHEMA_FILE)
    return data


def yaml_save(filename):
    f = open(filename, 'w')
    yaml.dump(data, f, Dumper=Dumper)
    f.close()



