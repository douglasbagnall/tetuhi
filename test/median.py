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

import Image
import imagescan
import ImageFilter, ImageChops
import random

im = Image.new('RGBA', (50,50), (255,)*4)
im2 = Image.new('RGBA', (50,50), (255,0,0,255))
pix = im.load()
for y in range(50):
    for x in range(50):
        n = x + y
        #pix[x,y] = (255 * (n % 2), 127 * (n % 3), 63 * (n % 5), 255)
        #pix[x,y] = (255 * (x % 4 > 1), 255 * (x % 4 > 1), 255, 255) 
        pix[x,y] = tuple(random.randrange(256) for x in range(3)) + (255,)


imagescan.fast_median(im.load(), im2.load())

im3 = im.filter(ImageFilter.MedianFilter(size=3))

im.save('/tmp/median-src.png')
im2.save('/tmp/median-dest.png')
im3.save('/tmp/median-pil.png')

im4 = ImageChops.difference(im2, im3)
im4.save('/tmp/median-diff.png')
