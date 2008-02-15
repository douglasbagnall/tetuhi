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
"""Makes nonsense words using N-grams."""
#XXX not actually used
import random

class Ngram:
    base_sep = '\n'
    def __init__(self, N, fn=None):
        self.store = {}
        self.sep = self.base_sep * (N - 1)
        if fn is not None:
            self.parseFile(fn)

    def parseFile(self, fn, lower=False):
        f = open(fn)
        if lower:
            unique = set(x.lower().strip() for x in f)
        else:
            unique = set(x.strip() for x in f)
        f.close()
        s = self.sep.join(unique) + self.sep
        pair = self.sep
        store = self.store
        for letter in s:
            store[pair] = store.get(pair, '') + letter
            pair = pair[1:] + letter

    def makeWords(self, count):
        """returns a string of madeup words."""
        text = []
        pair = self.sep
        choice = random.choice
        while count:
            #need to sort in order to remain deterministic (because set ordering is randomised)
            next = choice(sorted(self.store.get(pair, self.sep)))
            text.append(next)
            pair = pair[1:] + next
            if pair == self.sep:
                count -= 1
        return ''.join(text).strip()





class NameFactory:
    def __init__(self, N, files, lower=False):
        self.ngram = Ngram(N)
        for fn in files:
            self.ngram.parseFile(fn, lower=lower)

    def name(self):
        n = ''
        while not n:
            n = self.ngram.makeWords(1)
        return n
