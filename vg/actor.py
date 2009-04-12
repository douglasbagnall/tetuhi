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

"""Classes for sprite controllers."""

import pygame, os, random

from vg import config, utils, weights
from vg.misc import GameOver
from vg.rules import TYPE_MAP
from vg import magic
from vg.utils import log

from config import DEAD, PLAYING, DYING



class ImageInfo:
    """linking a displayed picture to its gamemap equivelent"""
    def __init__(self, im, silhouette, actor):
        if im.mode == 'RGBA':
            self.picture = pygame.image.frombuffer(im.tostring(), im.size, im.mode).convert_alpha()
        elif im.mode == 'RGB':
            self.picture = pygame.image.frombuffer(im.tostring(), im.size, im.mode).convert()
            self.picture.set_colorkey(config.CHROMA_KEY_COLOUR, pygame.RLEACCEL)

        self.silhouette = silhouette
        silhouette.load()

        self.offset = actor.game.map.add_element(silhouette.size[0],
                                                 silhouette.size[1],
                                                 actor.team.bit,
                                                 silhouette.tostring())

        #print "element %s is image at %s,%s" % (self.offset, actor.start_x, actor.start_y)
        self.rect = self.picture.get_rect()
        self.width, self.height = im.size


class InterstitialInfo:
    """For images that have no gamemap representation. (these are shown briefly)"""
    def __init__(self, im):
        if im.mode == 'RGBA':
            self.picture = pygame.image.frombuffer(im.tostring(), im.size, im.mode).convert_alpha()
        elif im.mode == 'RGB':
            self.picture = pygame.image.frombuffer(im.tostring(), im.size, im.mode).convert()
            self.picture.set_colorkey(config.CHROMA_KEY_COLOUR, pygame.RLEACCEL)

        self.rect = self.picture.get_rect()
        self.width, self.height = im.size


