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
"""Detect blobs in a picture, making use of the imagescan module
defined in img-c/img.c for the inner loops."""

import sys, os
import Image, ImageFilter, ImageOps, ImageChops
from time import sleep, time
import sha, array

from vg.display import stretch
from vg import config
from vg.misc import ParseError
import imagescan

BLACK_SCREEN = True

class Blob:
    """An image segment that could be turned into a sprite.
    """
    crowded = False
    def __init__(self, elements, picture):
        """lazy merging of the elements -- only the bounding rectangle
        is calculated now, because it is quite possible that the
        blob will be thrown out"""
        self.elements = tuple(elements)
        first = self.elements[0]
        left, top, right, bottom = first.pos
        self.volume = first.volume
        self.colour = first.colour
        self.bgcolours = picture.usedcolours
        self.obviousness_best = first.obviousness_best
        self.obviousness_sum = first.obviousness_sum
        #self.obviousness_global = 
        og = first.obviousness_global * first.volume
        for e in self.elements[1:]:
            left = min(e.pos[0], left)
            top = min(e.pos[1], top)
            right = max(e.pos[2], right)
            bottom = max(e.pos[3], bottom)
            #sum volume and colour info
            v = self.volume + e.volume
            self.colour = [ (a * self.volume + b * e.volume + v // 2) // v
                            for a, b in zip(self.colour, e.colour) ]
            self.volume = v
            self.obviousness_best = max(self.obviousness_best, e.obviousness_best)
            self.obviousness_sum += e.obviousness_sum
            og += e.obviousness_global * e.volume
            
        self.obviousness_global = og / self.volume
            
        self.pos = (left, top, right, bottom)
        self.width = right - left
        self.height = bottom - top
        self.silhouette = None

    def viable(self):
        """true if it is big  enough to survive."""
        return (self.width > 1 and self.height > 1 and
                self.obviousness_best > config.MIN_OBVIOUSNESS_BLOB and 
                self.obviousness_sum > config.MIN_OBVIOUSNESS_BLOB_SUM and 
                self.obviousness_global > config.MIN_OBVIOUSNESS_BLOB_GLOBAL)


    def finalise(self):
        """do all the complex work, after the selection has been
        finalised"""
        size = config.WORKING_SIZE

        im = Image.new('RGBA', size, config.CHROMA_KEY_COLOUR + (0,))

        mask = Image.new('L', size, 0)
        for e in self.elements:
            mask.paste(255, (e.pos[0], e.pos[1]), e.mask)

        im.paste(self.elements[0].im, (0,0), mask)
        self.im = im.crop(self.pos)
        self.im.load()
        pix = self.im.im.pixel_access()

        self.silhouette = mask.crop(self.pos)
        self.silhouette.load()
        spix = self.silhouette.im.pixel_access()
        dbim = self.silhouette.copy()

        #calculate some numbers for classification
        self.obviousness =  imagescan.obviousness(pix, self.bgcolours)
        self.perimeter, self.area = imagescan.surface_area(spix)
        self.convex_area, self.convex_perimeter, self.convex_length, self.convex_width, self.tilt = \
                          imagescan.convex_hull_size(spix, dbim.load())

        self.tiny = self.convex_perimeter <= config.TINY_SPRITE_SIZE
        self.huge = self.convex_perimeter > config.HUGE_SPRITE_SIZE
        self.aspect = float(self.height) / self.width 
        
        q1, q2, q3, q4 = imagescan.symmetry(spix)
        self.leftheavy = (q1 - q2 + q3 - q4)
        self.topheavy = (q1 - q3 + q2 - q4)


        #calculate solidity and save a demo image
        sim = Image.new('L', self.im.size)
        self.solidity = imagescan.solidity(pix, sim.load())
        if config.PROLIFIC_SAVES:
            sim.save('/tmp/solidity-%d-%d-%d-%d.png' % self.pos)            
            #save convexity image
            dbim.save('/tmp/convex-%d-%d-%d-%d.png' % self.pos)

        if config.USE_CHROMA_KEY:
            self.picture = self.im.convert('RGB')
        else:
            self.picture = self.im

        for e in self.elements:
            e.shrink()


    def __str__(self):
        return ("<Blob: solidity %s area: %s perimeter: %s convex a: %d convex p: %s tiny: %s huge: %s\n"
                "volume: %s width: %s height : %s>" %
                (self.solidity, self.area, self.perimeter, self.convex_area, self.convex_perimeter,
                 self.tiny, self.huge, self.volume, self.width, self.height))

    def __repr__(self):
        return "<Blob convex area: %s, perimeter: %s>" % (self.convex_area, self.convex_perimeter)


    def get_rotations(self, team_style=None, angle=45, scale=None):
        """get versions of the image for each compass direction"""
        #XXX maybe flips would be better in some circumstances.
        #XXX animate here too. need to get indication from actor of correct kind of motion.
        #XXX motion is arranged in teams.

        #print team_style, self.pos
        angles = range(360, 0, -angle)
        if self.silhouette is None:
            self.finalise()
        picture = self.picture
        silhouette = self.silhouette
        w, h = self.width, self.height
        if self.crowded and scale is None:
            scale = 0.8
        
        if scale is not None:
            w = int (w * scale)
            h = int (h * scale)
            picture = picture.resize((w, h))
            silhouette = silhouette.resize((w, h))
        if team_style == 'static':
            plain = [picture, silhouette]
            diag = [x.resize((w * 39 // 40, h  * 39 // 40)) for x in plain]
            return [plain, diag] * 4
                        
        elif team_style == 'mirror':
            right = [picture, silhouette]
            if w < 5 or h < 5:
                return [right] * 8
            updown = [x.resize((w * 9 // 10, h * 10 // 9)) for x in right]
            diagright = [x.resize((w * 19 // 20, h * 20 // 19)) for x in right]
            diagleft = [x.transpose(Image.FLIP_LEFT_RIGHT) for x in diagright]
            left = [x.transpose(Image.FLIP_LEFT_RIGHT) for x in right]
            return [updown, diagright, right, diagright, updown, diagleft, left, diagleft]

        elif team_style == 'lean':
            centre = [picture, silhouette]
            if w < 5 or h < 5:
                return [centre] * 8
            c = 1 + self.convex_perimeter // 60
            up = [x.transform(x.size, Image.QUAD,    (-0,-0, -c,h+c, w+c,h+c, w+0,-0)) for x in centre]
            down = [x.transform(x.size, Image.QUAD,  (-c,-c, -0,h+0, w+0,h+0, w+c,-c)) for x in centre]
            left = [x.transform(x.size, Image.QUAD,  (-0,-0, -0,h+0, w+c,h+c, w+c,-c)) for x in centre]
            right = [x.transform(x.size, Image.QUAD, (-c,-c, -c,h+c, w+0,h+0, w+0,-0)) for x in centre]
            return [up, centre, right, centre, down, centre, left, centre]


        else: #'directional'            
            ims = [(picture.rotate(x, expand=True), silhouette.rotate(x, expand=True))
                   for x in angles]
            #for p, s in ims:
            #    print p.size, s.size
            return ims

    def dying_image(self):        
        im = Image.new('RGBA', self.im.size, (0,0,0,0))
        pix = self.im.im.pixel_access()
        #print im, self.im, im.load(), self.im.load()
        imagescan.scramble(im.load(), pix)
        #second image, further scrambled
        #im2 = Image.new('RGBA', self.im.size, (0,0,0,0))        
        #imagescan.scramble(im2.load(), im.load())
        if config.PROLIFIC_SAVES:        
            im.save('/tmp/dying-%d-%d-%d-%d.png' % self.pos)            
            #im2.save('/tmp/dying2-%d-%d-%d-%d.png' % self.pos)            

        
        return im



class Element:
    """Representing an image segment on the page.  A blob can contain one or more of these"""
    def __init__(self, im, alpha, start, bgcolours):
        """im: the image being parsed
        alpha: a alpha layer of the image
        start: point where the new blob has been found"""
        #it doesn't really need a copy yet, (because it doesn't crop)
        self.im = im

        mask = Image.new('L', im.size, 0)
        mpix = mask.load()
        pix = alpha.load()

        # blank out the image in alpha, draw it in mask, and return the edge coordinates.
        self.pos, self.colour, self.volume, self.obviousness_best, self.obviousness_sum, og = \
                  imagescan.get_element(pix, mpix, im.load(), start[0], start[1], bgcolours)
        self.obviousness_global = og ** 0.5

        #print "obviousness is best:%s sum:%s avg: %s; global: %s, sqrt-> %s" %\
        #      (self.obviousness_best, self.obviousness_sum,
        #       self.obviousness_sum / self.volume, og, self.obviousness_global)
        #only save the containing rectangle
        self.mask = mask.crop(self.pos)
        #NB. in PIL 1.1.6, load()ing a cropped image doesn't return the pixel_access object
        self.mask.load()
        self.mpix = self.mask.im.pixel_access()

        isize = (self.mask.size[0] + imagescan.INFLUENCE_DIAMETER,
                 self.mask.size[1] + imagescan.INFLUENCE_DIAMETER)
        self.influence = Image.new('L', isize, 0)
        self.ipix = self.influence.load()
        # work out influence based on mask
        imagescan.influence_map(self.mpix, self.ipix)
        self.group = set([self])

    def contains(self, e):
        return (self.pos[0] <= e.pos[0] and  self.pos[1] <= e.pos[1] and
                self.pos[2] >= e.pos[2] and  self.pos[3] >= e.pos[3])

    def intersects(self, e):
        #true if they both have overlapping convex hulls, and the
        #smaller one contains some pixels of the larger
        return imagescan.intersect(self.mpix, e.mpix, self.pos, e.pos)

    def too_big(self):
        v1 = self.volume > self.im.size[0] * self.im.size[1] / 3
        v2 = self.volume > self.im.size[0] * self.im.size[1] / 4
        e = self.mask.size[0] == self.im.size[0] and self.mask.size[1] == self.im.size[1] 
        return v1 or (e and v2) 

    def viable(self):
        #true if the element is of a size that could survive as a blob by itself
        big = self.mask.size[0] > 1 and self.mask.size[1] > 1
        obvious = ((self.obviousness_best >= config.MIN_OBVIOUSNESS_ELEMENT and
                    self.obviousness_sum >= config.MIN_OBVIOUSNESS_ELEMENT_SUM) or
                   self.obviousness_global > config.MIN_OBVIOUSNESS_ELEMENT_GLOBAL)
        #if not obvious:
        #    print "refusing element with obviousness (%s, %s)" % (self.obviousness_best, self.obviousness_sum)
        return big and obvious

    def colour_distance(self, e):
        """how different are the elements' colours in RGB space"""
        return sum((a - b) ** 2 for a, b in zip(self.colour, e.colour)) ** 0.5

    def shrink(self):
        """attempt to free as much memory as possible"""
        try:
            del self.mpix
            del self.ipix
            del self.mask
            del self.im
            del self.influence
        except AttributeError:
            #print "seem to be shrinking element twice!"
            pass

def fast_median(im):
    """make a duplicate image, with each pixel set to the median of
    its 3x3 neighbourhood. each band is treated independantly"""
    im2 = Image.new('RGB', im.size)
    pix_in = im.load()
    pix_out = im2.load()
    imagescan.fast_median(pix_in, pix_out)
    return im2


def collect_entropy(im, bits=config.ENTROPY_BITS):
    """collect <bits> bits of entropy from the image."""
    i = im.tostring()
    s = sha.new()
    e = []
    blocks = bits // sha.digest_size
    q = len(i) // blocks
    for x in range(blocks):
        s.update(i[x * q: x * q + q])
        e.append(s.digest())
    #entropy is most use as a list of ints
    #interleave the blocks, so each part of the image has practical effect.
    #(a bit silly, nonetheless)
    e2 = ''.join(sum(zip(*e), ()))
    return list(array.array("l", e2))

def avoid_chromakey(im):
    """replace the chroma colour by one blue pixel off"""
    k = config.CHROMA_KEY_COLOUR
    ob = k[2]
    if ob > 128:
        nb = ob - 1
    else:
        nb = ob + 1
    r = range(256)
    lut = r + r# + range(ob) + [nb] + range(ob + 1, 256)
    r[ob] = nb
    lut += r
    return im.point(lut)


# finding and linking elements

def find_elements(im, bgcolours):
    """find separable elements in the alpha array, and create
    Element objects for each."""
    w, h = im.size
    #alpha will end up blank, as all the elements are found and removed.
    alpha = im.split()[3]
    apix = alpha.load()
    elements = []
    for x in range(w):
        for y in range(h):
            if apix[x, y]: #found one
                e = Element(im, alpha, (x, y), bgcolours)
                if e.too_big():
                    #XXX try special splitting?
                    print "not proceeding with an element of size %s" % e.volume
                    # returning single element array causes extra clearing
                    return [e]
                elements.append(e)
                if config.PROLIFIC_SAVES:
                    e.influence.save('/tmp/influence-%sx%s.png' % e.pos[:2])
    return elements

def crosslink_elements(elements):
    """work out how interlinked different elements seem to be"""
    print "trying to crosslink"
    connections = []
    for i, e1 in enumerate(elements):
        for e2 in elements[i+1:]:
            #print (e1.ipix, e2.ipix, e1.pos, e2.pos, config.WORKING_SIZE);
            c = imagescan.connectedness(e1.ipix, e2.ipix, e1.pos, e2.pos, config.WORKING_SIZE)
            if c:
                #print "found connectedness of %s between elements at %s & %s" % (c, e1.pos, e2.pos)
                #look for additional connectedness eg: envelopment.
                if e1.contains(e2) or e2.contains(e1):
                    c *= 2
                    #print "coincident: connected is now %s" % c
                if e1.intersects(e2):
                    c *= 4
                    #print "intersect: connected is now %s" % c
                if not e1.viable() or not e2.viable():
                    c *= 4
                    #print "viability: connected is now %s" % c
                if e1.volume < config.TINY_VOLUME or e2.volume < config.TINY_VOLUME:
                    #config.TINY_VOLUME is quite a big bigger than config.TINY_SPRITE_SIZE
                    c *= 2
                    #print "tiny: connected is now %s" % c
                    
                d = e1.colour_distance(e2)
                #print d
                if d > 15:
                    # 2/3 or 2/4 or 2/6 or 2/10
                    c *= 2
                    c //= 3 + (d > 30) + (d > 60) * 2  + (d > 90) * 4
                    #print "colour distance: connected is now %s" % c


                connections.append((c, e1, e2))

    connections.sort()
    connections.reverse()
    print "found %s connections" % len(connections)
    return connections


def find_common_colours(im, buckets=config.HISTOGRAM_BUCKETS):
    """make a three dimensional histogram"""
    #im = im.convert('RGB')
    colours = imagescan.find_background(im.load())
    colours.sort()
    colours.reverse()
    return [ (rgb & 0xff, (rgb >> 8) & 0xff, (rgb >> 16) & 0xff)
             for count, rgb in colours ]




class BlobFinder:
    """detects blobs in an image"""
    window = None
    path = '/tmp/'
    im = None #base image
    clean = None
    times = []
    blobs = None
    bgcolours = None

    def __init__(self, filename, window=None):
        self.window = window
        # make a place to save the files.
        self.path = os.path.dirname(filename)

        orig = Image.open(filename)
        shrunk = stretch(orig, config.WORKING_SIZE)
        median = fast_median(shrunk)        
        if config.USE_CHROMA_KEY:
            median = avoid_chromakey(median)

        #XXX should entropy use fullsize, shrunk, or median?
        # median loses entropy, but makes the actually processed picture canonical
        # fullsize makes the photograph canonical, but is slow. shrunk is compromise.
        self.entropy = collect_entropy(median)

        self.save(median, 'median')
        self.im = median.convert('RGBA')
        self.clean = self.im.copy()
        self.display(self.im, capture_n=25)

        self.bgcolours = find_common_colours(self.clean)
        self.usedcolours = []



    def parse(self, min_blobs, ideal_blobs_min, ideal_blobs_max, max_blobs):
        """find the blobs and background"""
        threshold = config.SUBSUME_THRESHOLD_INIT

        for i in range(len(self.bgcolours)):
            print "round %d" % i
            self.blank_background() #may raise a ParseError
            self.display(self.clean, BLACK_SCREEN, capture_n=12)
            if config.PROLIFIC_SAVES:
                self.save(self.clean, 'clean_%s' % i)

            elements = find_elements(self.clean, self.usedcolours)
            #if there are insufficient elements, nothing can be done now. Try extra blanking.
            if len(elements) < min_blobs:
                print "found only %d elements" % len(elements)
                if len(elements) == 0:
                    raise ParseError("The drawing seems to be blank!")

                #XXX attempt to split (if bgcolours are running out?)
                for n in range(config.BLANKS_PER_CYCLE - 1):
                    self.blank_background()

                continue

            # only get to here when enough elements are found.
            self.elements = elements
            self.connections = crosslink_elements(elements)

            blobs = []
            while threshold < config.SUBSUME_THRESHOLD_MAX and len(blobs) < min_blobs:
                blobs = self.find_blobs(min_blobs, threshold)
                threshold *= 2
            threshold //= 2
            #are there enough to abandon this damn loop?
            if len(blobs) >= min_blobs:
                break

                
        #there are enough blobs -- but are there too many?
        # for this hardish maximum, go right down to a low threshold of connection.
        print "threshold is %d, blobs are %d, max_blobs %d, thrshold min %d" % \
              (threshold, len(blobs), max_blobs, config.SUBSUME_THRESHOLD_MIN)
        while len(blobs) > max_blobs and threshold > config.SUBSUME_THRESHOLD_MIN:
            threshold //= 2
            print "trying to reduce: threshold is %d, blobs are %d" % (threshold, len(blobs))
            blobs = self.find_blobs(min_blobs, threshold)


        # now the number of blobs is OK. However, slight adjustments
        # might give something better.  Have a go at hitting the ideal
        # blob number.
        if len(blobs) > ideal_blobs_max:
            blobs = self.find_blobs(min_blobs, threshold * 2)
        elif len(blobs) < ideal_blobs_min:
            blobs = self.find_blobs(min_blobs, threshold // 2)

        self.finalise_blobs(blobs)
        background = self.grow_background()
        #memory kindness -- any point though?
        self.shrink()

        #now, some common colours are background. The most different common colours are
        #the palette
        palette = self.bgcolours[:]
        for bgc in self.usedcolours:
            for i, c in enumerate(palette):
                if sum((x - y) ** 2 for x, y in zip(c, bgc)) < config.PALETTE_DISTINCTION:
                    palette[i] = None
            palette = [x for x in palette if x is not None]
        if not palette:
            palette = [(0,0,0)]

        return (background, blobs, palette)
        #now what? start arbitrary carve up?

    def shrink(self):
        for e in self.elements:
            #catch any that weren't included in blobs.
            e.shrink()
        try:
            del self.elements
            del self.connections
        except AttributeError:
            print "tried shrinking blobfinder twice"



    def blank_background(self):
        """expand a region with pixels near to <colour>, varying by at most
        <threshold> from each other"""
        #XXX could use adaptive threshold, depending on recent variations.
        try:
            colour = self.bgcolours.pop(0) #raises IndexError if empty
        except IndexError, e:
            print e
            raise ParseError("Sorry, I can't find the game here.")
        
        self.usedcolours.append(colour)
        print "clearing background colour %s" % (colour,)

        im = self.clean
        r, g, b = colour
        rgb = (b << 16) + (g << 8) + (r)

        def _display(x):
            if config.DISPLAY_EVERYTHING:
                self.display(im, True)

        imagescan.expand_region(im.load(), [rgb], config.EXPAND_THRESHOLD_2,
                                config.EXPAND_INIT_THRESHOLD_2, _display)
        return im

    def grow_background(self):
        """fill in the bits where the background was obscured by blobs"""
        im = self.clean.copy()
        self.display(im, True, capture_n=3)
        c = (x for x in range(200))
        def _display(x):
            if config.DISPLAY_EVERYTHING:
                self.display(im, True)

        imagescan.grow_background(im.load(), _display)
        im = im.convert('RGB')
        if config.PROLIFIC_SAVES:
            self.save(im, 'background')
        #self.background = im
        self.display(im, capture_n=12)
        return im


    def ungroup_elements(self):
        for e in self.elements:
            e.group = set([e])

    def group_elements(self, threshold):
        print "trying with subsume threshold %s" % threshold
        # collect together interlinkable elements
        self.ungroup_elements()
        for d, e1, e2 in self.connections:
            if d < threshold:
                break
            g = e1.group | e2.group
            for e in g:
                e.group = g

        groups = []
        for e in self.elements:
            if e.group not in groups:
                groups.append(e.group)

        print [len(x) for x in groups]
        print "made %s groups containing %s elements from %s elements" %  \
              (len(groups), sum(len(x) for x in groups), len(self.elements))
        return groups


    def make_blobs(self, groups):
        """collect together a number of elements in to a blob"""
        blobs = []
        for g in groups:
            s = Blob(g, self)
            #print "dealing with blob of size %s, %s, volume %s, obviousness %s" % (s.width, s.height, s.volume, s.obviousness_best)
            if s.viable():
                blobs.append(s)

        return blobs


    def find_blobs(self, min_blobs, threshold):
        """given a stripped down image, make individual images for each bit"""
        groups = self.group_elements(threshold)
        if len(groups) < min_blobs:
            print "insufficient groups!"
            return []
        blobs = self.make_blobs(groups)
        return blobs

    def finalise_blobs(self, blobs):
        #so they are now real and can be finalised
        for x, s in enumerate(blobs):
            s.finalise()
            if config.PROLIFIC_SAVES:
                self.save(s.picture,'colorkey-%s' %(x)) #XXX shift to method?
                self.save(s.im,'alpha-%s' %(x))

        red = (255, 0, 0, 255)
        orange = (225, 200, 0, 255)
        im = self.clean.copy()
        self.display_boxes(self.elements, im, orange, 12)
        self.display_boxes(blobs, im, red, 25)

        #sleep(10)
        if config.PROLIFIC_SAVES:
            self.save(im, 'map')
        return blobs

    ####

    def display_boxes(self, set, im, colour, capture_n=25):
        if config.DISPLAY:
            pix = im.load()
            for s in set:
                #print s.pos
                x1, y1, x2, y2 = s.pos
                x2 -= 1
                y2 -= 1
                for a in range(x1, x2):
                    pix[a, y1] = colour
                    pix[a, y2] = colour
                for a in range(y1, y2):
                    pix[x1, a] = colour
                    pix[x2, a] = colour

        self.display(im, capture_n=capture_n)

    def display(self, im, wipe=False, capture_n=1):
        """show the picture, if a display window is defined"""
        if self.window is not None:
            self.window.display(im, wipe, capture_n)

    def save(self, im, subname):
        im.save(os.path.join(self.path, subname + '.png'))



    #### Utility functions of dubious utility

    def timer(self, msg):
        self.times.append((time(), msg))

    def time_summary(self):
        ts = self.times[0][0]
        for t, msg in self.times:
            print "%5.3f %5.3f   %s" % (t - ts, t - self.times[0][0], msg)
            ts = t
