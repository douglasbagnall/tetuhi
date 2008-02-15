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

from vg import assort

import Image, random

#random.seed(1)

SIZE = 100
VECTORS = 15

class TestVector(assort.SpriteVector):
    """for testing. Evaluates numeric sequences."""
    adjustment = 1
    def evaluate(self, sprite):
        self.extend(sprite)
        self.sprite = sprite

    def calculate_adjustment(self, mean_distance):
        pass


def test2d():
    
    vectors = [ (random.randrange(SIZE), random.randrange(SIZE)) for x in range(VECTORS) ]
    print "test vectors are %s" % vectors

    sorter = assort.TeamFinder(vectors, klass=TestVector)
    teams = sorter.find_teams()
    print teams

    im = Image.new('RGB', (SIZE, SIZE), (255,255,255))
    pix = im.load()
    print teams
    for t, c in zip(teams, ((160,0,0), (160,160,0), (0,160,0), (0,160,160), (0,0,160), (1600,0, 160))):
        for x in t:
            pix[x[0], x[1]] = c

    im.save('/tmp/test.png')



if __name__ == '__main__':
    test2d()
        
                            

    
