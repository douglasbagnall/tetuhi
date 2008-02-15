#!/bin/sh
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
cd ~/tetuhi/c
for opts in \
    " -DLINESCAN=1 -DLOWRES=1 -DPRESCAN=1 -DFAST_SWEEP=1 -DLOWRES_BITS=4 -DMIDRES_BITS=4" \
    " -DLINESCAN=0 -DLOWRES=1 -DPRESCAN=1 -DFAST_SWEEP=1 -DLOWRES_BITS=4 -DMIDRES_BITS=4" \
    " -DLINESCAN=1 -DLOWRES=1 -DPRESCAN=1 -DFAST_SWEEP=1 -DLOWRES_BITS=5 -DMIDRES_BITS=4" \
    " -DLINESCAN=0 -DLOWRES=1 -DPRESCAN=1 -DFAST_SWEEP=1 -DLOWRES_BITS=5 -DMIDRES_BITS=4" \
    " -DLINESCAN=1 -DLOWRES=0 -DPRESCAN=1 -DFAST_SWEEP=1                 -DMIDRES_BITS=4" \
    " -DLINESCAN=0 -DLOWRES=0 -DPRESCAN=1 -DFAST_SWEEP=1                 -DMIDRES_BITS=4" \
    " -DLINESCAN=1 -DLOWRES=1 -DPRESCAN=1 -DFAST_SWEEP=1 -DLOWRES_BITS=5 -DMIDRES_BITS=5" \
    " -DLINESCAN=0 -DLOWRES=1 -DPRESCAN=1 -DFAST_SWEEP=1 -DLOWRES_BITS=5 -DMIDRES_BITS=5" \
    " -DLINESCAN=1 -DLOWRES=0 -DPRESCAN=1 -DFAST_SWEEP=1                 -DMIDRES_BITS=5" \
    " -DLINESCAN=0 -DLOWRES=0 -DPRESCAN=1 -DFAST_SWEEP=1                 -DMIDRES_BITS=5" \
    " -DLINESCAN=0 -DLOWRES=0 -DPRESCAN=0 -DFAST_SWEEP=1                                " \
    " -DLINESCAN=0 -DLOWRES=0 -DPRESCAN=0 -DFAST_SWEEP=0                                " \
; do

    make clean
    CPPFLAGS2=$opts make -e python-install
    python test.py -c
done

cd -