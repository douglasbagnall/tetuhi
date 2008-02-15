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

from vg import config
import yaml
from pprint import pformat

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
    _print_details()
