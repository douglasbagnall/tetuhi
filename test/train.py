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
"""
        \/   |   \/
        /\15/|\8 /\
       /  \/ | \/  \
      /14 /\7|0/\ 9 \
    _/___/6|   |1\___\_
     \   \5|___|2/   /
      \13 \/4|3\/ 10/
       \  /\ | /\  /
        \/12\|/11\/
        /\   |   /\

   ______in,x,cols____
  |
 out,y,rows
  |
"""
import _test_common

from vg import config

import nnpy

def make_net(n_teams):
    net = nnpy.Network([16 + 16 * n_teams + config.EXTRA_INPUTS + config.FEEDBACK_NODES,
                        config.LAYER2_NODES, config.OUTPUT_NODES + config.FEEDBACK_NODES],
                       bias=1)
    return net


inner_weights = [5, 2, -1, -2, -2, -1, 2, 5]
outer_weights = [2, 1, 0, -1, -1, 0, 1, 2]


    

def chaser_weights(net, bits, chasee_bit):
    """turn towards stimulus."""
    n_inputs, n_hidden, n_outputs = net.shape
    net.zero_weights()
    #net.randomise((-0.005,0.005))
    net.dump_weights('/tmp/zero.weights')
    
    for b in range(bits):
        if b == chasee_bit:
            for output in range(4):
                for i in range(8):
                    o = (output * 2 + i) % 8
                    net.set_single_weight(0, b * 16 + o, output, inner_weights[i])
                    net.set_single_weight(0, b * 16 + 8 + o, output, outer_weights[i])

        elif b == config.B_WALL: #stay away from walls
            for output in range(4):
                for i in range(8):
                    o = (output * 2 + i) % 8
                    net.set_single_weight(0, b * 16 + o, output,  -0.3 * inner_weights[i])
                    net.set_single_weight(0, b * 16 + 8 + o, output,  -0.3 * outer_weights[i])
        elif False: #blanket hatred of others.
            for output in range(4):
                for i in range(8):
                    o = (output * 2 + i) % 8
                    net.set_single_weight(0, b * 16 + o, output, -0.1 * outer_weights[i])

    #if blocked, put emphasise on turn
    #and if not turning excessively, maybe bend
    for i in range(1,3):
        net.set_single_weight(0, bits * 16 + config.EXNODE_BLOCKED, i, 3)
        net.set_single_weight(0, bits * 16 + config.EXNODE_CONTINUITY, i, -1)

    #if stopped, do something
    for i in range(3):
        net.set_single_weight(0, bits * 16 + config.EXTRA_STOPPED, i, 4)

    #if turning excessively, straighten
    net.set_single_weight(0, bits * 16 + config.EXNODE_TURNY, 0, 3.5)





        

    #feed straight through from hidden to output.
    for o in range(n_outputs):
        net.set_single_weight(1, o, o, 1)

    net.save_weights('/tmp/net.weights')
    net.dump_weights('/tmp/net.weights.txt')



if __name__ == '__main__':
    net = make_net(2)
    chaser_weights(net, 3, 1)
