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


import ossaudiodev, wave, random
import os, pickle

SRC_DIR = '.'
OUT_FILE = '/tmp/sound_categories'

class Player:
    def __init__(self):
        out = ossaudiodev.open('w')
        out.channels(1)
        out.setfmt(ossaudiodev.AFMT_S16_LE)
        out.speed(44100)
        self.channel = out
        self._play = out.writeall


    def play(self,fn):
        wav = wave.open(fn)
        self._play(wav.readframes(999999))


class Classifier:
    def __init__(self, player, directory):
        self.store = {}
        self.categories = {}
        self.player = player
        self.directory = directory
        self.files = [x for x in os.listdir(directory) if 'mono' in x]
        random.shuffle(self.files)
        self.regenerate_prompt()

    def regenerate_prompt(self):
        self.prompt = "Pick one of the following categories, or enter something else to create a new one:\n"
        keys = self.categories.keys()
        keys.sort()
        index = ['%4s: %20s' %(x, self.categories[x]) for x in keys]
        if len(index) < 4:
            self.prompt += '\n'.join(index)
        else:
            self.prompt += '\n'.join(("%s %s" % x for x in zip(index, index[len(index)/2])))



    def _categorise(self, fn):
        print self.prompt
        a = ''
        while a == '':
            player.play(os.path.join(self.directory, fn))
            a = raw_input('> ').strip()

        if a in self.categories:
            self.store[fn] = self.categories[a]
        else:
            print "new category? (leave empty to ignore)"
            b = raw_input('- ').strip()
            if b:
                self.categories[a] = b
                self.store[fn] = b
                self.regenerate_prompt()

    def categorise(self):
        for fn in self.files:
            self._categorise(fn)
            self.save(OUT_FILE)

    def save(self, fn):
        f = open(fn, 'w')
        pickle.dump([self.categories, self.store], f)
        f.close()

if __name__ == '__main__':
    player = Player()
    c = Classifier(player, SRC_DIR)
    c.categorise()
