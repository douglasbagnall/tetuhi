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

from vg import config
import nnpy


def seed_nets(entropy):
    """seed the nnpy generator. once per game works for all nets"""
    random_holding_net = nnpy.Network([2,3,1])
    random_holding_net.seed_random(entropy[1])
    return random_holding_net



#these need to be more independantly generated.

inner_weights  = [4, 2, -1, -2, -2, -1, 2, 4]
outer_weights  = [2, 1, 0, -1, -1, 0, 1, 2]
null_weights   = [0] * 8
bullet_inner_w = [-2, -1, 3, 2, 2, 3, -1, -2]
sidestep_weights = [-2, 1, 1, 0, 0, 1, 1, -2]

fire_weights =  [2, 0.5, 0, 0, 0, 0, 0.5, 2,
                 1, 0.2, 0, 0, 0, 0, 0.2, 1]

weight_sets = {
    'ignore':     (null_weights, null_weights),
    'follow':     (null_weights, outer_weights),
    'flock':      ([-1.0 * x for x in inner_weights], outer_weights),

    'chase-near': (inner_weights, null_weights),
    'chase':      (inner_weights, outer_weights),
    'flee':       ([-x for x in inner_weights], [-x for x in outer_weights]),
    'avoid':      ([-1.0 * x for x in inner_weights], null_weights),
    'sidestep':   (sidestep_weights, null_weights),
}

extra_weights = (
 ('mobile',        config.EXNODE_STOPPED,    [2, 1, 1, 1, 0, 1]),
 ('stubborn',      config.EXNODE_TURNY,      [1, -0.5, -0.5, -0.5, 0, 0]),
 ('stubborn',      config.EXNODE_CONTINUITY, [0, 0.3, 0.3, 0, 0, 0.2]),
 ('unblocks',      config.EXNODE_BLOCKED,    [-2, 2, 2, 1.5, 0, 2]),
# ('unpredictable', config.EXNODE_TURN,      [0, 0, 0, 0, 0, 2])
)



def _set_weights(net, bit, inputs_per_type, attention, inner_w, outer_w=None):
    """Set the weights to prefer motion as indicated by inner_w and
    outer_w. Only sets the first 4 outputs (which correspond to the 0,
    90, 180, 270 degree turns)"""
    for output in range(4):
        for i in range(8):
            o = (output * 2 + i) % 8
            net.set_single_weight(0, bit * inputs_per_type + o, output, inner_w[i] * attention)
            if inputs_per_type == 16:
                net.set_single_weight(0, bit * inputs_per_type + 8 + o, output, outer_w[i] * attention)

def _set_simple_weights(net, b, weights):
    for i, w in enumerate(weights):
        net.set_single_weight(0, b, i, w)



def get_weights(net, bit_pattern, character, inputs_per_type, fire_at):
    """Set the net's weights in accordance with its role.

    If the net has 2 layers, the, the weights will feed straight
    through.

    If it has 3, the second interlayer will pretty much just feed straight through.
    """
    #XXX not pretty
    #XXX later, look in database of previously used nets.
    bits = len(bit_pattern)
    if len(net.shape) == 3:
        n_inputs, n_hidden, n_outputs = net.shape
    else:
        n_inputs, n_outputs = net.shape
        n_hidden = n_outputs
    if config.RANDOMISE_WEIGHTS:
        net.randomise((-0.01, 0.01))
    else:
        net.zero_weights()

    # nodes relating to directions
    for i, b in enumerate(bit_pattern):
        intention, attention = b
        if intention is not None:
            _set_weights(net, i, inputs_per_type, attention, *weight_sets[intention])

    #directionless nodes
    for c, b, weights in extra_weights:
        if c in character:
            b += bits * inputs_per_type
            _set_simple_weights(net, b, weights)

    #firing - fire_at is a list of bits to fire at.
    for bit in fire_at:
        s = bit * inputs_per_type
        for i in range(inputs_per_type):
            net.set_single_weight(0, s + i, fire_output, fire_weights[i])


    if len(net.shape) == 3:
        #feed straight through from hidden to output.
        #XXX should put other nodes to use.
        for o in range(n_outputs):
            net.set_single_weight(1, o, o, 1)

    name = '-'.join(str(x) for x in bit_pattern)
    #net.save_weights('/tmp/%s.weights' % name)
    #net.dump_weights('/tmp/%s.dump' % name)




def get_net(team):
    inputs = (team.inputs_per_type * (team.game.n_teams + 1) +
              config.EXTRA_INPUTS + config.FEEDBACK_NODES)
    outputs = len(team.directions) + team.fires + config.FEEDBACK_NODES

    if config.LAYER2_NODES:
        shape = [inputs, config.LAYER2_NODES, outputs]
    else:
        shape = [inputs, outputs]

    return nnpy.Network(shape, bias=1)
