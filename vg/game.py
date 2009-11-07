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
"""take a background and blobs and invent a game"""

import cPickle, os, sys
import time
import random
import traceback

from semanticCore import Gamemap

from vg import config
from vg import assort
from vg import utils
from vg import misc
from vg import actor, weights
from vg import rules
from vg import magic
from vg.misc import GameEscape, GameOver, SpriteError, GameStop, ParseError

from vg.utils import log

class Game:
    """Represents a game.  The game forks into two processes, and
    evolves in one (the child).  After a while, the evolved game saves
    key details to disk, and the game in the parent process picks them
    up.

    Saved attributes are:

    * sprite minds

    (This list should be growing, so is likely out of date).
    """
    game_over = True
    play_state = False
    nothing_happening = 0
    def __init__(self, background, blobs, window=None, entropy=None):
        if entropy:
            #seed the neural networks' RNG.
            self.random_holder = weights.seed_nets(entropy)

        self.background = background
        self.blobs = blobs
        self.map = Gamemap(config.WORKING_SIZE[0], config.WORKING_SIZE[1], config.PIXEL_SIZE)
        self.entropy = entropy
        if entropy:
            self.identity = "game-%x-%x" % tuple(entropy[:2])
        else:
            self.identity = "game-%x" % id(self)
        self.score = 0
        self.player_score = 0
        self.set_window(window)
        self.sort_sprites(blobs)
        self.effects = {}
        self.invisible_player = False
        self.high_score = 0
        self.score_redraw = False
        self.food_repopulation_constant = config.FOOD_REPOPULATION_CONSTANT
        self.enemy_repopulation_constant = config.ENEMY_REPOPULATION_CONSTANT
        self.ally_repopulation_constant = config.ALLY_REPOPULATION_CONSTANT
        self.resuscitate_in_packs = False

    def set_window(self, window):
        """separate from init for (legacy?) evolution reasons -- game
        can set window after it is formed"""
        self.window = window
        if window is not None:
            window.display(self.background)

    def sort_sprites(self, blobs):
        """Organise sprites into teams.  There will be at least one
        team of one, which becomes the player.
        """
        sorter = assort.TeamFinder(blobs, entropy=self.entropy)
        arrangement = sorter.find_teams()
        sorter.print_stats()

        self.teams = [actor.Team(v, i, self) for i, v in zip(config.TEAM_BITS, arrangement.vectors)]
        #print self.teams
        self.player_team = self.teams[0]
        self.other_teams = self.teams[1:]
        self.n_teams = len(self.teams) #needed for bootstrapping actors
        self.sprites = []
        #XXX possibly indeterminacy sneaking in, if sprites have ever been a set
        for t in self.teams:
            self.sprites.extend(t.sprites)
        self.nonplayer_sprites = self.sprites[1:]
        self.player_sprite = self.sprites[0]
        #so, now, there are teams of sprites, but no minds or rules.

    def mutate_rules_meta(self):
        #alter constant aspects of the game.
        for x in ('food_repopulation_constant',
                  'enemy_repopulation_constant',
                  'ally_repopulation_constant'):
            c = getattr(self, x)
            a, b = random.sample(range(30,40), 2)
            c = c * a // b
            setattr(self, x, c)
        if random.random() > 0.8:
            self.resuscitate_in_packs = not self.resuscitate_in_packs


    def _grow_rules(self):
        """get some rules, and tune them until they work"""
        stop = time.time() + config.RULE_GROWING_TIMEOUT
        results = []
        #start off with an arbitrary set
        #XXXXXX
        # add_rules can be moved here (only used here)
        #can be made invincible to suitability imbalance?

        rules.add_rules(self.player_team, self.other_teams)
        log("added rules")

        for i in range(config.RULE_GROWING_ATTEMPTS):
            log("attempt %s" % i)
            self.types = ['wall'] + [x.rules.name for x in self.teams]
            r, score = self.trial(config.CUTOFF_LENGTH + 2)
            score = score // 8
            log("trial with %s lasted %s" % (self.types, r))
            # diff from ideal matters, but overshooting is penalised
            if r > config.CUTOFF_LENGTH:
                error = config.OVERSHOOT_PENALTY
            else:
                error = abs(config.IDEAL_LENGTH - r)
            results.append((max(error - score, 0), self.types))
            if r > config.TARGET_LENGTH[1]:
                rules.mutate_rules(self.other_teams, -1)
            elif r < config.TARGET_LENGTH[0]:
                rules.mutate_rules(self.other_teams, 1)
            else:
                self.mutate_rules_meta()

            if time.time() > stop:
                log("stopping due to timeout")
                break


        log(*reversed(sorted(results)))
        return min(results)


    def grow_rules_in_fork(self):
        """make rules, in background processes"""
        #utils.make_dir(config.DATA_ROOT)

        def grow():
            try:
                log("hello")
                best = self._grow_rules()
                log("got rules")
                fn = os.path.join(config.DATA_ROOT, "rules-%s.pickle" % os.getpid())
                utils.pickle(best, fn)
                log("pickled rules")
            except Exception, e:
                traceback.print_exc()
                os._exit(os.EX_SOFTWARE)
            os._exit(0)

        def display():
            self.window.twiddle(0.5, self)

        finishers = utils.process_in_fork(grow, display, config.PROCESSES, config.PROCESS_TIMEOUT)
        results = []
        for pid in finishers:
            try:
                fn = os.path.join(config.DATA_ROOT, "rules-%s.pickle" % pid)
                results.append(utils.unpickle(fn))
                if config.TIDY_FILES:
                    os.remove(fn)
            except IOError, e:
                log("missing out on pid %s", e)
        if not results:
            # the game is crashing
            raise ParseError("Sorry. The artist was lazy and stupid, and the machine can't handle your picture.")

        error, self.types = min(results)
        if error > config.REALLY_BAD_GAME_ERROR:
            # the game is hardly lasting very long at all.
            # send out an apology, which has no actual effect
            self.window.text_cycle(config.GROVEL_CYCLE)

        rules.apply_rules(self.teams, self.types[1:])

        log("%s teams:" % len(self.teams))
        for t in self.teams:
            log(t.size, t, t.rules)


    def reset(self):
        """Start the game again"""
        self.map.clear()
        for x in self.sprites:
            x.reset()


    def trial(self, cutoff):
        """Play automatically, without display.  Return the number of
        rounds played (before GameOver raised).  If there is no
        GameOver before cutoff is reached, stop anyway."""
        log("trial with cutoff %s" % cutoff)
        self.finalise_rules()
        log("finalised")
        self.reset()
        log("reset")
        self.play_state = config.STATE_TRIAL
        self.player_score = 0
        PLAYING = config.PLAYING
        DYING = config.DYING
        log("trial with types %s" % self.types)
        self.game_over = False
        for r in range(cutoff):
            for s in self.sprites:
                if s.state == PLAYING:
                    s.decide()
            for s in self.sprites:
                if s.state == PLAYING:
                    s.update()
                elif s.state == DYING:
                    s.die2()
            self.track_play(r)
            if self.game_over:
                break
        return (r, self.player_score)

    def do_extra_keys(self, event):
        """any keys not handled as motion or firing must be handled here"""
        #XXX only getting pygame.KEYDOWN events, not KEYUP
        if event.key in (116, 115):
            self.player_sprite.toggle_mode()



    def end(self):
        self.game_over = True
        log("player is down!")

    def finalise_rules(self):
        self.food_sprites = []
        self.enemy_sprites = []
        self.ally_sprites = []
        for t in self.teams:
            log('about to finalise %s' %t)
            t.finalise()
            if t.rules.sprite_group == 'monster':
                self.enemy_sprites.extend(t.sprites)
            if t.rules.sprite_group == 'food':
                self.food_sprites.extend(t.sprites)
            if t.rules.sprite_group == 'ally':
                self.ally_sprites.extend(t.sprites)
        s = self.player_sprite
        self.player_extremes = (s.y, s.x, s.y, s.x)

    def finalise_display(self):
        """pick sounds for each team"""
        w = self.window
        if config.SOUND:
            w.clear_sounds()
            used_sound_groups = set()
            for t in self.teams:
                t.pick_sounds(used_sound_groups)
                w.register_sounds(t.sounds)

        self.stacked_sprites = []    #organise into stacking order for blitting
        #does not include player, which is blitted after, separately
        for o, x in sorted((t.rules.stacking_order, t) for t in self.other_teams):
            self.stacked_sprites.extend(x.sprites)


    def play(self, hz=config.PLAY_HERTZ, replay=False):
        self.reset()
        w = self.window
        w.blank_screen(self)
        self.dead_rects = []
        dying_rects = []
        if replay: # replay is flag and duration combined
            self.play_state = config.STATE_REPLAY
            self.player_sprite.auto_mode()
            replay *= hz
            for s in w.sounds.values():
                s.set_volume(0.3)
        else:
            self.play_state = config.STATE_PLAYING
            self.player_sprite.manual_mode()
            for s in w.sounds.values():
                s.set_volume(1)

        start = time.time()
        self.player_score = 0
        self.game_over = False
        r = 0
        frame = -1
        DYING = config.DYING
        PLAYING = config.PLAYING
        try:
            while True:
                w.clock.tick(hz)
                if not frame:
                    r += 1
                    w.interpret_events(self.do_extra_keys)
                    for s in self.sprites:
                        if s.state == PLAYING:
                            s.decide()
                    if config.DRAW_FINDS:
                        self.sprites[3].draw_finds()
                    #XXX reduce sounds a bit more.
                    sounds = set()
                    for s in self.sprites:
                        if s.state == PLAYING:
                            s.update()
                            sounds.add(s.chatter())
                        elif s.state == DYING:
                            s.die2()

                    w.speak(sounds)
                    self.track_play(r)

                    #XXX with replay, also want to stop for button press
                    if replay:
                        if r > replay or w.fire_button:
                            raise GameStop("replay is over")

                for s in self.sprites:
                    if s.state == PLAYING:
                        s.slither(frame)
                    elif s.state == DYING:
                        s.dying_animation()
                        dying_rects.append(s.rect)
                w.redraw(self)
                self.dead_rects = dying_rects
                dying_rects = []
                frame = (frame + 1) % config.FRAMES_PER_UPDATE
                if self.game_over and config.GAME_ENDS:
                    raise GameOver

        except Exception, e:
            self.playing = False
            t = time.time() - start
            log("%s ticks in %s seconds: ~ %s per sec" % (r, t, (r + 0.5)/t))
            self.save_map(r)
            if not isinstance(e, (GameEscape, GameOver, GameStop)):
                raise
            if isinstance(e, GameOver):
                w.write("G A M E  O V E R")
                if self.player_score > self.high_score:
                    self.high_score = self.player_score
                if not replay:
                    self.pause(3)


    def pause(self, duration=5):
        """Stop for a little bit """
        #self.window.clock.tick(0.5)
        def events(*args):
            raise GameStop()
        try:
            self.window.alert_pause(duration, fire_handler=events,
                                    key_handler=events, extra_handler=events)
        except GameStop:
            return

    def quit(self):
        self.save_map()
        raise GameEscape


    def save_map(self, round=0):
        if config.MODERATE_SAVES:
            import Image
            self.map.save_map('/tmp/map.pgm')
            im = Image.open('/tmp/map.pgm').convert('RGB')
            gb = range(256)
            r = [255] + gb[1:]
            im2 = im.point(r + gb + gb)
            im2.save('/tmp/map-%s.png' % round)


    def pickle_essentials(self, dirname=None):
        if dirname is None:
            dirname = self.identity
        d = make_dir(config.DATA_ROOT, dirname)

        fn = os.path.join(d, "game.pickle")
        utils.pickle([self.score], fn)
        #XXX broken, surely?
        for t in self.teams:
            t.pickle_essentials(d)

    def unpickle_essentials(self, dirname=None):
        if dirname is None:
            dirname = self.identity
            d = make_dir(config.DATA_ROOT, dirname)

        fn = os.path.join(d, "game.pickle")
        data = utils.unpickle(fn)
        self.score = data[0]
        for t in self.teams:
            t.unpickle_essentials(d)



    def evolve(self, fork=True):
        if fork:
            pid = utils.fork_safely()
            if pid:
                self.child = pid
                return pid

        # _evolve_to_move and _evolve_to_play removed in revision 240

        self._evolve_to_win()

        if fork:
            self.pickle_essentials()
            os._exit(0)



    def _evolve_to_win(self):
        """a bit primative really"""

        def enemy_evaluator(net):
            r, score = self.trial(config.CUTOFF_LENGTH)
            self.reset()
            print r
            return r

        def player_evaluator(net):
            r, score = self.trial(config.CUTOFF_LENGTH)
            self.reset()
            print r
            return config.CUTOFF_LENGTH - r

        for i in range(2):
            print "training enemies"
            print movers[1].net.generic_genetic(enemy_evaluator, 100, 8)
            print "training player"
            print movers[0].net.generic_genetic(player_evaluator, 100, 8)

        r, score = self.trial(config.CUTOFF_LENGTH + 2)
        self.score = config.IDEAL_LENGTH - abs(r - config.IDEAL_LENGTH)


    def reap_evolution(self):
        self.unpickle_essentials()
        #and what else?


    def intro(self):
        #XXX move to display?
        """after the game is set up, print a screen or two explaining how to play"""
        w = self.window
        w.display(self.background)

        for t in self.teams:
            w.intro_team(t)
            self.pause(5)

        w.display(self.background)


    def apply_magic(self, effect):
        e = effect(self)
        print "doing magic: %s" % e.name
        self.score_redraw = True
        self.effects[e.name] = e
        e.start()
        if self.play_state == config.PLAYING:
            s = self.player_sprite
            self.window.flash(e.caption, (s.x, s.y),
                              config.MIN_MAGIC_FLASH + len(e.caption))

    def track_play(self, r):
        """timeout any magic, increase difficulty if necessary"""
        for k, v in self.effects.items():
            #print k, v, v.timeout
            v.timeout -= 1
            if v.timeout == 0:
                print "undoing magic: %s" % k
                v.finish()
                del self.effects[k]

        #track whether the player moves
        top, left, bottom, right = self.player_extremes
        x, y = self.player_sprite.x, self.player_sprite.y
        if x < left:
            left = x
        elif x > right:
            right = x
        if y < top:
            top = y
        elif y > bottom:
            bottom = y
        self.player_extremes = (top, left, bottom, right)

        if r > random.randrange(self.enemy_repopulation_constant):
            print "r %s > randrange(enemy constant) %s" %(r,  self.enemy_repopulation_constant)
            self.resuscitate(self.enemy_sprites)
        if r > random.randrange(self.food_repopulation_constant):
            print "r %s > randrange(food constant %s)" %(r,  self.food_repopulation_constant)
            self.resuscitate(self.food_sprites)
        if r > random.randrange(self.ally_repopulation_constant):
            self.resuscitate(self.ally_sprites)
        if r > config.GAME_WINDUP:
            self.hurry((r - config.GAME_WINDUP) // config.GAME_WINDUP_STEP)


    def resuscitate(self, sprite_list):
        #print "resuscutating from %s" % sprite_list
        candidates = [s for s in sprite_list if s.state == config.DEAD]
        resuscitated = False
        if candidates:
            for i in range(10):
                sprite = random.choice(candidates)
                if sprite.resuscitate(self.player_sprite) and not self.resuscitate_in_packs:
                    resuscitated = True
                    break
        if not resuscitated:
            self.nothing_happening += 1
            if self.nothing_happening > config.HURRY_THRESHOLD:
                self.hurry(self.nothing_happening - config.HURRY_THRESHOLD)
        else:
            self.nothing_happening = 0

    def hurry(self, n):
        if n >= len(config.HURRY_WORDS):
            self.game_over = True
        elif self.play_state == config.PLAYING:
            s = self.player_sprite
            msg = config.HURRY_WORDS[n]
            self.window.flash(msg, (s.x, s.y),
                              config.MIN_MAGIC_FLASH + len(msg))


    def addscore(self, p):
        self.player_score += p
        self.score_redraw = True
