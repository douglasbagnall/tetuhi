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


def softclip(x):
    x /= 256.0
    return 400 *( x - 0.6 * x**3 + 0.5 * x**5 - 0.4 * x ** 7 + 0.25 * x ** 9
                  )

W = 512
H = 512

im = Image.new('L', (W, H))
pix = im.load()

_h = H - 1
for x in range(W):
    pix[x, _h - x] = 128
    y = int(softclip(x - 256)) + 256
    #print y
    if y < 512 and y >= 0:
        pix[x, _h - y] = 255

im.save('/tmp/softclip.png')
