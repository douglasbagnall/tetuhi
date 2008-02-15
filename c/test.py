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

import semanticCore
import Image, ImageDraw
import random, os, sys
from optparse import OptionParser

DEFAULT_SIZE = (128, 96)
PROFILE_FILE = '/tmp/test.prof'

def red_background(fn):
    """put a red background on a greyscale image"""
    im = Image.open(fn).convert('RGB')
    gb = range(256)
    r = [255] + gb[1:]
    im2 = im.point(r + gb + gb)
    im2.save(fn + '.png')


def bw_img():
    """intricate 32x32 image"""
    im = Image.new('L', (32,32))
    pix = im.load()
    for x in range(im.size[0]):
        for y in range(im.size[1]):
            if ((x * y) % 31) > 23 :
                pix[x,y] = 255

    im.save('/tmp/test.png')
    return im


def scattered_images(gm=None, placements=100):
    """Put pictures randomly over a gamemap"""
    if gm is None:
        gm = semanticCore.Gamemap(*DEFAULT_SIZE)
    gm.prescan_clear()
    #add some wee pictures
    for x in gm.bits:
        gm.add_element(3, 3, x, 'x\00x' '\00x\00' 'x\00x')
    w, h = gm.size
    for x in range(placements):
        n = random.randrange(7)
        gm.set_figure(n, random.randrange(w), random.randrange(h))
    return gm


def test_elements():
    gm = semanticCore.Gamemap(*DEFAULT_SIZE)
    im = bw_img()
    imstring = im.tostring()
    for x in gm.bits:
        gm.add_element(im.size[0], im.size[1], x, imstring)
    w,h = gm.size
    points = []
    for a in range(7):
        points.append((a, [((((5 + a) * b) ** 3) % w, (((7 + a) * b) ** 3) % h) for b in range(3)]))
        for x, y in points[-1][1]:
            gm.set_figure(a, x, y)
            print "figure %s set at %s,%s" %(a,x,y)

    print gm.region_contents([1,1, 100,100, 1,100, 100,1]);
    print gm.region_contents([20,21 ,20,40, 40,21, 39,39]);
    print gm.region_contents([1,2,3,4,5,6,7,8]);
    gm.save_map('/tmp/map.pgm', 0)
    im2 = Image.open('/tmp/map.pgm')

    for bit in range(8):
        def _p(v):
            return (v & (1 << bit))
        im3 = im2.point(_p, '1')
        im3.save('/tmp/bit-%s.png' % bit)

    for a, p in points:
        for x, y in p:
            gm.clear_figure(a, x, y)

    gm.save_map('/tmp/map3.pgm')



def test_img():
    import sys
    sys.path.append('../test')
    from scan_results import EXPECTED
    xpoints = (55,)
    ypoints = (0,1,14,15, 45, 55, 65, 80, 90, 110)
    xpoints = (0,1,14,15,16,28,45,55,65)
    ypoints = xpoints
    for x in xpoints:
        for y in ypoints:
            gm = semanticCore.Gamemap(*DEFAULT_SIZE)
            finds = gm.scan_zones( [x,y, 10,10], [1,15,45] )
            del gm
            try:
                os.rename("/tmp/db_img.pgm", "/tmp/img_%03dx%03d.pgm" %(x,y))
            except OSError, e:
                pass
            if (x,y) in EXPECTED:
                if EXPECTED[(x,y)] != finds:
                    raise RuntimeError("Error: got %s, wanted %s" % (finds, EXPECTED[(x,y)]))


def py_rect_contents(gm, rect):
    pyfound = 0
    for y in range(rect[1], rect[3]+1):
        for x in range(rect[0], rect[2]+1):
            e = gm.get_point(x,y)
            pyfound |= e
    return pyfound


def point_in_poly(x, y, pairs):
    """Use trigonomtry to work out whether a point is in the shape"""
    import math
    dxys = []
    for p in pairs:
        if x == p[0] and y == p[1]:
            return True
        dxys.append((x - p[0], y - p[1]))
        
    
    atans = [math.atan2(dy, dx) for dy, dx in dxys]
    atans.sort()
    atans.reverse()
    s = atans[-1] + 2 * math.pi 
    for a in atans:
        if  s - a > math.pi + 0.001:
            #print "biggest angle is %s" % (a - s)
            return False
        s = a
    return True


def py_poly_contents(gm, points):
    found = 0
    points = points[:]
    ys = []
    xs = []
    while points:
        ys.append(points.pop())
        xs.append(points.pop())
        
    pairs = zip(xs, ys)

    for y in range(min(ys), max(ys) + 1):
        for x in range(min(xs), max(xs) + 1):
            if point_in_poly(x, y, pairs):
                found |= gm.get_point(x,y)                

    return found

    
