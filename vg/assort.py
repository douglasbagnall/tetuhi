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
"""Attempting to find meaningful team clusters.

The first version uses successive k-means clustering, with k
increasing until a cluster of 1 is found.

Perhaps it would be better to find and isolate the 1 first, then find
clusters in the rest.  The player should stand out.

Perhaps:  reject too big or too small (initially).
sort by sum of distances (greater is better).


"""
from vg import config

from vg.misc import SpriteError
from math import log
import random, time

class BlobVector(list):
    """Feed it a blob, and it will build a vector or numbers
    representing the blob in an feature space.  The vector will be
    stored in self, and the blob in .blob.

    Ideally the vector numbers should have a range in the order of 200."""

    def evaluate(self, blob):
        """populate the list (ie, self)"""
        self.blob = blob

        r, g, b = blob.colour
        y =        0.299    * r + 0.587    * g + 0.114    * b
        cb = 128 - 0.168736 * r - 0.331264 * g + 0.5      * b
        cr = 128 + 0.5      * r - 0.418688 * g - 0.081312 * b
        w, h = blob.width, blob.height
        size = blob.convex_area ** 0.37
        self.extend((#blob.tiny * 40 - blob.huge * 20,
                     3 * size,  # approximate size
                     200 * blob.volume / blob.convex_area,        #how much foreground
                     30 * blob.perimeter / (blob.volume ** 0.7),  #how much foreground/background transition
                     40 * blob.aspect, #aspect ratio
                     y * 14,
                     cb * 20,
                     cr * 20,
                     blob.obviousness * 3,
                     blob.solidity * 2,                          #changes in colour (edges, including external)
                     40 * blob.perimeter / blob.convex_perimeter,
                     #80 * blob.leftheavy / blob.volume,
                     #80 * blob.topheavy / blob.volume,
                    ))


    def calculate_adjustment(self, total_mean):
        """Work out how bad this blob would be as the player, and
        store in self.adjustment (high number is worse).  It has to be
        called after all the blobs have been created, because
        relative distances matter."""
        self.mean_distance = sum(self.distances) / len(self.distances)
        self.relative_distance = self.mean_distance / total_mean
        self.min_distance = min(x for x in self.distances if x != 0)
        self.relative_min_distance = self.min_distance / total_mean

        s = self.blob
        #adjustment based on size.
        ratio = float(s.convex_perimeter) / config.IDEAL_SPRITE_SIZE
        adj = 1 + abs(log(ratio * 3, 3) - 1) ** 4
        #print "blob of size %s has ratio %1.4s -> adj %1.4s." % (s.convex_perimeter, ratio, adj),

        #based on obviousness (3 ways of calculating, cumulative)
        for ob, limit in ((s.obviousness, 36.0),
                          (s.obviousness_best, 255.0),
                          (s.obviousness_sum, 1000.0)):
            if ob < limit:
                adj *= limit / (0.5 + ob)

        #this part of adjustment also applies to enemy
        self.monster_rating = 1.0 / adj

        #adjust for placement in picture -- central is more likely
        edge_h = min(s.pos[0], config.WORKING_SIZE[0] - s.pos[2])
        if edge_h < config.EDGE_H:
            adj *=  (2.0 * config.EDGE_H) / (config.EDGE_H + edge_h)
            #print "adjusting by %s because %s from vertical edge" % \
            #      ((2.0 * config.EDGE_H) / (config.EDGE_H + edge_h), edge_h)

        edge_v = min(s.pos[1], config.WORKING_SIZE[1] - s.pos[3])
        if edge_v < config.EDGE_V:
            adj *= (2.0 * config.EDGE_V) / (config.EDGE_V + edge_v)
            #print "adjusting by %s because %s from vertical edge" % \
            #      ((2.0 * config.EDGE_V) / (config.EDGE_V + edge_v), edge_v)

        print s.pos, edge_h, edge_v
        if edge_h < 1 or edge_v < 1: # ignore touching or over the edge
            print "edge is too close!"
            adj *= 5000
            self.monster_rating *= 0.7

        # adjust for difference from others
        adj /= (self.relative_distance * self.relative_min_distance ** 1.5)
        #adj /= (self.relative_distance + 4 * self.relative_min_distance)

        #print "mean d %1.4s, min distance %1.4s, final adjustment: %1.5s. monster_rating %1.5f" %\
        #      (self.relative_distance, self.relative_min_distance, adj, self.monster_rating)

        self.adjustment = adj




class Mean(list):
    """A vector in the blob evaluation space.  The .blobs
    attribute is filled with a list of blobs that are closer to this
    than other Means."""
    circumference = 0
    def __init__(self, *args):
        list.__init__(self, *args)
        self.blobs = []



def d_euclidean(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b)) ** 0.5