class Actor(pygame.sprite.Sprite):
    ##XXX need  configuration for things like:
    # mind shape
    # scan shape
    # need to set up a few possibilities (2, 3?) in each independant axis, and let them multiply.
    # take out hardcoded 16s, etc
    oldrect = (0,0,0,0)
    scan_results = None
    state = PLAYING
    start_playing = True
    firing = False
    fired = False
    continuity = 0
    apparent_direction = 0
    turniness = 0.0
    feedback = [0] * config.FEEDBACK_NODES
    inexorable_force = (0,0)
    best = 0

    def __init__(self, s, team):
        pygame.sprite.Sprite.__init__(self)
        self.start_x = (s.pos[0] + s.width / 2) #maybe need to keep track of centre (of e.g. gravity)
        self.start_y = (s.pos[1] + s.height / 2)
        self.start_direction = 0 #XXXrandom.choice(team.directions)
        self.game = team.game
        self.team = team #NB reference cycle

        self.register(s)
        self.reset()

    def register(self, s):
        """put the image[s] in the game's image store."""
        self.images = []
        for i, x in enumerate(s.get_rotations(self.team.motion_style)):
            im, sil = x
            #if i not in self.team.directions:
            if i & 1:
                self.images.append(InterstitialInfo(im))
            else:
                self.images.append(ImageInfo(im, sil, self))
        im = s.dying_image()
        self.dying_image = InterstitialInfo(im)

    def reset(self, pos=None, orientation=0, playing=False):
        """Put the actor on the screen if playing is true (at pos
        or its default position) otherwise take it off and away."""
        self.orientation = orientation
        self.intention = (0, 0, self.orientation)
        if playing or self.team.start_playing:
            self.state = PLAYING
        else:
            self.state = DEAD
        if pos is None:
            pos = (self.start_x, self.start_y)
        self.x, self.y = pos
        im = self.images[orientation]
        self.image = im.picture

        hw = im.width // 2
        hh = im.height // 2
        if self.x < 0:
            self.x = hw
        elif self.x >= config.WORKING_SIZE[0]:
            self.x = config.WORKING_SIZE[0] - hw
        if self.y < hh:
            self.y = hh
        elif self.y >= config.WORKING_SIZE[1]:
            self.y = config.WORKING_SIZE[1] - hh
        if pos != (self.x, self.y):
            print "pos %s shifted to %s, %s in reset"


        self.rect = self.image.get_rect()
        self.rect.centerx = self.x
        self.rect.centery = self.y
        self.blocked = False
        self.stopped = False
        self.firing = False
        self.inexorable_force = (0,0)
        self.turn(orientation)

    def turn(self, direction):
        i = self.images[direction]
        self.image = i.picture
        self.bitmap = i.offset
        self.orientation = direction
        self.width = i.width
        self.height = i.height
        self.left = self.x - (self.width >> 1)
        self.top  = self.y - (self.height >> 1)


    def decide(self, prescan=True):
        """Decide what to do based on surrounds and other state.  The
        surrounds are scanned in zones arranged thus:

             \    |    /
              \15_|_8_/
              |\  |  /|
            14| \7|0/ |9
            __|6|   |1|___
              |5|___|2|
            13| /4|3\ |10
              |/__|__\|
              / 12|11 \

         ... continuing on spiralling out if there are more zones.

         Whatever is found is fed into the neural net, and an intention is
         devised.
        """
        if self.team.net is None:
            # no decision to be made
            return

        #shape of inputs:
        # team.inputs_per_team inputs each for bits 0 to whatever (at offsets 16 * bit ..  16 * bit + 15)
        # config.EXTRA_INPUTS + config.FEEDBACK_NODES miscellaneous inputs
        #
        #this order has to match that set in config
        inputs = self.game.map.scan_zones(self.rect, self.team.zones, "list", self.orientation, prescan) + \
                 [self.stopped, self.turniness, self.continuity, self.blocked] + self.feedback
        if self.team.game.invisible_player:
            #hack to make player invisible to other teams
            n = self.team.inputs_per_type
            b = config.PLAYER_BIT * n
            inputs[b : b + n] = [0] * n

        direction, self.feedback, self.best, self.firing = self.team.navigate(inputs)

        if direction == 'stop':
            self.intention = (0,0, self.orientation)
            self.stopped = True
            self.turniness = max(self.turniness - 0.1, -4)
        elif direction == 0:
            self.turniness = max(self.turniness - 0.05, -4)
            self.intention = self.team.steps[self.orientation]
        else: #turning
            self.turniness = min(self.turniness + 0.3, 4)
            self.intention = self.team.steps[(self.orientation + direction) % 8]

        #XXX is this actually any good?
        self.continuity = (direction == 0)

        #for drawing during debug
        self.scan_results = inputs



    def update(self, group=None):
        """Actually move the thing, if it is possible (ie, not
        blocked).  The actor's intentions should have been decided
        earlier (by the decide method) and stored in self.intention.
        """
        #XXX this is not clearing outside the map, but some scans go outside.
        self.game.map.clear_figure(self.bitmap, self.left, self.top)
        #pre-test for dying
        #XXX is this necessary? -- really double buffering is the trick
        #could also have short circuit answer (eg use lowres)
        #hit = self.game.map.touching_figure(self.bitmap, self.left, self.top)
        #if hit & self.team.die_bits:
        #    self.die()

        dx, dy, direction = self.intention

        dx += self.inexorable_force[0]
        dy += self.inexorable_force[1]

        old_direction = None
        if direction != self.orientation:
            old_direction = direction
            self.turn(direction)

        #so now, the shape is turned and moved.  See if it is touching anything.
        hit = self.game.map.touching_figure(self.bitmap, self.left + dx, self.top + dy)
        #if self.team.net is None or self.team.bit == 1:
        #    print "%s has hit %s. self.orientation %s, direction %s, dx %s, dy %s" %(self, hit, self.orientation, direction, dx, dy)

        if hit & self.team.die_bits:
            print "bit %s hit mask %s at %s. die bits are %s" % (self.team.bit, hit, (self.left + dx, self.top + dy), self.team.die_bits)
            if not hit & self.team.block_bits:
                #because it can have hit something else blocking
                self.left += dx
                self.top += dy
                self.x += dx
                self.y += dy
            if self.team.rules.magic:
                self.team.do_magic()
            self.die()

        elif hit & self.team.block_bits:
            #print "%s is blocked" %self
            self.blocked = True
            if old_direction is not None:
                #XXX not always right
                self.turn(old_direction)

        else:
            self.left += dx
            self.top += dy
            self.x += dx
            self.y += dy
            self.blocked = False

        if self.firing:
            self.team.attempt_fire(self)
            self.firing = False


        self.hit = hit # for chatter
        self.game.map.set_figure(self.bitmap, self.left, self.top)


    def chatter(self):
        return self.team.chatter(self)


    def slither(self, frame):
        """Make visually smooth motion -- this only needs to happen in
        actual play.  Depending on config.FRAMES_PER_UPDATE, only
        every nth frame might show the real state."""
        #XXX much of this is based on the assumptions that
        #config.FRAMES_PER_UPDATE == 2, and 8 directional images
        if self.apparent_direction != self.orientation:
            #need to spin in the right way
            diff = (8 + self.apparent_direction - self.orientation) % 8
            if diff in (3, 4, 5):
                # what? for now, an arbitrary spin.
                self.apparent_direction = (self.apparent_direction + 2) % 8
            elif diff in (1, 2):
                self.apparent_direction = (self.apparent_direction - 1) % 8
            else:
                self.apparent_direction = (self.apparent_direction + 1) % 8

        if frame + 1 < config.FRAMES_PER_UPDATE:
            # self.x, self.y is still ahead. shift there gradually.
            x = self.rect[0] + (self.rect[2] >> 1)
            y = self.rect[1] + (self.rect[3] >> 1)
            x += (self.x - x) / config.FRAMES_PER_UPDATE
            y += (self.y - y) / config.FRAMES_PER_UPDATE
        else:
            x, y = self.x, self.y

        i = self.images[self.apparent_direction]
        self.image = i.picture
        self.rect = (x - (i.width >> 1), y - (i.height >> 1), i.width, i.height)

    def dying_animation(self):
        i = self.dying_image
        self.image = i.picture
        self.rect = (self.x - (i.width >> 1), self.y - (i.height >> 1), i.width, i.height)

    def die(self):
        self.state = DYING
        self.game.addscore(self.team.score)
        print "sprite %s is dying" % self

    def die2(self):
        #make sure it is gone
        self.state = DEAD
        print "sprite %s is DEAD" % self
        self.oldrect = (0,0,0,0)
        self.game.map.clear_figure(self.bitmap, self.left, self.top)
        if self.team.bit == config.PLAYER_BIT:
            self.game.end()

        #self.visible = False

    def resuscitate(self, player):
        """try to come back in a random place"""
        if self.state != DEAD:
            return False
        if self.width > config.WORKING_SIZE[0] // 2 or self.height > config.WORKING_SIZE[1] // 2:
            #leave too big ones out of it.
            return False

        x = random.randrange(config.WORKING_SIZE[0] - self.width)
        y = random.randrange(config.WORKING_SIZE[1] - self.height)
        hit = self.game.map.touching_figure(self.bitmap, x, y)
        if hit:
            return False
        print "resuscitating a player at %s, %s or size %s,%s" % (x, y, self.width, self.height)
        x += self.width // 2
        y += self.height // 2
        #XXX not quite the right calculation
        if (abs(x - player.x) - abs(y - player.y) +
            (player.width + self.width + player.height + self.height) // 2 <
            config.MIN_RESUSCITATE_DISTANCE):
            return False
        self.dying_animation()
        self.reset(pos=(x, y), playing=True)
        return True


    old_p = []
    def draw_finds(self, sr=None):
        """draw lines indicating what weas found where"""
        #blank the previous set
        if sr is None:
            sr = self.scan_results

        for x in self.old_p:
            pygame.draw.polygon(self.game.window.screen, 0xffffff, x, 1)
        self.old_p = []
        def poly(points, i):
            i = (i & ~7) + ((i + 8 - self.orientation) & 7)
            self.old_p.append(points)
            r = 0xcc0000 * (not sr[i])
            g = 0x00cc00 * (not sr[pb + i])
            b = 0x0000cc * (not sr[2 * pb + i])
            colour = r + g + b
            pygame.draw.polygon(self.game.window.screen, colour, points, 1)
            self.old_p.append(points)

        pb = (len(self.team.zones) - 1) * 8 # how many finds per bit
        cx, cy = self.x, self.y
        left, top = self.left, self.top
        right, bottom = left + self.width, top + self.height
        i = 0
        for z1, z2 in zip(self.team.zones, self.team.zones[1:]):
            poly([(cx, top - z1),
                  (cx, top - z2),
                  (right + z2, top - z2),
                  (right + z1, top - z1)], i)
            i += 1
            poly([(right + z2, top - z2),
                  (right + z1, top - z1),
                  (right + z1, cy),
                  (right + z2, cy),
                  ], i)
            i += 1
            poly([(right + z1, cy),
                  (right + z2, cy),
                  (right + z2, bottom + z2),
                  (right + z1, bottom + z1),
                  ], i)
            i += 1
            poly([(right + z2, bottom + z2),
                  (right + z1, bottom + z1),
                  (cx, bottom + z1),
                  (cx, bottom + z2),
                  ], i)
            i += 1
            poly([(cx, bottom + z2),
                  (cx, bottom + z1),
                  (left - z1, bottom + z1),
                  (left - z2, bottom + z2),
                  ], i)
            i += 1
            poly([(left - z2, bottom + z2),
                  (left - z2, cy),
                  (left - z1, cy),
                  (left - z1, bottom + z1),
                  ], i)
            i += 1
            poly([(left - z2, top - z2),
                  (left - z2, cy),
                  (left - z1, cy),
                  (left - z1, top - z1),
                  ], i)
            i += 1
            poly([(cx, top - z1),
                  (cx, top - z2),
                  (left - z2, top - z2),
                  (left - z1, top - z1),
                  ], i)
            i += 1

    #for manual control of the player
    _auto_decide = decide
    def _manual_decide(self):
        """Decide what to do based on keyboard!"""
        k = self.game.window.key_state
        if k is None:
            self.intention = (0, 0, self.orientation)
        #XXX other options here too. (?)
        else:
            self.intention = self.team.steps[k]
        if self.game.window.fire_button:
            print "fire!"
            self.firing = True

    def _no_decide(self):
        pass

    def dumb_mode(self):
        self.decide = _no_decide

    def manual_mode(self):
        self.decide = self._manual_decide

    def auto_mode(self):
        self.decide = self._auto_decide

    def toggle_mode(self):
        print self.decide, self._manual_decide, self._auto_decide, self._manual_decide is self.decide
        if self.decide == self._manual_decide:
            self.decide = self._auto_decide
        else:
            self.decide = self._manual_decide

    def __str__(self):
        return "<actor: team %s, pos: (%s, %s), state: %s>" %\
               (self.team.bit, self.x, self.y, self.state)

    __repr__ = __str__


