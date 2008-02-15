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
"""
bits to generate tables and suchlike.

"""

MUTATE_LUT_SIZE = 256



import random
from math import cos, pi, sqrt, sin, tan

import re


def L(n):
    def _fn(seq):
        return sum([ x ** n for x in seq]) ** (1.0/n)
    return _fn


def write_define(seq, name):
    define = []
    line = "#define %s  " % (name)
    blank = " " * len(line)
    while seq:
        while seq and len(line) < 80:
            line += "%.8f, " % seq.pop()
        if seq:
            define.append(line)
            line = blank
    return '\\\n'.join(define) + '\n'

#       *w += 0.5 * tan(3 * random.random()) - 1.5) * fabs(*w + 0.1);

def show_lut(diffs):
    a = 1.0
    s = 0
    print diffs
    for b in diffs:
        a *= b
        s += b
    print len(diffs)
    print diffs[65],diffs[192]
    print a
    print s
    return
    
def generate_lut():
    a = [ tan(tan(tan((x) * 0.006))) for x in range(1, 129) ]
    random.shuffle(a)
    add = []
    for x in a:
        add.append(x)
        add.append(-x)
    d1 = [ 1.0 + tan(tan((x+0.5) * 0.0054)) for x in range(1, 129) ]
    d2 = [ 1.0/x for x in d1]
    d2.reverse()
    mul = d2 + d1
    show_lut(add)
    
    #print write_define(diffs, 'MUTATE_LUT_MUL')
    print write_define(add, 'MUTATE_LUT_ADD')


if __name__ == '__main__':
    generate_lut()
