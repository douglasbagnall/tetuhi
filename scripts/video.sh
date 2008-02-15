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

VIDEO_DIR=/tmp/

if [[ $1 ]]; then
    VIDEO_DIR=$1
fi


mencoder "mf://$VIDEO_DIR/*.jpg" -mf type=jpg:fps=25 -o $VIDEO_DIR/output.avi -ovc lavc -lavcopts vcodec=mpeg4
mencoder "mf://$VIDEO_DIR/*.jpg" -mf type=jpg:fps=25 -o $VIDEO_DIR/output-mpeg1.avi -ovc lavc -lavcopts vcodec=mpeg1video

#ffmpeg -i $VIDEO_DIR/___%d.jpg /tmp/output-ffmpeg.mpg

#ffmpeg -vcodec copy -i $VIDEO_DIR/output.avi $VIDEO_DIR/output.mp4
ffmpeg -vcodec copy -i $VIDEO_DIR/output-mpeg1.avi $VIDEO_DIR/output.mpg