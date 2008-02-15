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
"""Stuff to make testing easier"""
import sys

sys.path.append('/home/douglas/tetuhi')
sys.path.append('..')

VERBOSE = True

class Fake(object):
    """generic fake recursing object"""
    parent = None
    name = 'anonymous'

    def __init__(self, name=None):        
        if name is not None:
            self.name = name
        if VERBOSE:
            print "created fake %s object" % self.name

    def __getattr__(self, attr):
        return Fake("%s.%s" % (self.name, attr))
        


if __name__ == '__main__':
    f = Fake()
    print f.a.k.e

