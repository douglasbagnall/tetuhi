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

# 0 1 2
# 7   3
# 6 5 4
#
# what is a theory of connectivity?
#
# o o o   o . o   . . .   . . .   . . .   . . .   . . .   o . .
# . x .   . x .   . x .   . x .   . x .   . x .   . x .   . x .
# o o o   o o .   . . .   . . .   . . .   . . .   . . .   o . o
#
#


links = []
for x in range(8):
    if x & 1:
        links.append(set((z + x) % 8 for z in (-2, -1, 1, 2))) 
    else:
        links.append(set((z + x) % 8 for z in (-1, 1))) 

print links

def link(points):
    if len(points) < 2 or len(points) > 6: 
        return 1
    p = points.pop()
    connected = links[p].copy()
    while True:
        points2 = tuple(points)
        for q in points2:
            if q in connected:            
                points.remove(q)
                connected |= links[q]
        if len(points) == len(points2):
            #no further progress
            if points:
                print points
                return 1
            return 0


def go():
    answer = []
    for i in range(256):
        bits = [int(i & (1 << x) != 0) for x in range(8) ]
        bits_on = set(z for z, x in enumerate(bits) if x)
        if i == 255:
            a = 1
        elif bits_on:
            a = link(bits_on)
        else:
            a = 1
            
        print i, bits, bits_on, a
        answer.append(a)
    
    return answer

answer = go()
for i,x in enumerate(answer):
    if i % 16 == 0:
        print
    print "%s," % x,
    
