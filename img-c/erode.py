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

def go():
    #im = Image.open('/home/douglas/images/tetuhi-examples/hills-cars.jpg-sprites/alpha-2.png')
    im = Image.open('/home/douglas/images/tetuhi-examples/cowboy-tanks.jpg-sprites/alpha-2.png')
    r,g,b, a1 = im.split()
    im.save('/tmp/_orig.png')
    a1.save('/tmp/_alpha.png')
    a2 = Image.new('L', a1.size, 0)
    a2.paste(a1, (0,0))
    pix1 = a1.load()
    pix2 = a2.load()
    imagescan.find_mountains(pix2)
    a2.save('/tmp/_erode.png')
    a3 = Image.new('L', a1.size, 0)
    pix3 = a3.load()
    imagescan.find_ridges(pix2, pix3)
    a3.save('/tmp/_erode2.png')




go()