def test_scan2(scans=30000, margin=10):
    random.seed(1)
    gm = scattered_images()
    gm.print_options()
    #gm.prescan_clear()
    errors = 0
    error_rects = []
    for i in range(scans):
        rect = [random.randrange(-margin, DEFAULT_SIZE[0] + margin),
                random.randrange(-margin, DEFAULT_SIZE[1] + margin),
                random.randrange(-margin, DEFAULT_SIZE[0] + margin),
                random.randrange(-margin, DEFAULT_SIZE[1] + margin)]
        if rect[0] > rect[2]:
            rect[0],rect[2] = rect[2],rect[0]
        if rect[1] > rect[3]:
            rect[1],rect[3] = rect[3],rect[1]
        pyfound = py_rect_contents(gm, rect)
        #c needs all corners explicitly
        cfound = gm.region_contents(rect + [rect[0],rect[3],rect[2],rect[1]]   )
        if pyfound != cfound:
            print "rect #%s %s.  python found %x, c found %x" %(i, rect, pyfound, cfound)
            error_rects.append((i, rect))
            errors += 1

    print "%s errors" % errors
    gm.save_map('/tmp/test2.pgm')
    red_background('/tmp/test2.pgm')
    try:
        red_background('/tmp/poly_test.pgm-lowres.pgm')
    except IOError, e:
        print e

    if error_rects and 0:
        n, r = error_rects.pop()
        for x in range(-15, 16):
            if x < 0:
                r2 = (r[0] + x, r[1], r[2], r[3])
            else:
                r2 = (r[0], r[1], r[2]+ x, r[3])
            im3 = im.crop(r2)
            im3.save('/tmp/test%s+%s.png' % (n,x))
            pyfound = py_rect_contents(gm, r2)
            print "rect %s (#%s + %s) contains %x" % (r2, n, x, pyfound)




def test_scan_irregular(scans=3000, margin=4):
    random.seed(1)
    gm = scattered_images(placements=50)
    gm.print_options()
    errors = 0
    gross_errors = 0
    error_rects = []
    for i in range(scans):
        points = []
        for j in range(4):
            points.append(random.randrange(-margin, DEFAULT_SIZE[0] + margin))
            points.append(random.randrange(-margin, DEFAULT_SIZE[1] + margin))

        pyfound = py_poly_contents(gm, points)
        cfound = gm.region_contents(points)
        #print "shape #%s %s.  python found %x, c found %x" %(i, points, pyfound, cfound)
        if pyfound != cfound:
            print ("shape #%s %s.  python found %x, c found %x, diff is %x" %
                   (i, points, pyfound, cfound, pyfound ^ cfound))
            im = Image.new('L', DEFAULT_SIZE, 0)
            pix = im.load()
            for j in range(0, len(points), 2):
                try:
                    pix[points[j], points[j + 1]] = 255
                except IndexError:
                    pass
            im.save('/tmp/poly-%s.png' % i)
            
            gross = True
            for offsets in ((-1,0), (1, 0), (0,-1), (0, 1)):
                points2 = []
                for i, p in enumerate(points):
                    points2.append(p + offsets[i & 1])
                pyfound2 = py_poly_contents(gm, points2)
                combined = pyfound | pyfound2
                fixed = (combined ^ cfound) == 0 or (pyfound2 ^ cfound) == 0 
                if fixed:
                    gross = False
                print ("shifted by %s found %s: %x error %x (combined %x, error %x) %s" %
                       (offsets, points2, pyfound2, pyfound2 ^ cfound, combined,
                        combined ^ cfound, "*" * fixed))
            gross_errors += gross
            errors += 1

    print "%s errors" % errors
    print "%s gross errors" % gross_errors
    gm.save_map('/tmp/poly_test.pgm')
    red_background('/tmp/poly_test.pgm')
    try:
        red_background('/tmp/poly_test.pgm-lowres.pgm')
    except IOError, e:
        print e




def test_speed():
    random.seed(1)
    images = 20
    cycles = 20000
    #gm = semanticCore.Gamemap(512, 384)
    gm = semanticCore.Gamemap(256,192)
    w,h = gm.size
    print '-' * 78
    print "placing %d images on map of (%d,%d), searching %d times on %s" % \
          (images, w, h, cycles, os.uname()[1])

    gm.print_options()
    scattered_images(gm, images)
    border = w * 4 // 3
    #gm.prescan_clear()
    rects = []
    rr = random.randrange
    for i in range(cycles):
        rw, rh = rr(1, 40), rr(1, 40)
        x, y, = rr(w - rw), rr(h - rh)
        rect = [x,y, rw, rh]
        #if not i % 6:
        #    gm.set_figure(0, 0, 0)
        #    gm.prescan_clear()
        cfound = gm.scan_zones(rect, [1, 15, 70])


if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option("-e", "--elements", action="store_true")
    parser.add_option("-c", "--correctness", action="store_true")
    parser.add_option("-C", "--correctness-irregular", action="store_true")
    parser.add_option("-s", "--speed", action="store_true")

    (options, args) = parser.parse_args()
    if options.elements:
        test_elements()
    elif options.correctness:
        test_scan2(10000) 
    elif options.correctness_irregular:
        test_scan_irregular(500)
    elif options.speed:
        try:
            import cProfile as profile
        except ImportError:
            #print "don't have cProfile in this old ancient python"
            import profile
        import pstats
        profile.run('test_speed()', PROFILE_FILE)
        p = pstats.Stats(PROFILE_FILE)
        p.sort_stats('cumulative').print_stats(10)
        #p.sort_stats('name').print_stats('semantic')
    else:
        test_elements()
        print "what?"
