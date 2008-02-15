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
"""Exceptions, and other things thast want to be everywhere"""

import config
import os

class GameEscape(RuntimeError):
    """jump to the next step"""

class GameQuit(RuntimeError):
    """get out"""

class GameOver(Exception):
    pass

class ActorDeath(Exception):
    pass

class SpriteError(Exception):
    pass

class ParseError(Exception):
    pass

class AttractEscape(Exception):
    pass

class GameReplay(Exception):
    pass

class GameStop(Exception):
    pass

class CycleJump(Exception):
    def __init__(self, jump):
        self.jump = jump

