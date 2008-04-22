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
"""Endlessly loop, trying to get people to play the game"""
import sys, traceback, time
import random, os

from vg import config, utils

from vg.misc import AttractEscape, GameReplay


class Attractor:
    last_game = None
    def __init__(self, window):
        self.window = window
        self.window.set_palette([(0xcc, 0, 0)])
        try:
            self.window.display(config.ATTRACTOR_BACKGROUND)
        except IOError, e:
            self.window.display_colour((255,255,255))
        self.window.write("starting...")

    def attract(self):
        """Play video until someone presses the button.  Then take a
        photo, save it, and return the file name"""
        if self.last_game:
            try:
                self.text_cycle(config.COUNTDOWN_CYCLE)
            except AttractEscape:
                raise GameReplay(self.last_game)
        try:
            while True:
                for text in (config.INSTRUCTIONS_CYCLE,
                             config.CREDITS_CYCLE):
                    self.text_cycle(text)
                    self.replay(12)

        except AttractEscape:
            #take the photo here? or in the outer loop?
            return self.take_photo()


    def take_photo(self):
        """Trigger the camera in a background process, and wait for the picture to arrive"""
        #utils.make_dir(config.PHOTO_DIRECTORY)
        format = "%Y-%m-%d_%H-%M-%S/original." + config.CAPTURE_IMAGE_SUFFIX
        filename = os.path.join(config.PHOTO_DIRECTORY, time.strftime(format))
        def photograph():
            os.system(config.CAMERA_SCRIPT + ' ' + filename)

        def display():
            self.window.unwrite()
            self.window.write(random.choice('"#*+*oO0.'))
            self.window.alert_pause(0.5)

        utils.process_in_fork(photograph, display, 1, config.CAMERA_TIMEOUT)
        #XXX need error checking
        return filename



    def remember_game(self, game):
        """save the game so it can be used for attractive replays"""
        self.last_game = game


    def text_cycle(self, filename, interuptable=True):
        """play the text in the file"""
        def fire_handler():
            if interuptable:
                raise AttractEscape("time to take a photo!")

        self.window.text_cycle(filename, fire_handler=fire_handler)


    def replay(self, duration):
        if self.last_game:
            self.last_game.play(replay=duration)


    def apologise(self, exception):
        """nothing much to do but carry on"""
        print exception
        self.window.unwrite()
        self.window.notice(str(exception))
        self.window.alert_pause(5)
