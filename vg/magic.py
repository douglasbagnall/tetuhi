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
"""magic that can happen when the player eats magic food
(XXX or hits the button in no shooting games?)

methods might be:

shrink
grow
others shrink, grow
speed up
move differently
inexorable force for everyone (gravity).
rule change
 - predator/prey swap
 - immunity

swap sides -- control other teams, briefly.

"""
#XXX this is extraordinarily last-minute and hacky

from vg import config

TIMEOUT = 100

#XXX unimplemented effects
def immunity():
    """player.lives ++"""

def monsters_flee():
    """how to do this?"""

def pre_zoom(team, size):
    """make up a new set of images for the team in the new size (ratio)."""

def zoom(team):
    """clear the teams images. (being sure to leave no trace on the gamemap).
    Perhaps, alert the player.  Blit in the new ones.
    """


class Effect:
    def __init__(self, name, start, finish, timeout, caption):
        self.name = name
        self.timeout = timeout
        self.start = start
        self.finish = finish
        self.caption = caption

def invisibility_effect(game):
    """other characters can't see the player"""
    def start():
        game.invisible_player = True

    def finish():
        game.score_redraw = True
        game.invisible_player = False

    return Effect("invisibility", start, finish, TIMEOUT * 2 // 3, "you are invisible!")



def mind_control_effect(game):
    """team1.decide, team2.decide =  team2.decide, team1.decide"""
    # no effect in trials, or replays.
    enemies = [t for t in game.other_teams if t.rules.sympathy < 0]
    player = game.player_sprite

    def start():
        if game.play_state == config.STATE_PLAYING:
            for t in enemies:
                for s in t.sprites:
                    s.manual_mode()
            #player.auto_mode()

    def finish():
        game.score_redraw = True
        if game.play_state == config.STATE_PLAYING:
            for t in enemies:
                for s in t.sprites:
                    s.auto_mode()
            #player.manual_mode()

    return Effect("mind control", start, finish, TIMEOUT * 2 // 3, "control your enemies actions (and vice versa)")



def gravity_effect(game):
    """everything falls to the bottom"""
    def start():
        if game.player_team.falling:
            #don't want to increas gravity beyond 1
            return
        for s in game.sprites:
            s.inexorable_force = (s.inexorable_force[0], s.inexorable_force[1] + 1)
        for t in game.teams:
            t.falling = True

    def finish():
        game.score_redraw = True
        for s in game.sprites:
            s.inexorable_force = (s.inexorable_force[0], s.inexorable_force[1] - 1)
        for t in game.teams:
            t.falling = False

    return Effect("gravity", start, finish, TIMEOUT, "everything is falling!")


def fast_player_effect(game):
    """Speed up player"""
    team = game.player_team
    #old_steps = team.steps
    #print old_steps

    def start():
        team.steps = [(x * 3 // 2, y * 3 // 2, i) for x, y, i in team.steps]

    def finish():
        game.score_redraw = True
        team.steps = team.original_steps

    return Effect("fast player", start, finish, TIMEOUT, "super fast")


def slow_enemy_effect(game):
    """Slow down unsympathetic teams"""
    teams = [t for t in game.other_teams if t.rules.sympathy < 0]
    #old_steps = [t.steps for t in teams]
    #print old_steps
    def start():
        for t in teams:
            t.steps = [(x * 2 // 3, y * 2 // 3, i) for x, y, i in t.steps]

    def finish():
        game.score_redraw = True
        for t in teams:
            t.steps = t.original_steps

    return Effect("slow enemies", start, finish, TIMEOUT, "your pursuers are slower")






#this is what is used externally

effects = [v for (k, v) in globals().items() if k.endswith('_effect')]
