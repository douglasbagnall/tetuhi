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
"""configuration variables """
#XXX should be moved to/complemented by a config file

from os.path import expanduser as _expanduser

DETERMINISTIC = True
GAME_ENDS = True
PROLIFIC_SAVES = False
MODERATE_SAVES = False
TIDY_FILES = False  #delete pickles once they have been gathered.

FULLSCREEN = False
CAN_QUIT = True

ENEMY_FIRES = True

#SOUND_DIR = './audio'
#SOUND_YAML = './audio/sound.yaml'
SOUND = False
SOUND_DIR = None
SOUND_YAML = None


USE_CHROMA_KEY = True
RANDOMISE_WEIGHTS = True

FRIENDLY_ONLY = False
NASTY_ONLY = False

HURRY_THRESHOLD = 10
#HURRY_CONTINUATION = 2
HURRY_WORDS = ["hurry up", "do something", "boring...", "not such a good game, after all",
               "if nothing happens soon...", "the game will end inexplicably", "too late"]
GAME_WINDUP = 3000
GAME_WINDUP_STEP = 150

#use black because that is what ends up filling out rotations
CHROMA_KEY_COLOUR = (0,0,0)

FOOD_REPOPULATION_CONSTANT = 120000
ENEMY_REPOPULATION_CONSTANT = 100000
ALLY_REPOPULATION_CONSTANT = 250000

MIN_RESUSCITATE_DISTANCE = 100

ALLOW_ERRORS_TO_KILL_X = False

PLAY_HERTZ = 25
PAUSE_HERTZ = 25
FRAMES_PER_UPDATE = 2
MIN_MAGIC_FLASH = 25

SCORE_POS = (5,-2)

MIN_SPRITES = 3
MAX_SPRITES = 150
IDEAL_SPRITES = (8, 24)

SUBSUME_THRESHOLD_INIT = 16000
SUBSUME_THRESHOLD_MAX = SUBSUME_THRESHOLD_INIT * 5
SUBSUME_THRESHOLD_MIN =  SUBSUME_THRESHOLD_INIT // 4

#ENTROPY_BITS should be > 8 * PROCESSES
ENTROPY_BITS = 640
PROCESSES = 4
PROCESS_TIMEOUT = 30
CAMERA_TIMEOUT = 15

DATA_ROOT = './data'
LOG_FORMAT = './log/%s.log'
CHILD_LOG = './log/child.log'
CONFIG_ROOT = './config'
RULES_FILE = './config/rules.yaml'
SCHEMA_FILE = './config/schema.yaml'
INSTRUCTIONS_CYCLE = './config/instructions.yaml'
CREDITS_CYCLE = './config/credits.yaml'
CAMERA_CYCLE = './config/camera.yaml'
COUNTDOWN_CYCLE = './config/countdown.yaml'
GROVEL_CYCLE = './config/grovel.yaml'
ATTRACTOR_BACKGROUND = './media/images/hills-cars.jpg'
PHOTO_DIRECTORY = './photos/'

CAPTURE_SCRIPT = 'scripts/capture.sh'
FAKE_CAPTURE_SCRIPT = 'scripts/fake-capture.sh'
CAMERA_SCRIPT = CAPTURE_SCRIPT
#CAMERA_SCRIPT = FAKE_CAPTURE_SCRIPT


TEST_IMAGE = _expanduser('~/images/tetuhi-examples/cowboy-tanks.jpg')


MIN_OBVIOUSNESS_ELEMENT = 100
MIN_OBVIOUSNESS_ELEMENT_SUM = 200
MIN_OBVIOUSNESS_ELEMENT_GLOBAL = 20
MIN_OBVIOUSNESS_BLOB = 150
MIN_OBVIOUSNESS_BLOB_SUM = 400
MIN_OBVIOUSNESS_BLOB_GLOBAL = 20

#for evolving games
IDEAL_LENGTH = 550
TARGET_LENGTH = (400, 700)
CUTOFF_LENGTH = 800

RULE_GROWING_ATTEMPTS = 30
RULE_GROWING_TIMEOUT = 6
RULE_GROWING_PARENT_TIMEOUT = 8

#overshooting is as bad as dying in 100
OVERSHOOT_PENALTY = IDEAL_LENGTH - 100
REALLY_BAD_GAME_ERROR = 440

LAYER2_NODES = 8
EXTRA_INPUTS = 4
FEEDBACK_NODES = 1
OUTPUT_NODES = 6

EXNODE_STOPPED, EXNODE_TURNY, EXNODE_CONTINUITY, EXNODE_BLOCKED = range(EXTRA_INPUTS)



WINDOW_SIZE =   (768,576)
WORKING_SIZE =  (768,576)

PIXEL_SIZE = 4
ACTOR_ZONES = [4, 50, 250]
DUMB_ACTOR_ZONES = [4, 80]

#convex hull perimeters
TINY_SPRITE_SIZE = WINDOW_SIZE[0] // 32
IDEAL_SPRITE_SIZE = WINDOW_SIZE[0] // 3
TINY_VOLUME = TINY_SPRITE_SIZE ** 2 // 4


SEMANTIC_SIZE = tuple(x / PIXEL_SIZE for x in WORKING_SIZE)
HUGE_SPRITE_SIZE = sum(WORKING_SIZE)
TOO_BIG_TO_WOBBLE = sum(WORKING_SIZE)

BIG_SPRITE_SIZE = HUGE_SPRITE_SIZE // 3

#ess than this far from the edge counts against an object being player
EDGE_H = WORKING_SIZE[0] // 6
EDGE_V = WORKING_SIZE[1] // 6

PLAIN_TEXT_SIZE = 24
PLAIN_LINE_HEIGHT = int(PLAIN_TEXT_SIZE * 1.2)

#wandering things don't go past here
PAGE_MARGIN = 10

#play states
STATE_TRIAL = 0
STATE_REPLAY = 1
STATE_PLAYING = 2


HISTOGRAM_BUCKETS = 32
DISPLAY = True
DISPLAY_EVERYTHING = False

#12 is a good threshold for the Unicorns, 25 for sheep picture, 16 for others.
#threshold is *squared*
EXPAND_THRESHOLD_2 = 12
EXPAND_INIT_THRESHOLD_2 = 16

PALETTE_DISTINCTION = 1600

WALL_BIT  = 0
PLAYER_BIT  = 1
TEAM_BITS = range(1, 8)
TEAMS_MAX = 7
TEAMS_MIN = 3
TEAM_FINDER_CANDIDATES = 10

#default is 22.05k, 16bit, stereo
#set to 44.1k mono
AUDIO_FREQ = 44100
AUDIO_CHANNELS = 1
AUDIO_BITS = 16
AUDIO_MIX_CHANNELS = 16

USE_PLAYER = True
DRAW_FINDS = False

BLANKS_PER_CYCLE = 4

DEAD = 0
DYING = 1
PLAYING = 2

LIFE_STATES = (DEAD, DYING, PLAYING)

BULLET_SPEED = 3
