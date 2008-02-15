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

im = Image.open("/home/douglas/images/tetuhi-examples/cowboy-tanks.jpg-sprites/alpha-1.png")

for i in range(7):
    im.save('/tmp/tank-%s.png' % i)
    im2 = Image.new('RGBA', im.size)
    imagescan.scramble(im2.load(), im.load())
    im = im2

