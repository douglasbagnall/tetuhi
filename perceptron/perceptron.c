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
#include <string.h>
#include <stdlib.h>
#include "libperceptron.h"

/*include the innermost loop from inline assembly */
#ifdef USE_X86_64_SSE
#include "opinion_sse64.c"
#else
#include "opinion_c.c"
#endif



/************************* Constructor ****************************/


nn_Network_t *
nn_new_network( unsigned int layer_sizes[], int bias){
    /* returns a network, with layers and interlayers set up.
     * layer_sizes is zero terminated; eg [2,4,3,0,2] will result in
     * a 3 tier network with sizes 2,4,3. */
    int i;
    nn_Network_t *net;
    size_t weight_alloc = 0;
    size_t node_alloc = 0;
    size_t net_alloc = ALIGNED_BYTES(sizeof(nn_Network_t));
    size_t total_bytes;

    bias = (bias != 0); //restrict to 1 or 0
    //find sizes, and do one big malloc for the whole net and weights
    //this simplifies freeing it, a little bit.
    for (i = 0; i < NN_MAX_LAYERS - 1; i++){
	if (layer_sizes[i] == 0)
	    break;
	weight_alloc += ALIGNED_SIZE((layer_sizes[i] + bias) * layer_sizes[i + 1]);
	node_alloc += ALIGNED_SIZE(layer_sizes[i] + bias);
    }
    if (i < 2){
        //ie, there are less than 2 layers
        _warn("given too few layers, need at least 2, got %u \n", i);
	return NULL;
    }
    /* +1 on next two, because transfer from python NULL-terminates */
    node_alloc += ALIGNED_SIZE(layer_sizes[i] + bias + 1);
    total_bytes = ALIGNED_BYTES(net_alloc + (node_alloc + weight_alloc + 1) * sizeof(weight_t));
    void *m = malloc(total_bytes);
    if (! m){
        _warn("no memory for network!");
        return NULL;
    }
    /* we have the memory. zero all the elements of the network structure -- weights and nodes don't care*/
    net = (nn_Network_t *)m;
    memset(net, 0, sizeof(nn_Network_t));
    net->depth = i;
    net->nodes = m + net_alloc;
    net->weights = m + net_alloc + node_alloc * sizeof(weight_t);
    net->bias = bias;
    net->total_alloc_bytes = total_bytes;
    net->node_alloc_size = node_alloc;
    net->weight_alloc_size = weight_alloc;

    //debug("net %u  nodes %u   weights %u  total %u\n", net_alloc, node_alloc, weight_alloc, total_bytes);

    node_t *n = net->nodes;

    //set up layers
    for (i = 0; i < net->depth; i++){
	nn_Layer_t *layer = net->layers + i;
	layer->insize = layer_sizes[i];
	layer->outsize = layer_sizes[i] + net->bias;
	layer->values = n;
	n += ALIGNED_SIZE(layer->outsize);
	layer->values[layer_sizes[i]] = 1.0; //bias sits safely unchanged, whether or not it is used.
    }
    weight_t *w = net->weights;

    //set up interlayers
    for (i = 0; i < net->depth - 1; i++){
	nn_Interlayer_t *il = net->interlayers + i;
	il->input = net->layers + i;
	il->output = net->layers + i + 1;
	il->size = (il->input->outsize * il->output->insize);
	il->weights = w;
	w += ALIGNED_SIZE(il->size);
    }
    net->input = net->layers;
    net->real_inputs = net->input->values;
    net->output = net->layers + net->depth - 1;
    return net;
}



/*********************** Destructor ***************************/


void
nn_delete_network(nn_Network_t *net){
    if (net->deltas){
	free(net->deltas);
	net->deltas = NULL;
    }
    if (net->weight_lut){
	free(net->weight_lut);
	net->weight_lut = NULL;
    }
    if (net->momentums){
	free(net->momentums);
	net->momentums = NULL;
    }
    free(net);
}

/************** duplicating networks ***/
/* nn_duplicate_network creates a new self contained copy of a network.
   it *doesn't* copy delta or momentum arrays
   weight_lut pointers and size will be in same state as original.
*/

