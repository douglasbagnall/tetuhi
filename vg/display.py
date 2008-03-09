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
"""Do pretty much everything to do with I/O"""
import os, random
from math import atan2, pi

import Image
import pygame, yaml
from pygame import mixer, sprite

from vg import config
from vg.utils import id_generator
from vg.misc import GameEscape, GameQuit, CycleJump

pygame.init()
mixer.init(config.AUDIO_FREQ, config.AUDIO_BITS, config.AUDIO_CHANNELS)

from vg import config

def stretch(im, size, filter=Image.NEAREST):
    im.load()
    im = im._new(im.im.stretch(size, filter))
    return im

def echo_event(e):
    print "got event %s" % e
    for k in dir(e):
        print "%10s: %s" % (k, getattr(e, k))



class Flash:
    def __init__(self, window, message, pos, duration, colour=None):
        self.surface = window.text(message, colour)
        self.rect = self.surface.get_rect(center=pos)
        if self.rect.left <= 0:
            self.rect.left = 1

        self.duration = duration
        self._blit = window.screen.blit

    def tick(self):
        if self.duration:
            self.duration -= 1
        return self.duration

    def blank(self, background):
        self._blit(background, self.rect, self.rect)

    def blit(self):
        self._blit(self.surface, self.rect)



class Window:
    """make up a window"""
    capturing = False
    background = None
    wander_rect = [300, 300, 0, 0]
    wander_dir = [2, 0, 2]
    fire_button = False
    key_state = None
    player_blink = False
    score_rect = None
    flashes = set()

    def __init__(self, size, capture=None):
        self.size = size
        flags = 0# | pygame.HWSURFACE #| pygame.DOUBLEBUF
        if config.FULLSCREEN:
            flags |= pygame.FULLSCREEN
            pygame.mouse.set_visible(False)
        self.screen = pygame.display.set_mode(size, flags)
        self.clock =  pygame.time.Clock()
        if capture is not None:
            self.capturing = True
            self.capturegen = id_generator(prefix = capture + '/___', suffix='.jpg', pattern='%s%06d%s')
        self.sounds = {}
        pygame.mixer.set_num_channels(config.AUDIO_MIX_CHANNELS)
        for i in range(config.AUDIO_MIX_CHANNELS):
            mixer.Channel(i).set_endevent()

        self.font_plain = pygame.font.Font('config/Essays1743.ttf', config.PLAIN_TEXT_SIZE)

        #stop silly events -- only COMMENTED ones get through
        pygame.event.set_blocked([
            #pygame.QUIT,
            pygame.ACTIVEEVENT,
            #pygame.KEYDOWN,
            #pygame.KEYUP,
            pygame.MOUSEMOTION,
            pygame.MOUSEBUTTONUP,
            pygame.MOUSEBUTTONDOWN,
            #pygame.JOYAXISMOTION,
            pygame.JOYBALLMOTION,
            pygame.JOYHATMOTION,
            pygame.JOYBUTTONUP,
            #pygame.JOYBUTTONDOWN,
            pygame.VIDEORESIZE,
            pygame.VIDEOEXPOSE,
            pygame.USEREVENT,
            ])
        #don't need repeating buttons
        pygame.key.set_repeat()

        js = pygame.joystick.get_count()
        print "JOYSTICK count is %s" % js
        if js:
            self.joystick = pygame.joystick.Joystick(0)
            self.joystick.init()

        self.keys_down = []
        self.word_rects = []
        self.set_palette([(0x55,0x55,0x55)])

    def clear_sounds(self):
        self.sounds.clear()

    def register_sounds(self, sounds):
        #set up the sounds for the new game
        for v in sounds.values():
            for x in v:
                p = os.path.join(config.SOUND_DIR, x)
                try:
                    self.sounds[x] = mixer.Sound(p)
                except OSError, e:
                    print e


    def display(self, im, clear=False, capture_n=1):
        if isinstance(im, str):
            im = Image.open(im)
        if im.size != self.size:
            print "WARNING: resizing from %s to %s" % (im.size, self.size)
            im = stretch(im, self.size)
        if clear:
            self.screen.fill((0,0,0))

        im2 = pygame.image.frombuffer(im.tostring(), im.size, im.mode).convert_alpha()
        self.background = im2
        self.screen.blit(im2, (0,0))
        pygame.display.flip()
        if self.capturing:
            self.capture(capture_n)

    def display_colour(self, colour, capture_n=1):
        self.screen.fill(colour)
        self.background = self.screen.copy()
        pygame.display.flip()
        if self.capturing:
            self.capture(capture_n)

    def capture(self, n=1):
        """save the screen state to an image, for the purpose of making screenshot movies"""
        s = pygame.image.tostring(self.screen, 'RGB')
        im = Image.fromstring('RGB', self.size, s)
        for x in range(n):
            im.save(self.capturegen.next())

    def wait(self):
        while True:
            event = pygame.event.wait()
            if event.type == pygame.QUIT and config.CAN_QUIT:
                break


    def close(self):
        self.screen.quit()

    def update(self):
        pygame.display.flip()

    def blank_screen(self, game):
        if not self.background:
            self.display(game.background)
        else:
            self.screen.blit(self.background, (0,0))


    def redraw(self, game):
        """For each sprite, clear the background, and redraw"""
        blit = self.screen.blit
        bgd = self.background
        DEAD = config.DEAD
        #XXX could keep track of find unchanging ones
        # (by s.oldrect == s.rect and s.image unchanged?)
        # also, on some games, it will be better to clear the whole thing.
        #could possiby track occluded rectangles.
        for f in tuple(self.flashes):
            f.blank(bgd)
            if not f.tick():
                self.flashes.remove(f)

        for r in game.dead_rects:
            blit(bgd, r, r)
        for s in game.sprites:
            if s.state != DEAD:
                blit(bgd, s.oldrect, s.oldrect)
        for s in game.stacked_sprites:
            if s.state != DEAD:
                s.oldrect = blit(s.image, s.rect)
        #blit the player last, on top.
        s = game.player_sprite
        if s.state != DEAD:
            if game.invisible_player:
                self.player_blink = not self.player_blink
                if self.player_blink:
                    s.oldrect = blit(s.image, s.rect)
            else:
                s.oldrect = blit(s.image, s.rect)

        for f in self.flashes:
            f.blit()

        if game.play_state == config.STATE_PLAYING:
            self.update_scores(game)

        if self.capturing:
            self.capture()
        self.update()

    def update_scores(self, game):
        if game.score_redraw:
            if self.score_rect:
                print self.score_rect
                self.screen.blit(self.background, self.score_rect, self.score_rect)
            hs = magic = ""
            if game.high_score:
                hs = "high: %s" % game.high_score
            if game.effects:
                magic = " ".join(game.effects.keys())
            s = self.text("score: %s %s %s" % (game.player_score, hs, magic))
            self.score_rect = self.screen.blit(s, config.SCORE_POS)
            game.score_redraw = False

    def hide(self, actor):
        group = sprite.Group(actor)
        group.clear(self.screen, self.background)

    def speak(self, sounds):
        """play a set of sounds"""
        for x in sounds:
            if x in self.sounds:
                #self.sounds[x].set_volume(random.uniform(0.7,1))
                self.sounds[x].play()

    def unwrite(self):
        blit = self.screen.blit
        for r in self.word_rects:
            blit(self.background, r, r)
        self.word_rects = []

    def write(self, message, pos='centre', colour=None):
        """write the message in the default font"""
        s = self.text(message, colour)
        w, h = s.get_size()
        max_x, max_y = config.WORKING_SIZE
        if pos == 'centre':
            pos = (max(2, (max_x - w) // 2),
                   max(2, (max_y - h) // 2))
        r = self.screen.blit(s, pos)
        self.word_rects.append(r)
        pygame.display.flip()
        return pos, colour


    def write_lines(self, message, pos='centre', colour=None):
        self.unwrite()
        if isinstance(message, str):
            message = message.split('\n')

        if pos == 'centre':
            max_x, max_y = config.WORKING_SIZE
            w, h = (0, 0)
            for line in message:
                _w, _h = self.font_plain.size(line)
                w = max(w, _w)
                h += config.PLAIN_LINE_HEIGHT # _h
            pos = (max(2, (max_x - w) // 2),
                   max(2, (max_y - h) // 2))

        for line in message:
            (x, y), colour = self.write(line, pos, colour)
            pos = (x, y + config.PLAIN_LINE_HEIGHT)

    def notice(self, message):
        """a warning or exception message, not in palette, which may not yet exist"""
        self.write(message, colour=(0xcc,0,0,0))

    def flash(self, message, pos, duration):
        self.flashes.add(Flash(self, message, pos, duration))



    def set_palette(self, palette):
        self.palette = palette


    def intro_team(self, team):
        """Try to write the team into in an appropriate place."""
        #XXX better to blit the sprites in a better place.
        self.screen.blit(self.background, (0,0))
        message = team.intro()
        w, h = self.font_plain.size(message)
        max_x, max_y = config.WORKING_SIZE
        left, top = max_x, max_y
        right, bottom = 0, 0
        for s in team.sprites:
            self.screen.blit(s.image, s.rect)
            left = min(left, s.rect[0])
            top  = min(top, s.rect[1])
            right = max(right, s.rect[2])
            bottom = max(bottom, s.rect[3])

        margin = config.PAGE_MARGIN
        cx = max((left + right - w) // 2, margin)
        cy = max((top + bottom - h) // 2, margin)

        if top > h + margin:
            x, y = (min(left, max_x - (w + margin)), top - h)
        elif bottom < config.WORKING_SIZE[1] - (h + margin):
            x, y = (min(left, max_x - (w + margin)), bottom)
        elif left > w + margin:
            x, y = (left - w, cy)
        elif right < config.WORKING_SIZE[0] - (w + margin):
            x, y = (right, cy)
        else:
            x, y = cx, cy

        self.write(message, pos=(x, y))



    def interpret_events(self, extra_key_handler=echo_event):
        """works out what keys are down and which has
        priority. Returns the meaning of that key.  The state is
        remembered."""
        #self.keys_down is a kind of stack
        #add() needs some trickery in case it somehow gets too big.
        if extra_key_handler is None:
            extra_key_handler = echo_event

        def add(s):
            self.keys_down.append(s)
            if len(self.keys_down) > 4:
                self.keys_down = self.keys_down[-4:]

        remove = self.keys_down.remove

        #fire button is always false except for the moment it is pressed
        self.fire_button = False

        for event in pygame.event.get():
            #print event
            if event.type == pygame.QUIT and config.CAN_QUIT:
                raise GameQuit("user pressed X in top corner")
            elif event.type in (pygame.KEYDOWN, pygame.KEYUP):
                states = {
                    273 :  0,
                    275 :  2,
                    274 :  4,
                    276 :  6,
                    0:     None
                    }
                if event.type == pygame.KEYDOWN:
                    if event.key in states:
                        add(states.get(event.key))
                    elif event.key in (303, 32):
                        self.fire_button = True
                    elif event.key in (113, 27) and config.CAN_QUIT:
                        raise GameQuit("user pressed Q or escape")
                    else:
                        #print event.key
                        extra_key_handler(event)

                elif event.type == pygame.KEYUP:
                    s = states.get(event.key)
                    if s in self.keys_down:
                        remove(s)

            elif event.type == pygame.JOYAXISMOTION:
                v = event.value
                if v:
                    s = ((event.axis == 0) * 6 + (v > 0) * 4) % 8
                    add(s)
                else:
                    s1 = (event.axis == 0) * 2
                    s2 = s1 + 4
                    if s1 in self.keys_down:
                        remove(s1)
                    if s2 in self.keys_down:
                        remove(s2)
            elif event.type == pygame.JOYBUTTONDOWN:
                print 'bang!'
                self.fire_button = True

        if self.keys_down:
            self.key_state = self.keys_down[-1]
        else:
            self.key_state = None

        return (self.key_state, self.fire_button)




    def wander(self, sprite):
        """do pointless motion to pass time for audience"""
        BOUNDARY = 20
        blit = self.screen.blit
        bgd = self.background
        x, y, _w, _h = self.wander_rect
        if _w == 0 and _h == 0:
            x, y = sprite.left, sprite.top

        dx, dy, orientation = self.wander_dir
        blit(bgd, self.wander_rect, self.wander_rect)
        im = sprite.images[orientation].picture
        w, h = im.get_size()

        if x + w > self.size[0] - BOUNDARY or x < BOUNDARY:
            dx = - dx
        elif y + h >  self.size[1] - BOUNDARY or y  < BOUNDARY:
            dy = -dy
        dy += random.randrange(-1, 2)
        dx += random.randrange(-1, 2)
        TOP_SPEED = 5
        if abs(dx) > TOP_SPEED:
            dx = dx * TOP_SPEED / (TOP_SPEED + 1)
        if abs(dy) > TOP_SPEED:
            dy = dy * TOP_SPEED / (TOP_SPEED + 1)

        orientation = (orientation + 1) % 8 #7 - int((atan2(dx, dy) + pi) * 4 / pi + 2) % 8


        self.wander_dir = (dx, dy, orientation)
        #print self.wander_dir
        self.wander_rect = [x + dx, y + dy, w, h]
        blit(im, self.wander_rect)
        self.update()



    def twiddle(self, wait, game):
        """move the player sprite's images around on the screen
        --without affecting the actual sprite"""
        s = game.player_sprite
        def wander(i):
            self.wander(s)
        self.alert_pause(wait, decoration=wander)




    def alert_pause(self, wait, decoration=None, fire_handler=None,
                   key_handler=None, extra_handler=None):
        """a pause during which events are interpreted, and during
        which action can continue."""
        for i in range(int(wait * config.PLAY_HERTZ)):
            self.clock.tick(config.PLAY_HERTZ)
            if decoration is not None:
                decoration(i)
            if not i % config.FRAMES_PER_UPDATE:
                key, fire = self.interpret_events(extra_handler)
                if fire and fire_handler:
                    fire_handler()
                if key and key_handler:
                    key_handler(key)


    def message_screen(self, x, **kwargs):
        """This vfunction is called used by text_cycle and
        presentation. x is a dictionary describing wha to show ---
        usually read from a yaml file"""
        #print x
        im = x.get('image')
        text = x.get('text')
        delay = x.get('delay', 3)
        directory = kwargs.pop('directory', '')
        if im:
            try:
                if directory:
                    im = os.path.join(directory, im)
                self.display(im)
            except IOError,e:
                print e
        if text:
            self.write_lines(text)

        self.alert_pause(delay, **kwargs)


    def text_cycle(self, filename, **kwargs):
        """play the text in the file"""
        f = open(filename)
        for x in yaml.load(f):
            self.message_screen(x, **kwargs)

    def presentation(self, filename, player=None, player_args=()):
        """play the text in the file as a presentation. That just
        means the fire button advances play."""
        f = open(filename)
        p = [x for x in yaml.load(f)]
        def fire_handler(*args):
            raise CycleJump(1)
        def key_handler(key):
            raise CycleJump([-5, 0, #up
                             1, 0,  #right
                             5, 0,  #down
                             -1, 0 #left
                             ][key])

        def extra_handler(event):
            print event, event.key
            if event.key == 112 and player:
                try:
                    player(*player_args)
                except GameQuit:
                    raise CycleJump(0)
            else:
                raise CycleJump(1)

        i = 0
        d = os.path.dirname(filename)
        while i < len(p):
            x = p[i]
            try:
                self.message_screen(x, fire_handler=fire_handler, key_handler=key_handler,
                                    extra_handler=extra_handler, directory=d)
                i += 1
            except CycleJump, e:
                i += e.jump
                i = min(i, len(p) - 1)
                i = max(i, 0)


    def text(self, message, colour=None):
        """Render the text in the tetuhi game style, and return the surface"""
        if colour is None:
            colour = random.choice(self.palette)
        shadow = tuple(x//2 for x in colour)
        message = str(message)
        front = self.font_plain.render(message, True, colour)
        back = self.font_plain.render(message, True, shadow)
        r = front.get_rect()
        s = pygame.Surface((r[2] + 1, r[3] + 1), pygame.SRCALPHA, front)
        s.blit(back, (1, 1))
        s.blit(front, (0,0))
        return s
