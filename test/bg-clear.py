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
import _test_common

import time
import imagescan

import Image

IMAGES = {
    'macraes': '/home/douglas/images/tetuhi-examples/macraes-mine.jpg-blobs/clean_3.png', 
    'unicorn': '/home/douglas/images/tetuhi-examples/Unicorns and Rainbows.jpg-blobs/clean_1.png',
    'hills'  : '/home/douglas/images/tetuhi-examples/hills-cars.jpg-blobs/clean_1.png',
    'horse'  : '/home/douglas/images/tetuhi-examples/cowboy-tanks.jpg-blobs/clean_1.png',
    'maze'   : '/home/douglas/images/tetuhi-examples/maze.jpg-blobs/clean_1.png',
    'notes'  : '/home/douglas/images/tetuhi-examples/notes.jpg-sprites/clean_1.png',
}

IMAGE = IMAGES['macraes']

im = Image.open(IMAGE)
im.save('/tmp/orig.png')

t = time.time()
imagescan.grow_background(im.load())
print "took %s secs" % (time.time() - t)

im2 = im.convert('RGB')

r,g,b,a =im.split()

im2.save('/tmp/rgb.png')
im.save('/tmp/rgba.png')
a.save('/tmp/alpha.png')
