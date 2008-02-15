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
import os, sys

import yaml_common


#from vg import config
from vg.config import SOUND_YAML

#SRC_DIR = '/home/douglas/sounds/tetuhi/categorised/'
SRC_DIR = '/home/douglas/sounds/short2/'
SOUND_YAML =  '/tmp/sound.yaml'


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



player = Player()

#YAML =  '/home/douglas/tetuhi/' + SOUND_YAML[2:]
SOUND_YAML =  '/tmp/sound.yaml'


def play_and_display(f, d):
    if f:
        p = os.path.join(SRC_DIR, d, f)
        print '  %25s %7s' %(f, os.path.getsize(p))
        player.play(p)


def play_set(set):
    print set['name']
    for k, v in set['sounds'].items():
        if k == 'misc':
            continue
        print k

        if isinstance(v, str):
            play_and_display(v, set['directory'])

        else:
            for f in v:
                play_and_display(f, set['directory'])



def categorise():
    data = yaml_common.yaml_load(SOUND_YAML)
    random.shuffle(data)
    for s in data:
        if 'suitability' in s:
            continue
        #s['directory'] = s['name'][0]
        play_set(s)
        print "unsuitable for Monster, Environment, Player, Ally, Food/prey, aNybody; or Good all round. blank: repeat"
        a = None
        while not a or not a.issubset(set('mepafng')):
            a = set(raw_input('> ').strip().lower())
            if not a:
                play_set(s)

        if 'n' in a:
            s['suitability'] = []
        elif 'g' in a:
            s['suitability'] = ['monster', 'environment', 'player', 'ally','food']
        else:
            s['suitability'] = [x for x in ['monster', 'environment', 'player', 'ally','food'] if x[0] not in a]



SET = 36

def randomise():
    data = yaml_common.yaml_clear()

    root = SRC_DIR
    sizeof = os.path.getsize
    join = os.path.join
    for d in os.listdir(root):
        d2 = os.path.join(root, d)
        files = os.listdir(d2)
        random.shuffle(files)
        free = len(files)
        used = 0
        i = 0
        while used < free:
            name = '%s_%s' % (d, i)
            sample = [y[1] for y in sorted((sizeof(join(d2, x)), x) for x in files[used:used + SET])]
            if len(sample) < 26:
                break
            turning = [None] + sample[:7]
            blocked = sample[8:16]
            hit = sample[16]
            fire = sample[17]
            excited = sample[18:26]
            misc =  sample[26:]
            random.shuffle(blocked)
            random.shuffle(turning)
            s = {'name': name,
                 'directory': d,
                 'sounds':{'hit':     files[used],
                           'turning': turning,
                           'blocked': blocked,
                           'excited': excited,
                           'fire'   : fire,
                           'misc'   : misc
                           }
                 }
            data.append(s)

            used += SET
            i += 1
    print "%s sets made" % len(data)
    yaml_common.yaml_save(SOUND_YAML)


def analyse():
    toplevel = set(['directory', 'name', 'sounds', 'suitability'])
    sounds = {'hit': 8, 'excited': 8, 'fire': 1, 'blocked': 1, 'misc': 10, 'turning': 7}
    i_sounds = []
    data = yaml_common.yaml_load(os.path.join(SOUND_YAML))
    for s in data:
        name = s['name']
        if toplevel.difference(s):
            print "%s has top level %s, not %s" (name, s.keys(), toplevel)
        for k, v in sounds.items():
            if len(s['sounds'].get(k,[])) != v:
                print "%s has %s of %s, not %s" %(name, len(s['sounds'].get(k,[])), k, v)
            if name.startswith('i'):
                i_sounds.extend(s['sounds'][k])

    print i_sounds
    i_files = os.listdir(SRC_DIR + '/i')
    print [x for x in i_files if x not in i_sounds]



if '-r' in sys.argv:
    randomise()
elif '-a' in sys.argv:
    analyse()

else:
    try:
        categorise()
    finally:
        yaml_common.yaml_save(SOUND_YAML)
