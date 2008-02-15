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
"""Pretend to have parsed a picture, making blobs out of
nothing. Useful for febuggin the game engine because it is faster than
really parsing"""
#XXX but it is broken!

import sys, os, random
import Image, ImageFilter, ImageOps

from vg import config

from vg.cblobdetect import Blob


class FakeBlob(Blob):
    silhouette = None
    def __init__(self, size, colour, position):
        w, h = size
        x, y = position
        im = Image.new('RGBA', size, colour)
        pix = im.load()
        pix[1, 1] = (0,0,0)
        pix[1, 2] = (0,0,0)
        pix[2, 1] = (0,0,0)
        pix[2, 2] = (0,0,0)
        pix[w - 2, 1] = (0,0,0)
        pix[w - 2, 2] = (0,0,0)
        pix[w - 3, 1] = (0,0,0)
        pix[w - 3, 2] = (0,0,0)
        self.im = im
        self.width = w
        self.height = h
        self.pos = (x, y, x+w, y+h)
        self.colour = colour
        self.volume = w * h
        self.convex_area = w * h
        self.area = w * h
        self.perimeter = w + w + h + h
        self.convex_perimeter = w + w + h + h
        self.tiny = self.convex_perimeter <= config.TINY_SPRITE_SIZE
        self.huge = self.convex_perimeter > config.HUGE_SPRITE_SIZE
        self.solidity = 1

    def finalise(self):
        """do all the complex work, after the selection has been
        finalised (this could happen in __init__, but it might
        needlessly repeat)"""
        r,g,b,a = self.im.split()
        self.silhouette = a

        if config.USE_CHROMA_KEY:
            self.picture = self.im.convert('RGB')
        else:
            self.picture = self.im


class BlobFinder:
    """pretends to be a blobfinder, but just makes up images and
    places them arbitrarily (fast and clear, for testing)"""

    blobs = [((10, 15), (255,0,0)),
             ((10, 20), (255,128,0)),
             ((15, 20), (180,180,0)),
             ((15, 20), (100,150,0)),
             ((25, 30), (0,180,120)),
             ((35, 25), (30,100,200)),
             ((25, 35), (50,0,255)),
             ((25, 45), (200,0,200)),
             ]
    entropy = None

    def parse(self, *args):
        """find the blobs and background"""
        bg = Image.new('RGB', config.WINDOW_SIZE, (255, 255, 255))
        blobs = []
        sx = 20
        sy = 20

        for size, colour in self.blobs:
            s = FakeBlob(size, colour, (sx, sy))
            blobs.append(s)
            sx = (sx + 241) % config.WORKING_SIZE[0]
            sy = (sy + 347) % config.WORKING_SIZE[1]

        #self.background = bg
        #self.blobs = blobs
        return bg, blobs

    def __init__(self, *args, **kwargs):
        
        self.entropy = [ random.randrange(2**30) for x in range(64) ]


    def time_summary(self, *args):
        pass
