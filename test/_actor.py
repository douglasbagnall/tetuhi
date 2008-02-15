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

from vg import config
from vg import fakeblobdetect
from vg.display import Window
    

from vg.actor import Actor
from vg import weights

class FakeGameMap:
    def __init__(self):
        pass
    def add_element(self, *args):
        pass

class FakeGame:
    def __init__(self, n_teams=2):
        self.n_teams = n_teams
        self.window = Window(config.WINDOW_SIZE)
        self.map = FakeGameMap()




def test_neural():
    # a cyanish rectangle with eyes.
    sprite = fakeblobdetect.fake_sprite((25, 30), (0,180,120), (100,100))
    game = FakeGame()
    actor = Actor(sprite, game, 1)
    for w in ([weights.WALL, weights.ALLY, weights.CHASER],
              [weights.WALL, weights.CHASEE, weights.ALLY],):                  
        actor.set_weights(w)
        print w
        for b0 in range(16):
            for b1 in range(16):
                for b2 in range(16):
                    inputs = [0] * actor.net.shape[0]
                    inputs[b0] = 1
                    inputs[16 + b1] = 1
                    inputs[32 + b2] = 1
                    actor.decide(inputs=inputs)
                    print inputs, actor.intention
    




if __name__ == '__main__':
    test_neural()