def kmeans(svectors, n_means):
    """Find clusters via k-means algorithm."""
    #initialise the mean vectors to reasonable(?) points
    means = [Mean([ random.uniform(min(v), max(v))
                    for v in zip(*svectors) ])
             for i in range(n_means) ]

    old_means = None
    while means != old_means:
        old_means = means
        for m in means:
            m.blobs = []
        for s in svectors:
            distances = [ d_euclidean(m, s) for m in means ]
            bd = min(distances)
            bm = distances.index(bd)
            means[bm].blobs.append(s)

        for m in means:
            vsums = zip(*m.blobs)
            lv = float(len(m.blobs))
            for i, x in enumerate(vsums):
                m[i] = sum(x) / lv

    means = [m for m in means if m.blobs]
    #it has settled into the final state. work out the total error (squared).
    #XXX should test this!
    error = 0.1
    for m in means:
        m.circumference = max(d_euclidean(s, m) for s in m.blobs)
        vsums = zip(*m.blobs)
        for centre, pop in zip(m, vsums):
            error += sum((p - centre) ** 2 for p in pop)

    return error, [m.blobs for m in means], means


def dimension_stats(d):
    """calculate mean and std dev"""
    mean = float(sum(d)) / len(d)
    diffs2 = sum((x - mean) ** 2 for x in d)
    variance = diffs2 / len(d)
    return mean, variance ** 0.5



class TeamFinder:
    """Find clusters of blobs, somehow"""
    def __init__(self, blobs, klass=BlobVector, entropy=None):
        if len(blobs) < config.MIN_SPRITES:
            raise SpriteError("maybe enough in theory (%d) but an error is inevitable" % len(blobs))

        if entropy is not None:
            #reseed, because there has probably been a bifurcation, with entropy different in each fork
            random.seed(entropy[6])

        self.svectors = []
        for x in blobs:
            vector = klass()
            vector.evaluate(x)
            self.svectors.append(vector)

        #work out mutual distance between all blobs
        #XXX could include self, thereby keeping a useful lookup, but useful for what?
        for v in self.svectors:
            v.distances = [d_euclidean(v, w) for w in self.svectors if w is not v]

        all_distances = sum(sum(v.distances) for v in self.svectors)
        mean_distance = all_distances / (len(self.svectors) ** 2)

        for v in self.svectors:
            v.calculate_adjustment(mean_distance)

        self.invert = zip(*self.svectors)

    def print_stats(self):
        """print info about the range of vector dimensions"""
        for x in self.invert:
            mean, stddev = dimension_stats(x)
            print ("min %8.2f mean %8.2f max %8.2f (range %8.2f) stddev %8.2f" %
                   (min(x), mean, max(x), max(x) - min(x), stddev))


    def _get_arrangement(self, player, others, n):
        adj = player.adjustment
        svectors = [x for x in others if x is not player]
        error, teams, means = kmeans(svectors, n - 1)
        md = 1e300
        containable = 1
        for m in means:
            d = d_euclidean(player, m)
            md = min(d, md)
            if (d < m.circumference and d):
                containable = max(m.circumference / d, containable)

        adj_error = error * player.adjustment * containable / md ** 2
        #print ("n: %1d, player size: %3d, adj: %4.2f, "
        #       "err: %7.2f -> %5.2f. md: %4.2f, containable: %3.2f"
        #       %(n, player.blob.convex_perimeter, adj, error, adj_error, md, containable))
        return adj_error, teams



    def find_teams(self, min_teams=config.TEAMS_MIN, max_teams=config.TEAMS_MAX):
        """Look for teams until a cluster of 1 is found. Try to stay
        below max_teams, but there is no guarantee."""
        #XXX probably need to special case:
        # really big things (environmental)
        # really small things (bullets, food)
        # the player.

        tt = time.time()
        sizes = range(min_teams, max_teams + 1)
        sizes += sizes[1:-1]
        print "sizes are %s" % sizes
        best = None
        tries = 0
        sorted_vectors = [x[1] for x in sorted((x.adjustment, x) for x in self.svectors)]
        candidates = []
        for x in range(1, 7):
            # 7-> potentially 91 goes, 36 candidates.
            # 8-> potentially 140 goes, 49 candidates.
            candidates.extend(sorted_vectors[:x * x])
        for player in candidates:
            others = [x for x in sorted_vectors if x is not player]
            error, teams = self._get_arrangement(player, others, random.choice(sizes))
            if best is None or error < best.error:
                best = Arrangement(player, teams, error)

        print "finding teams took %s " % (time.time() - tt)
        print "best error is %s" % best.error
        return best


#------------------------------------------------------------


class Arrangement:
    def __init__(self, vplayer, vteams, error):
        #start the teams off with player at bit 1, biggest at bit 2 (enemy)
        #this can change later
        self.vectors = [[vplayer]]
        #sorting by size, but no good reason!
        self.vectors.extend(reversed([y[1] for y in sorted((len(x), x) for x in vteams)]))
        self.error = error
        #print "made arrangement with player of size %s, error %s" %(vplayer.blob.convex_perimeter, error)
