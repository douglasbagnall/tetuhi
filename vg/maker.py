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
"""invent a game using the blobs of a blobfinder"""

import pygame
import cPickle, os
import time, traceback

from vg import config
from vg.misc import SpriteError, ParseError
from vg.game import Game

class GameMaker:
    """manages the production of a working game, mediating between
    blobfinder and games."""
    def __init__(self, bf, window):
        """Might raise a parseError if the blobfinder doesn't work"""
        self.window = window
        self.entropy = bf.entropy

        #parse might raise ParseError exception, which outer caller should catch.
        bg, sprites, palette = bf.parse(config.MIN_SPRITES, config.IDEAL_SPRITES[0],
                                        config.IDEAL_SPRITES[1], config.MAX_SPRITES)

        window.set_palette(palette)
        self.sprites = sprites
        self.background = bg


    def find_game(self, fork=True):
        """return a game, presumably the best one possible.  If none
        can be found, a SriteError might be raised."""
        g = Game(self.background, self.sprites, self.window, entropy=self.entropy)
        g.grow_rules_in_fork()
        g.finalise_rules()
        g.finalise_display() #sets up sprites and ties sound to the window.
        return g

    def evolve_game(self, fork=True):
        #entropy gets rotated, so each game evolves differently. (unless config.PROCESSES is large (160 or so))
        entropy = self.entropy[:]
        games = []
        for x in range(config.PROCESSES):
            entropy = entropy[1:] + entropy[0]
            g = Game(self.background, self.sprites, entropy=entropy)
            g.grow_rules_in_fork()
            games.append(g)

        running = {}
        #run each game's evolution in its own process
        for game in games:
            pid = game.evolve(fork=fork)
            running[pid] = game

        #wait for any old subprocess.
        if fork:
            timeout = time.time() + config.PROCESS_TIMEOUT
            while running and time.time() < timeout:
                time.sleep(0.5)
                pid, status = os.waitpid(-1, os.WNOHANG)
                if pid in running:
                    print "got pid %s" % pid
                    game = running.pop(pid)
                    game.reap_evolution()

            for pid in running: #kill slowcoaches, if any
                os.kill(pid, 9)

        best = games[0]
        for g in games:
            if g.score > best.score:
                best = g

        best.set_window(self.window)
        return best
