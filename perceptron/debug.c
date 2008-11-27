/*
 * Copyright (C) 2008 Douglas Bagnall
 *
 * This file is part of Te Tuhi Video Game System, or Te Tuhi for short.
 *
 * Te Tuhi is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Te Tuhi is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Te Tuhi.  If not, see <http://www.gnu.org/licenses/>.
 *
 * see the README file in the perceptron directory for information
 * about possible future licensing.
 */
#include "libperceptron.h"

/****************** debug****/

void
nn_print_weights(nn_Network_t *net){
    int i, j;
    int width;
    nn_Interlayer_t *il;
    for (i = 0; i < net->depth - 1; i++){
	il = net->interlayers + i;
	width = il->input->outsize;
	printf("\n");
	for (j = 0; j < il->size; j++){
	    if (! j % width){
		printf("\n");
	    }
	    printf(" %.2f,", il->weights[j]);
	}
	printf("\n");
    }
}


void
debug_interlayer(nn_Interlayer_t *il){
    debug("interlayer: %p {\n input: %p\n output:%p\n weights: %p\n momentums %p\n "
	  "input values: %p\n output values: %p\n size: %u\n }\n",
	  il, il->input, il->output, il->weights, il->momentums,
	  il->input->values, il->output->values,
	  il->size);
}

void
debug_network(nn_Network_t *net){
    debug("network: %p {\n weights: %p\n nodes:%p\n deltas: %p\n momentums %p\n "
	  "depth: %d\n input_size: %u\n output_size: %u\n weight_alloc_size: %zu\n "
	  " node_alloc_size: %zu\n}\n",
	  net, net->weights, net->nodes, net->deltas, net->momentums,
	  net->depth, net->layers->insize, net->output->insize,
	  net->weight_alloc_size, net->node_alloc_size);
#if 1
    int i;
    for(i = 0; i < min(NN_MAX_LAYERS, net->depth - 1); i++){
	debug_interlayer(net->interlayers + i);
    }
#endif
}
