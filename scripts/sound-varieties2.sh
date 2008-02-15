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

# for latecomers

INDIR=$1

cd $INDIR

for f in `ls ` ; do    
    echo $f 
    sox $f  $f-slower.wav speed 0.6667
    sox $f  $f-faster.wav speed 1.3333
    sox $f  $f-lower.wav key -500 40
    sox $f  $f-reverse.wav reverse
    #echo -n .
done