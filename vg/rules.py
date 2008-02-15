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
"""Attempt to encapsulate rulesets

team sets can have
 player + any of
 [enemy, food, enemy-bullet, player-bullet, moving-enviroment, static-environment, magic?, ally?, enemy2?]
 (~40k or 362k)

9!/(8!) + 9!/2(7!) + 9!/6(6!) + 9!/24(5!) + 9!/5!(4!) + + 9!/6!(3!)

but some combinations are perhaps impossible/implausible
order is fixed.

"""

from vg import config
import yaml
import random
from bisect import bisect

class TeamRules:
    def __init__(self, data):
        #XXX use cleverer defaults.
        self.name = data['name']
        self.rules = data.get('rules', {})
        self.aims = data.get('aims', {})
        self.attention = data.get('attention', {})
        self.stacking_order = data.get('stack', 0)
        self.suitability_data = data.get('suitability', {})
        self.mind = data.get('mind')
        self.character = data.get('character', [])
        #non-neural character traits
        for c in ('bullet', 'magic'):
            setattr(self, c, c in self.character)
            if c in self.character:
                self.character.remove(c)
        self.directions = data.get('directions', [])
        self.steps = data.get('steps')
        self.intro_single =  data.get('intro_single')
        self.intro_plural =  data.get('intro_plural')
        self.sympathy =  data.get('sympathy', 0)
        self.sound_group = data.get('sound_group')
        self.sprite_group = data.get('sprite_group')
        self.depends = self.rules.get('depends', [])
        self.excludes = self.rules.get('exclude', [])
        self.score = data.get('score', 0)

    def suitability(self, team):
        #XXXX improve suitability, balance so every team has a chance.
        d = self.suitability_data
        s = 0
        #print d, self.name
        for attr in ('huge', 'tiny', 'monster_rating'):
            s += d[attr] * getattr(team, attr)
        return s

    def __str__(self):
        return "<team-type %s>" % self.name



NON_PLAYER_TYPES = []
PLAYER_TYPES = []
TYPE_MAP = {}

def _load_base_types():
    global PLAYER_TYPES, NON_PLAYER_TYPES, TYPE_MAP
    f = open(config.RULES_FILE)
    for x in yaml.load(f):
        t = TeamRules(x)
        if t.mind not in ('player', 'wall'):
            if not config.ENEMY_FIRES and t.name in ('enemy_bullet', 'enemy_guided_bullet'):
                continue
            if config.FRIENDLY_ONLY and t.sympathy < 0:
                continue
            if config.NASTY_ONLY and t.sympathy > 0:
                continue
            NON_PLAYER_TYPES.append(t)
        elif t.mind == 'player':
            PLAYER_TYPES.append(t)

    TYPE_MAP = dict((x.name, x) for x in PLAYER_TYPES + NON_PLAYER_TYPES)

_load_base_types()


def find_conflicts(types):
    """return a list of unmet dependencies in the set of types"""
    conflicts = {}
    names = set(t.name for t in types)
    for t in types:
        #print t, types
        s = {'excludes': [x for x in t.excludes if x in names],
             'requires': [x for x in t.depends if x not in names],
             }
        if s['excludes'] or s['requires']:
            conflicts[t.name] = s
    if conflicts:
        print "found conflicts %s" % conflicts
    return conflicts


def add_rules(player, others):
    """each passed in team gets appended with a set of rules"""
    #cycle until no conflicts found (conflicts are quite rare)
    types = random.sample(NON_PLAYER_TYPES, len(others))
    while find_conflicts(types):
        types = random.sample(NON_PLAYER_TYPES, len(others))
    #initial optimisation is simple and greedy
    unassigned = others[:]
    for x in types:
        s, team = max((t.suitabilities[x.name], t) for t in unassigned)
        print "picked %s for %s (suitability %s) out of %s" % (team, x, s, unassigned)
        team.apply_rules(x)
        unassigned.remove(team)

    player.apply_rules(random.choice(PLAYER_TYPES))



def mutate_rules(teams, direction):
    """give one of the teams a new set of rules. direction is -1 or 1
    depending on whether the mutation is wanted to shorten the player's
    life or not"""
    #XXX need also to mutate for player success duration, if that becomes an ending
    #XXX also, perhaps, mutate players
    #XXX also, make bullet and guided bullet exclusive? somehow.
    used = [x.rules for x in teams]
    non_players = [x for x in NON_PLAYER_TYPES if x not in used]

    #XXX probably round the wrong way:
    #better to pick a replacement, and look for the victim --
    # then walls are more likely to stay still
    for i in range(9):
        if len(teams) > 3:
            victim = min((v.rule_suitability, v) for v in random.sample(teams, 1 + len(teams) // 2))[1]
        else:
            victim = random.choice(teams)
        # pick a probably suitable set of rules
        candidates = []
        total = 0
        sample = random.sample(non_players, 5)
        for t in sample:
            total += victim.suitabilities[t.name] + 0.1
            candidates.append(total)
        n = bisect(candidates, random.uniform(0, total))
        rules = sample[n]
        #order doesn't matter for conflicts
        if find_conflicts([rules] + [x for x in used if x != victim.rules]):
            #this could be quite wasteful
            print "ignoring rule change %s -> %s" %(victim.rules, rules)
            continue

        gain = (rules.sympathy - victim.rules.sympathy) * direction
        if  gain > 0:
            victim.apply_rules(rules)
            break



def apply_rules(teams, rule_names):
    """the teams get given the named rule sets"""
    for t, name in zip(teams, rule_names):
        t.apply_rules(TYPE_MAP[name])
