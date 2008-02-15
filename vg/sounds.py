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


import random, os

from vg import config

import yaml
try:
    from yaml import CLoader as Loader
except ImportError:
    from yaml import Loader


class SoundManager:
    sounds = [] #constant across all instances
    sets = {}

    def __init__(self, filename=config.SOUND_YAML):
        if not self.sounds:
            self.load(filename)

    def load(self, filename):
        """Load up the class copies of yaml data"""
        f = open(filename)
        self.sounds[:] = yaml.load(f, Loader=Loader)
        f.close()
        for x in self.sounds:
            for s in x['suitability']:
                sounds = self.sets.setdefault(s, [])
                sounds.append(x)

    def pick_sounds(self, sound_group, exclude):
        """return a dictionary mapping event types to lists of sound
        file names.  sound_group is the kind of team asking for the
        sounds, and exclude is a set of sound types/directories not to
        be used (ie, have been used for other teams sounds)"""
        #try a few times to get one from a new set. It is not
        #impossible for a set to repeat, but is unlikely.
        for i in range(10):
            c = random.choice(self.sets.get(sound_group, self.sounds))
            if c['directory'] not in exclude:
                break

        # make an independent copy, with directory prepended.
        join = os.path.join
        exclude.add(c['directory'])
        sounds = {}
        for k, v in c['sounds'].items():
            sounds[k] = [ join(c['directory'], x) for x in v ]
        return sounds
