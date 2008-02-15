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



import sys
INSTALL_PREFIX = '/usr/local'



def get_version():
    from os import popen
    pipe = popen('svnversion')
    v = pipe.read()
    print v
    return str(v)




if __name__ == '__main__':
    if 'install' in sys.argv:
        sys.argv.append('--prefix=' + INSTALL_PREFIX)
    #sys.prefix = INSTALL_PREFIX
    #sys.exec_prefix = INSTALL_PREFIX
    from distutils.core import setup, Extension
    setup(name="imagescan",
          version=get_version(), 
          ext_modules=[Extension("imagescan", ["img.c"])])
