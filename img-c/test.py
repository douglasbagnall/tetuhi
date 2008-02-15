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

def bw_img():
    """intricate, sparse 128x128 image"""
    im = Image.new('L', (128,128), 0)
    pix = im.load()
    for x in range(im.size[0]):
        for y in range(im.size[1]):
            if ((x * y) % 257) < ((x * y) % 84):
                pix[x,y] = 255

    im.save('/tmp/test.png')
    return im


def test():
    im = bw_img()
    im2 = Image.new('L', im.size, 0)
    pix = im.load()
    pix2 = im2.load()
    print "about to do map", imagescan.influence_map
    t = time.time()
    imagescan.influence_map(pix, pix2, 0, 0, 128, 128)
    print "done that, took %s secs" % (time.time() -t)
    im2.save('/tmp/influence.png')
    im.save('/tmp/mask.png')
    print "returning"


def test_surface_area():
    im = bw_img()
    print "black and white image has surface area %s" % imagescan.surface_area(im.load())
    im2 = Image.new('L', (20, 20), 0)
    pix = im2.load()
    pix[5,5] = 255
    print "with 5,5 surface area  %s" % imagescan.surface_area(pix)
    pix[0,5] = 255
    print "adding 0,5 surface area %s" % imagescan.surface_area(pix)
    for y in range(5,7):
        for x in range(1,5):
            pix[x,y] = 255
            print "after (%s,%s) surface area %s" % (x, y, imagescan.surface_area(pix))

    im3 = Image.new('L', (50, 50), 0)
    pix = im3.load()
    for y in range(50):
        for x in range(50):
            if (x + y) % 2:
                pix[x,y] = 255
    im3.save('/tmp/grid.png')
    for i in range(1,21):
        im4 = im3.crop((0,0,i,i))
        im4.load()
        #print im4, im4.load(), im4.load()
        #print dir(im4.im)
        a = imagescan.surface_area(im4.im.pixel_access())                
        print "%s square surface %s, area %s, linear ratio %s square ration %s" % (i, a, i*i, float(a)/i, float(a)/(i*i) )
            
    


if __name__ == '__main__':
    #test()
    #test_surface_area()
    pass