static inline void
copy_net_core(nn_Network_t *orig, nn_Network_t *copy){
    void *o = (void *) orig;
    void *m = (void *) copy;
    int i;
    memcpy(m, o, orig->total_alloc_bytes);
    //fix all the pointers
    for (i = 0; i < copy->depth; i++){
	copy->layers[i].values = m + ((void *)orig->layers[i].values - o);
	copy->layers[i].deltas = NULL;
    }
    for (i = 0; i < copy->depth - 1; i++){
	copy->interlayers[i].input = copy->layers + i;
	copy->interlayers[i].output = copy->layers + i + 1;
	copy->interlayers[i].weights = m + ((void *)orig->interlayers[i].weights - o);
	copy->interlayers[i].momentums = NULL;
    }
    copy->input = copy->layers;
    copy->output = copy->layers + copy->depth -1;
    copy->real_inputs = copy->input->values;
    //copy->input = m + ((void *)orig->input - o);
    //copy->output = m + ((void *)orig->output - o);
    copy->weights = m + ((void *)orig->weights - o);
    copy->nodes = m + ((void *)orig->nodes - o);
    copy->momentums = NULL;
    copy->deltas = NULL;
}


nn_Network_t *
nn_duplicate_network(nn_Network_t *orig){
    nn_Network_t *copy = malloc(orig->total_alloc_bytes);
    if (! copy){
	_warn("couldn't alloc new network!\n");
	return NULL;
    }
    copy_net_core(orig, copy);
    return copy;
}

inline void
nn_copy_network_details(nn_Network_t *orig, nn_Network_t *copy){
    copy_net_core(orig, copy);
}


/*********** initialisers ****************************/

void
nn_randomise_weights(nn_Network_t *net, double min_val, double max_val){
    // put random values in all the weight arrays -also zeros momentum.
    double scale = (max_val - min_val);
    int i, j;
    for (i = 0; i < net->depth - 1; i++){
	for (j = 0; j < net->interlayers[i].size; j++){
	    net->interlayers[i].weights[j] = min_val + nn_rng_uniform_double(scale);
	}
    }
}

void
nn_zero_weights(nn_Network_t *net){
    /*could use memset, or even nn_randomise_weights(0,0) */
    int i, j;
    for (i = 0; i < net->depth - 1; i++){
	for (j = 0; j < net->interlayers[i].size; j++){
	    net->interlayers[i].weights[j] = 0.0;
	}
    }
}

/* setting individual weights. returns 0 on success, -1 if the index is inappropriate. */

int
nn_set_single_weight(nn_Network_t *net, int interlayer, int input, int output, weight_t value){
    nn_Interlayer_t *il;
    //printf("got interlayer %d, input %d, output %d, value %f\n", interlayer, input, output, value);
    if (interlayer < net->depth - 1){
	il = &(net->interlayers[interlayer]);
	int cols = il->input->outsize;
	int rows = il->output->insize;
	if (output < rows && input < cols){
	    il->weights[output * cols + input] = value; // or is it the other way round?
	    return 0;
	}
    }
    return -1;
}


/* nn_opinion returns an output vector given some inputs if input
   pointer points to existing net inputs (from previous opinion,
   training or explicit setting), time is saved.
   //XXX not checking for overlap (don't do that)
*/

inline weight_t *
nn_opinion(nn_Network_t *net, weight_t *inputs){
    if (inputs != net->real_inputs){
	memcpy(net->input->values, inputs, net->input->insize * sizeof(weight_t));
	inputs = net->input->values;
    }
    return inline_opinion(net, inputs);
}





/*************** disk IO ***************/

#define NET_HEADER           "multilayer perceptron\nbias: %d\n"
#define NET_HEADER_READABLE  "dump of multilayer perceptron\nbias: %d\n"
#define LAYER_HEADER         "layer size: %u\n"
#define DATA_HEADER          "weights follow two carriage returns (%u numbers, %u bytes each)\n"
#define END_HEADER           "good luck!\n\n"