if config.SOUND:
    from vg.sounds import SoundManager
    sound_manager = SoundManager()

class Team:
    #from type rules
    excited_threshold = 0.9
    _mind_lut = {
        'complex'  :(16, config.ACTOR_ZONES),
        'simple'   :(8, config.DUMB_ACTOR_ZONES),
        'none'     :(0, []),
        'reactive' :(0, []),
        'player'   :(16, config.ACTOR_ZONES)
        }

    falling = False

    def chatter(self, sprite):
        #XXX don't repeat if another in the team is saying the same
        #XXX shift this out into
        if config.SOUND:
            excited = sprite.best > self.excited_threshold
            if excited: #do something trickier?
                #excitement comes in waves? (each team out of sync)
                #print "excited (bit %s): %s > %s" % (self.bit, best, self.excited_threshold)
                self.excited_threshold += 0.06
            else:
                self.excited_threshold -= 0.001
            if sprite.fired:
                return self.sounds['fire'][0]
                sprite.fired = False

            if sprite.state == DYING:
                return random.choice(self.sounds['misc'])
            if sprite.blocked:
                return self.sounds['blocked'][0]
            t = (8 + sprite.apparent_direction - sprite.orientation) % 8
            if excited:
                return self.sounds['excited'][t]
            if t:
                #special case: straight ahead is never pronounced, so no sound is there
                return self.sounds['turning'][t - 1]
            if sprite.hit: #but not dying
                for x in range(8):
                    if sprite.hit & (1 << x):
                        return self.sounds['hit'][x]

        return None



    inputs_per_type = 16
    rules = None
    rule_suitability = -1e300
    fires = False
    fireables = []

    def __init__(self, vectors, bit, game):
        self.bit = bit
        self.game = game
        self.blobs = [v.blob for v in vectors]
        self.size = len(vectors)

        #perhaps put these detail things in a sub-object
        self.monster_rating = sum(v.monster_rating for v in vectors) / len(vectors) #want to penalise outliers?
        self.max_size = max(s.convex_perimeter for s in self.blobs) #or RMS?
        self.min_size = min(s.convex_perimeter for s in self.blobs)
        self.mean_aspect = sum(s.aspect for s in self.blobs) / self.size
        #XXX need more tricky stuff here.

        self.tiny = self.max_size < config.TINY_SPRITE_SIZE
        self.big = self.max_size > config.BIG_SPRITE_SIZE
        self.huge = self.max_size > config.HUGE_SPRITE_SIZE
        self.monsterish = self.min_size > config.TINY_SPRITE_SIZE and not self.huge
        print "aspect is %s" % self.mean_aspect

        if self.huge:
            self.motion_style = 'static'
        elif self.mean_aspect < 0.8:
            self.motion_style = 'mirror'
        elif self.big or  self.mean_aspect > 1.4:
            self.motion_style = 'lean'
        else:
            self.motion_style = 'directional'

        #self.name = self.new_team_name()
        self.start_playing = True

        self.sprites = [ Actor(x, self) for x in self.blobs ]

        self.suitabilities = dict((k, v.suitability(self)) for k, v in TYPE_MAP.iteritems())


    def apply_rules(self, rules):
        self.rules = rules
        self.rule_suitability = self.suitabilities[rules.name]

        self.steps = self.rules.steps
        self.original_steps = self.steps
        self.directions = self.rules.directions
        self.type_name = rules.name
        #XXX more, surely?

        self.inputs_per_type, self.zones = self._mind_lut[self.rules.mind]

        self.block_bits = 0xff
        self.die_bits = 0
        log("applied rules to %s" % self)

    def grow_mind(self):
        """turn the rules' aims into a mind."""
        r = self.rules
        print "growing mind"
        #work out whether this thing can fire bullets.
        # it has to be a) meant to and b) have bullets available
        fire_at = []

        #bit_pattern has (intention, attention)
        # attention determines how powerful the aim is
        try:
            bit_pattern = [ (r.aims.get(x), r.attention.get(x, 1))  for x in self.game.types ]
        except AttributeError:
            print r, r.__dict__

        self.net = weights.get_net(self)
        print "setting weights"

        weights.get_weights(self.net, bit_pattern, self.rules.character, self.inputs_per_type, fire_at)
        print "leaving grow_mind"


    def finalise(self):
        """make the actual network that the actors will use"""

        #this has to happen after all the rules are set
        self.block_bits = 0xff
        self.die_bits = 0
        #print self.game.types[self.bit]
        unblocks = self.rules.rules.get('unblock', ())
        dies = self.rules.rules.get('die', ())
        fires = self.rules.rules.get('fires', ())
        fired = self.rules.bullet
        print "rules.bullet is %s" % fired

        for i, x in enumerate(self.game.types):
            print "dealing with type %s (%s) out of %s" % (i, x, self.game.types)
            if x in unblocks:
                self.block_bits &= ~(2 ** i)
            if x in dies:
                self.die_bits |= 2 ** i
                self.block_bits &= ~(2 ** i) #can't  be blocked by your killers
            if x in fires:
                print x, i, self.game.teams
                print "type %s is firing %s (team %s)" %(self, x, self.game.teams[i - 1])
                self.fireables = self.game.teams[i - 1].sprites
                for f in self.fireables:
                    f.state = DEAD
                    f.team.start_playing = False
                self.fires = True
                self.firing_threshold = 0.66
        print "about to grow mind"
        if self.inputs_per_type:
            self.grow_mind()
        else:
            self.net = None
        print "about to reset sprites"
        for a in self.sprites:
            a.reset()
        print "about to do magic"
        if self.rules.magic:
            self.magic_effect = random.choice(magic.effects)
        self.score = self.rules.score
        print "leaving finalise"

    def do_magic(self):
        self.game.apply_magic(self.magic_effect)


    def pick_sounds(self, exclude):
        """attempt to pick a sound set that is in a different category
        than any other teams'"""
        if config.SOUND:
            self.sounds = sound_manager.pick_sounds(self.rules.sound_group, exclude)

    def mutate(self, degree):
        shape = self.net.shape
        mutations = int(shape[0] * degree)
        self.net.random_mutations(mutations)

    def pickle_essentials(self, directory):
        """Save the mind"""#XXX and rules
        fn = os.path.join(directory, "%s.mind.pickle" % self.bit)
        utils.pickle(self.net, fn)

    def unpickle_essentials(self, directory):
        """replace mind"""
        fn = os.path.join(directory, "%s.mind.pickle" % self.bit)
        self.net = utils.unpickle(fn)


    def intro(self):
        if len(self.sprites) == 1:
            s = self.rules.intro_single
        else:
            s = self.rules.intro_plural
        return s % self.__dict__


    def navigate(self, inputs):
        """navigate for the actors"""
        answer = self.net.opinion(inputs)

        best = -1e300
        for x, d in zip(answer, self.directions):
            if x > best:
                best = x
                direction = d

        while direction == 'random':
            direction = random.choice(self.directions)

        feedback = answer[-config.FEEDBACK_NODES:]
        if self.fires:
            fire = answer[len(self.directions)] > self.firing_threshold
            self.firing_threshold += (fire * 0.01 - 0.005)
            #print answer[len(self.directions)]
        else:
            fire = False
        return direction, feedback, best, fire


    fire_directions = [(0,-1), (1,-1), (1,0), (1,1), (0,1), (-1,1), (-1,0), (-1,-1),]
    def attempt_fire(self, sprite):
        for s in self.fireables:
            print "looking to fire with %s" % s
            if s.state == DEAD:
                dx, dy = self.fire_directions[sprite.orientation]
                ox = dx * (sprite.width + s.width + 2) // 2
                oy = dy * (sprite.height + s.height + 2) // 2
                x, y = sprite.x + ox, sprite.y + oy
                print "x,y are %s, %s" % (x, y)
                if x < 0 or y < 0 or x > config.WORKING_SIZE[0] or y > config.WORKING_SIZE[1]:
                    print "can't fire outide walls. averting terrible segfault"
                    return
                s.reset(pos=(x,y), orientation=sprite.orientation, playing=True)
                s.inexorable_force = (config.BULLET_SPEED * dx, config.BULLET_SPEED * dy)
                if self.falling:
                    s.inexorable_force = (s.inexorable_force[0],
                                          s.inexorable_force[1] + 1)
                print "fired a missile"
                sprite.fired = True #chatter
                return

        print "didn't fire, nothing to shoot with"
