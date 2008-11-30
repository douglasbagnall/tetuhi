#!/usr/bin/python
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
#
# see the README file in the perceptron directory for information
# about possible future licensing.

from distutils.core import setup, Extension

setup(name="nnpy",
      version="1.0",
      ext_modules=[Extension("nnpy", ["nnpy.c"], libraries=["perceptron"])])
