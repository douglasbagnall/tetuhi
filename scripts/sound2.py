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


import ossaudiodev, wave, random
import os, pickle, shutil

SRC_DIR = '/home/douglas/sounds/tetuhi/'
DEST_DIR = '/home/douglas/sounds/tetuhi/categorised'

SUB_DIRS = ('faster', 'lower', 'mono', 'reversed', 'slower')

f = open('/tmp/sound_categories')
categories, waves = pickle.load(f)

#reverse the categories
categories2 = dict((v, k) for k, v in categories.items())



sizes = ((10000, 'short'), (25000, 'medium'), (50000, 'longish'), (100000, 'long'), (99999999,'huge'))


i = 0
for fn, c in waves.items():
    c2 = categories2[c]
    for d in SUB_DIRS:
        p = os.path.join(SRC_DIR, d, fn)
        size = os.path.getsize(p)
        for s, size_name in sizes:
            if size < s:
                break

        p2 = os.path.join(DEST_DIR, c2, '%s-%s-%s.wav' %(i, d, size_name))
        shutil.copy(p, p2)

    i += 1
