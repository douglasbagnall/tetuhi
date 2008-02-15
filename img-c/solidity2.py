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

import imagescan
import Image
import time

#     . o .     . o .    . . .     . o .    . o .    . . .    . . .    . . .    . . .
#     o x .     . x .    o x o     o x o    o x o    . x .    . x .    . x .    . x .
#     . . .     . . .    . . .     . . .    . o .    . . .    . . .    . . .    . . .
#
#     erode      no      no        erode    no        no

def neighbours(pix, x, y, w, h):
    if x == 0:
        left = 0
        right = pix[1, y]
    elif x == w - 1:
        right = 0
        left = pix[x - 1, y]
    else:
        left = pix[x - 1, y]
        right = pix[x + 1, y]

    if y == 0:
        top = 0
        bottom = pix[x, 1]
    elif y == h - 1:
        bottom = 0
        top = pix[x, y - 1]
    else:
        top = pix[x, y - 1]
        bottom = pix[x, y + 1]
    return (left, top, right, bottom)

def erodability(pix, x, y, w, h):
    left, top, right, bottom = neighbours(pix, x, y, w, h)
    return (left + top + right + bottom) / 4
    
    if ((top != bottom and (left or right)) or
        (left != right and (top or bottom))):
        print left, top, right, bottom
        return 128
    
    return 255

def erodable(pix, x, y, w, h):
    left, top, right, bottom = neighbours(pix, x, y, w, h)
    this = pix[x,y]
    
    if ((top != bottom and (left or right)) or
        (left != right and (top or bottom))):
        if min((left, top, right, bottom)) > 50:
            return False
        return True
    
    return False
        

def go():
    im = Image.open('/home/douglas/images/tetuhi-examples/cowboy-tanks.jpg-sprites/alpha-2.png')
    r,g,b, a1 = im.split()
    im.save('/tmp/_orig.png')
    a1.save('/tmp/_alpha.png')
    a2 = Image.new('L', a1.size, 0)
    pix1 = a1.load()
    pix2 = a2.load()
    w, h = a1.size
    changing = True
    i = 0
    while changing:
        changing = False
        for y in range(h):
            for x in range(w):
                if pix1[x,y]:
                    #print pix1[x,y]
                    #erode if it is surrounded on a few sides only
                    pix2[x,y] = erodability(pix1, x, y, w, h)
                else:
                    pix2[x,y] = 0
                    
        for y in range(h):
            for x in range(w):
                if pix1[x,y]:
                    #print pix1[x,y]
                    #erode if it is surrounded on a few sides only
                    if erodable(pix2, x, y, w, h):
                        pix1[x,y] = 0
                        changing = True
                else:
                    pix2[x,y] = 0                    
        i += 1
        a1.save('/tmp/_erode-%04d.png' % i)
        a2.save('/tmp/_erode2-%04d.png' % i)
        #a2, a1 = a1, a2
        #pix2, pix1 = pix1, pix2



go()
