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

INDIR=short
OUTDIR=mono
LOWER=lower
SLOWER=slower
REVERSED=reversed
FREQ=44100

for f in `ls $INDIR` ; do
    g=$f.wav
    echo $f
    sox $INDIR/$f -r $FREQ -2 -c 1 $OUTDIR/$g
    sox $OUTDIR/$g  $SLOWER/$g speed 0.6667
    sox $OUTDIR/$g  $LOWER/$g key -500 40
    sox $OUTDIR/$g  $REVERSED/$g reverse
    #echo -n .
done