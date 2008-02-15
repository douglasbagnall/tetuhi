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

#XXX can adjust focal length here.
# 35 to 350
# or maybe this should be in a separate place, so it can be less often adjsuted (ie, init only)
gphoto2  --set-config=/main/capturesettings/focallength=270

DIR=`dirname $1`
BASE_NAME=`basename $1`
mkdir -p $DIR

cd $DIR

CAPTURE_NAME='capt0000.jpg'


#rm $CAPTURE_NAME
gphoto2 --quiet --capture-image -f /store_00010001 -p $CAPTURE_NAME --force-overwrite

mv $CAPTURE_NAME $BASE_NAME

echo "REAL capture here!"
echo "did gphoto2 --quiet --capture-image -f /store_00010001 -p $CAPTURE_NAME"