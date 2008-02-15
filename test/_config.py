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
"""look out for unused config variables"""

import _test_common
import os

from vg import config

DIRS = '/home/douglas/tetuhi', '/home/douglas/tetuhi/vg'

results = set()
files = {}
for d in DIRS:
    for x in os.listdir(d):
        if x.endswith('.py') and x != 'config.py':
            #mashing directories together -- wouldn't work in general
            f = open(os.path.join(d, x))
            files[x] = f.read()
            f.close()


names = [x for x in config.__dict__ if x.isupper()]

for n in names:
    for x in files.values():
        if n in x:
            results.add(n)
            break

for n in names:
    if n not in results:
        print n

#print results
