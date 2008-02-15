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

DIR=~/tetuhi/audio

for d in `ls $DIR`; do
    cd $DIR/$d
    for f in `ls *.wav`; do	
	gain=$(sox $f -n stat -v  2>&1)
	calc=$(perl -e '$g = pop; print (($g > 25) ? (25.0+$g)/50 : (($g <6.0) ? (6+$g)/12.0 :1.0))' $gain)
	echo gain for $f is $gain. calc is $calc
	sox $f $f-2.wav vol $calc
	mv $f-2.wav $f
    done
done