int
nn_save_weights(nn_Network_t *net, char *filename){
    /*saves the weight arrays to the named file
     * in an undifferentiated stream */
    unsigned int i;
    FILE *f = fopen(filename, "wb");
    if (! f){
        _warn("couldn't open file '%s'", filename);
        return -1;
    }
    /*descriptive headers at start */
    fprintf(f, NET_HEADER, net->bias);
    for (i = 0; i < NN_MAX_LAYERS; i++){
	if (! net->layers[i].insize)
	    break;
	fprintf(f, LAYER_HEADER, net->layers[i].insize);
    }
    fprintf(f, DATA_HEADER, net->weight_alloc_size, sizeof(weight_t));
    fputs(END_HEADER, f);
    //write the net itself
    if (fwrite(net->weights, sizeof(weight_t), net->weight_alloc_size, f)){
        _warn("could'nt write to '%s'", filename);
        return -1;
    }
    fclose(f);
    return 0;
}

int
nn_load_weights(nn_Network_t *net, char *filename){
    /* loads the weight arrays from the named file. Neither robust nor secure.*/
    int i;
    unsigned int count, size;
    int bias;
    size_t a;
    unsigned int sizes[NN_MAX_LAYERS];
    int shush = 0;
    FILE *f = fopen(filename, "rb");
    if (! f){
        _warn("couldn't open file '%s'\n", filename);
        return -1;
    }
    shush += fscanf(f, NET_HEADER, &bias);
    if (net->bias != bias){
        _warn("saved net has %s bias, unlike this one -- can't cope with this yet\n", bias ? "": "no ");
        goto hell;
    }

    for (i = 0; i < NN_MAX_LAYERS; i++){
	if(! fscanf(f, LAYER_HEADER, sizes + i))
	    break;
	if(sizes[i] != net->layers[i].insize){
	    _warn("saved layer %d is size %u, wanted %u\n", i, sizes[i], net->layers[i].insize);
        goto hell;
	}
    }
    sizes[i] = 0;
    shush += fscanf(f, DATA_HEADER, &count, &size);
    if (sizeof(weight_t) != size || count != net->weight_alloc_size){
        _warn("saved net won't fit: %u numbers of %u bytes; wanted %u x %u\n",
	      count, size, (uint32_t)net->weight_alloc_size, (uint32_t)sizeof(weight_t));
        goto hell;
    }

    //shush += fseek(f, strlen(END_HEADER), SEEK_CUR);
    shush += fscanf(f, END_HEADER);

    if(size == sizeof(weight_t) &&
       net->depth == i //&& so on and so forth
       ){
	a = fread(net->weights, sizeof(weight_t), count, f);
	if (a != count){
	    _warn("error reading file '%s'\n", filename);
	    goto hell;
	}
    }
    fclose(f);
    return 0;
 hell:
    fclose(f);
    return -1;
}


int
nn_dump_weights_formatted(nn_Network_t *net, char *filename){
    /*saves the weight arrays to the named file
     * in a hopefully readable format -- designed for people, not machines. */
    unsigned int i, j, k;
    nn_Interlayer_t *il;
    FILE *f = fopen(filename, "wb");
    if (! f){
        _warn("couldn't open file '%s'", filename);
        return -1;
    }
    /*descriptive headers at start */
    fprintf(f, NET_HEADER_READABLE, net->bias);
    for (i = 0; i < NN_MAX_LAYERS; i++){
	if (! net->layers[i].insize)
	    break;
	fprintf(f, LAYER_HEADER, net->layers[i].insize);
    }
    fprintf(f, "  ____inputs____\n  |\n  |\noutputs\n  |\n  |\n");

    // write each interlayer
    for (i = 0; i < net->depth - 1; i++){
	fprintf(f, "\nInterlayer %d\n\n    ", i);
	il = net->interlayers + i;
	int cols = il->input->outsize;
	int rows = il->output->insize;
	if (! il->size)
	    break;
	for (j = 0; j < il->input->insize; j++){
	    fprintf(f, " %6d ", j);
	}
	if (net->bias)
	    fprintf(f, "  bias");
	fprintf(f, "\n");
	for (j = 0; j < rows; j++){
	    fprintf(f, "%4d", j);
	    for (k = 0; k < cols; k++){
		double w = il->weights[j * cols + k];
		if (fabs(w) > 999999){
		    fprintf(f, "% 7.1g ", w);
		}
		else{
		    fprintf(f, "% 7.2f ", w);
		}
	    }
	    fprintf(f, "\n");
	}
    }
    fclose(f);
    return 0;
}
