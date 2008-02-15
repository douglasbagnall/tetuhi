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


/*************** gettin an opinion: **********************
   nn_opinion successively calls calculate interlayer
 **/

static inline void
nn_calculate_interlayer(nn_Interlayer_t *il){
    /*ask for answer from input nodes, multiply by weights, and hand to
     * output nodes. returns nothing.
     *
     *   ______in,x,cols____
     *  |
     * out,y,rows
     *  |
     *  |                                 */
    asm("/*C interlayer here */");
    int rows = il->output->insize;
    int cols = il->input->outsize;  //includes bias, if any.
    int x, y, y2;
    weight_t a;
    for (y = 0, y2 = 0; y < rows; y2 += cols, y ++){
        a = 0.0;
        for (x = 0; x < cols; x++){
            a += (il->input->values[x]) * (il->weights[y2 + x]);
        }
        //deform
        il->output->values[y] = DEFORM(a);
    }
    //asm("/*loop ends*/");
}



/* calculate output given a set of inputs.
*/

static inline weight_t *
inline_opinion(nn_Network_t *net, weight_t *inputs){
    int i;
    weight_t *orig_input_values = net->layers->values;
    net->input->values = inputs;
    for (i = 0; i < (net->depth - 1); i++){
	nn_calculate_interlayer(net->interlayers + i);
    }
    net->input->values = orig_input_values;
    return net->output->values;
}
