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
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
/*
#include <limits.h>

*/


#include "libperceptron.h"
//#include "perceptron.c"


#define XOR_TEST   1
#define COUNT_TEST 2
#define COUNT_TEST_2 3

#define SET_SHAPE   {3, 7, 1, 0}
#define SET_INPUTS  {0,0,0, 0,0,1, 0,1,0, \
	    0,1,1, 1,0,0, 1,0,1,                \
	    1,0,1, 1,1,0, 1,1,1,		\
	    0,1,1, 1,1,0, 1,0,0,		\
	    0,0,0, 0,1,1, 1,1,1,		\
	    0,1,0, 1,0,0, 0,1,1,		\
	    1,0,0, 1,0,1, 1,1,0,		\
	    0,0,1, 1,0,1, 0,1,0,		\
	    }
#define SET_N 3
#define SET_SETS 8
#define SET_BIAS 1



#define USE_TEST XOR_TEST

#define TEST_MOMENTUM 0.0
#define TEST_LEARN_RATE 0.34
#define TEST_SCALE 1.0
#define TEST_RAND_EXTREMA 0.2

#if (USE_TEST == XOR_TEST)
#define TEST_SHAPE   {2, 4, 1, 0}
#define TEST_INPUTS  {1,1, 1,0, 0,1, 0,0}
#define TEST_OUTPUTS { 0,   1,   1,   0 }
#define TEST_LENGTH 4
#define TEST_BIAS 0
#endif

#if (USE_TEST == COUNT_TEST)
#define TEST_SHAPE   {3, 7, 2, 0}
#define TEST_INPUTS  {0,0,0, 0,0,1, 0,1,0, 0,1,1, 1,0,0, 1,0,1, 1,1,0, 1,1,1}
#define TEST_OUTPUTS { 0,0,   0,1,   0,1,   1,0,    0,1,  1,0,   1,0,    1,1 }
#define TEST_LENGTH 8
#define TEST_BIAS 1
#endif
#if (USE_TEST == COUNT_TEST_2)
#define TEST_SHAPE   {3, 11, 1, 0}
#define TEST_INPUTS  {0,0,0, 0,0,1, 0,1,0, 0,1,1, 1,0,0, 1,0,1, 1,1,0, 1,1,1}
#define TEST_OUTPUTS { 0.0,  0.33,  0.33,  0.67,  0.33,  0.67,  0.67,  1.0 }
#define TEST_LENGTH 8
#define TEST_BIAS 1
#endif




#define TEST_ACCEPTABILITY 0.05


nn_Network_t *
get_test_net(){
    unsigned int sizes[NN_MAX_LAYERS] = TEST_SHAPE;
    nn_Network_t *net = nn_new_network(sizes, TEST_BIAS);
    nn_randomise_weights(net, -TEST_RAND_EXTREMA, TEST_RAND_EXTREMA);
    return net;
}


static inline int
show_opinions(nn_Network_t *net, weight_t *inputs, int len,
	      weight_t *targets, weight_t threshold){
    int i, j;
    int insize = net->input->insize;
    int outsize = net->output->insize;
    weight_t *opinion;
    int pass = 1;
    for (i = 0; i < len; i++){
	weight_t err = 0;
	opinion = nn_opinion(net, inputs);
	printf("[");
	for (j = 0; j < insize; j++){
	    printf("%0.4f, ", inputs[j]);
	}
	printf("] -> [");
	for (j= 0; j < outsize; j++){
	    printf("%f, ", opinion[j]);
	    err += FABS(opinion[j] - targets[j]);
	}
	int ok = (err / outsize < threshold);
	pass = pass && ok;
	printf("] %0.3f  %s \n",
	       err / outsize,
	       ok ? "*": ""
	       );
	targets += outsize;
	inputs += insize;
    }
    if(!pass){
	printf("Something is WRONG\n");
	return 1;
    }
    return 0;
}

/* opinion speed test */

void
test_opinion_speed(){
    int i;
    int N = 1000000;
    unsigned int shape[6] = {57, 15, 1, 0};
    nn_Network_t *net = nn_new_network(shape, 0);
    nn_randomise_weights(net, -TEST_RAND_EXTREMA, TEST_RAND_EXTREMA);
    //doesn't matter that it is all 0.
    weight_t *test_mem = calloc(N * 60, sizeof(weight_t));
    weight_t *inputs = test_mem;
    weight_t ticker;
    double t = clock();

    for (i = 0; i < N; i++){
	__builtin_prefetch(inputs + (256 / sizeof(weight_t)), 0, 0);
	__builtin_prefetch(inputs + (512 / sizeof(weight_t)), 0, 0);
	ticker += *nn_opinion(net, inputs);
	inputs += 60;
    }
    printf("took %f seconds to do %d cycles\n", (clock() - t)/CLOCKS_PER_SEC, N);
    //nn_print_weights(net);
    //debug_network(net);
    nn_delete_network(net);
    free(test_mem);
}


nn_Network_t *
test_backprop(){
    nn_Network_t *net = get_test_net();
    weight_t inputs[] = TEST_INPUTS;
    weight_t targets[] = TEST_OUTPUTS;
    int len = TEST_LENGTH;

    int count = 6000;
    double target_diff = 0.00001;
    int print_every = 0;
    int j;
    int cycles = 20;
    double t = clock();
    double diff;
    double diff_sum = 0.0;

    for (j = 0; j < cycles; j++){
	nn_randomise_weights(net, -TEST_RAND_EXTREMA, TEST_RAND_EXTREMA);

	diff = nn_train_set(net, inputs, targets,
			    len, count, TEST_LEARN_RATE, TEST_MOMENTUM,
			    target_diff, print_every);

	diff_sum += diff *diff;
	printf("diff was %.5f, RMS weights were %.6f\n", diff,
	       sqrt(nn_mean_weights_squared(net)));

    }

    printf("took %f seconds to do %d cycles\n", (clock() - t)/CLOCKS_PER_SEC, cycles);
    printf("RMS difference: %.8f\n", sqrt(diff_sum/cycles));

    show_opinions(net, inputs, len, targets, TEST_ACCEPTABILITY);

    //nn_delete_network(net);
    return net;
}

void
print_vectors_of_length(nn_Network_t *net, int n){
    int i, j;
    int max = 1 << n;
    weight_t inputs[n];
    weight_t prev = 1e100;
    for (i = 0; i < max; i++){
	for (j = 0; j < n; j++){
	    inputs[j] = (i & 1 << (n - j - 1)) >> (n - j - 1);
	    printf("%.1f ", inputs[j]);
	}
	weight_t *opinion = nn_opinion(net, inputs);
	char *up = (prev < *opinion) ? "^" : "";
	prev = *opinion;
	printf(" --> %.9f %s\n",  opinion[0], up);

    }
}


int
test_best_of_set(){
    unsigned int sizes[NN_MAX_LAYERS] = SET_SHAPE;
    nn_Network_t *net = nn_new_network(sizes, SET_BIAS);
    weight_t inputs[] = SET_INPUTS;
    weight_t test[SET_N];
    int sets = SET_SETS;
    int count = 5000;
    int i, j;
    int cycles = 1000;
    double t = clock();
    unsigned int diff;
    unsigned int misorders = 0, error = 0;

    for (j = 0; j < cycles; j++){
	nn_randomise_weights(net, -TEST_RAND_EXTREMA, TEST_RAND_EXTREMA);
	diff = nn_learn_best_of_sets(net, inputs, SET_N, sets, count);
	//debug("diff was %u, RMS weights were %f\n", diff, sqrt(nn_mean_weights_squared(net)));
	weight_t prev = 1e100;
	int score = 0;
	for (i = 0; i < 8; i++){
	    test[0] = (i & 4) >> 2;
	    test[1] = (i & 2) >> 1;
	    test[2] = (i & 1);
	    weight_t *opinion = nn_opinion(net, test);
	    //debug("opinion: %p: %f\n", opinion, *opinion);
	    score += (*opinion > prev);
	    prev = *opinion;
	}
	//if(score)
	    //printf("%d ordering errors\n", score);
	misorders += score;
	error += diff;
    }

    printf("took %f seconds to do %d cycles\n", (clock() - t)/CLOCKS_PER_SEC, cycles);
    printf("average %.3f misorders per cycle\n", (double)(misorders)/cycles);
    printf("average %.3f error per cycle\n", (double)(error)/cycles);
    //nn_print_weights(net);
    print_vectors_of_length(net, SET_N);
    nn_delete_network(net);
    return 0;
}

int
test_best_of_set_genetic(){
    unsigned int sizes[NN_MAX_LAYERS] = SET_SHAPE;
    nn_Network_t *net = nn_new_network(sizes, SET_BIAS);
    weight_t inputs[] = SET_INPUTS;
    weight_t test[SET_N];
    int sets = SET_SETS;
    int count = 1000;
    int population = 16;
    int i, j;
    int cycles = 1000;
    double t = clock();
    unsigned int diff;
    unsigned int misorders = 0, error = 0;

    for (j = 0; j < cycles; j++){
	nn_randomise_weights(net, -TEST_RAND_EXTREMA, TEST_RAND_EXTREMA);
	//print_vectors_of_length(net, SET_N);
	//nn_print_weights(net);
	diff = nn_learn_best_of_sets_genetic(net, inputs, SET_N, sets, count, population);
	//debug("diff was %u, RMS weights were %f\n", diff, sqrt(nn_mean_weights_squared(net)));
	//nn_print_weights(net);

	weight_t prev = 1e100;
	int score = 0;
	for (i = 0; i < 8; i++){
	    test[0] = (i & 4) >> 2;
	    test[1] = (i & 2) >> 1;
	    test[2] = (i & 1);
	    weight_t *opinion = nn_opinion(net, test);
	    //debug("opinion: %p: %f\n", opinion, *opinion);
	    score += (*opinion > prev);
	    prev = *opinion;
	}
	//if(score)
	    //printf("%d ordering errors\n", score);
	misorders += score;
	error += diff;
    }

    printf("took %f seconds to do %d cycles\n", (clock() - t)/CLOCKS_PER_SEC, cycles);
    printf("average %.3f misorders per cycle\n", (double)(misorders)/cycles);
    printf("average %.3f error per cycle\n", (double)(error)/cycles);
    //nn_print_weights(net);
    print_vectors_of_length(net, SET_N);
    nn_delete_network(net);
    return 0;
}




int test_anneal(){
    nn_Network_t *net = get_test_net();
    weight_t inputs[] = TEST_INPUTS;
    weight_t targets[] = TEST_OUTPUTS;
    int len = TEST_LENGTH;

    int count = 40000;

    double target_diff = 0.00001;
    int print_every = 0;
    //print_every = 20000;
    int j;
    int cycles = 20;
    double t = clock();
    double diff;
    double diff_sum = 0.0;
    double temperature = 55.0;
    double sustain = 0.9992;
    double min_temp = 0.0001;

    for (j = 0; j < cycles; j++){
	nn_randomise_weights(net, -TEST_RAND_EXTREMA, TEST_RAND_EXTREMA);
	diff = nn_anneal_set(net, inputs, targets,
			     len, count, target_diff,
			     print_every, temperature, sustain, min_temp);
	//printf("diff was %.5f, RMS weights were %.6f\n", diff,
	//       sqrt(nn_mean_weights_squared(net)));
	diff_sum += diff *diff;
    }
    printf("took %f seconds to do %d cycles\n", (clock() - t)/CLOCKS_PER_SEC, cycles);
    printf("RMS difference: %.8f\n", sqrt(diff_sum/cycles));

    show_opinions(net, inputs, len, targets, TEST_ACCEPTABILITY);

    nn_print_weights(net);
    nn_delete_network(net);
    return 0;

}

int
test_save(nn_Network_t *net){
    nn_save_weights(net, "/tmp/test.nn");
    debug("saved weights\n");
    nn_Network_t *net2 = get_test_net();

    nn_load_weights(net2, "/tmp/test.nn");
    debug("loaded weights\n");
    weight_t inputs[] = TEST_INPUTS;
    weight_t targets[] = TEST_OUTPUTS;
    int r = show_opinions(net2, inputs, TEST_LENGTH, targets, TEST_ACCEPTABILITY);
    if(r)
	debug("****************************************\n");
    return r;
}

int
test_duplicate(nn_Network_t *net){
    nn_Network_t *net2 = nn_duplicate_network(net);
    weight_t inputs[] = TEST_INPUTS;
    weight_t targets[] = TEST_OUTPUTS;
    show_opinions(net2, inputs, TEST_LENGTH, targets, TEST_ACCEPTABILITY);
    nn_Network_t *net3 = nn_duplicate_network(net2);
    show_opinions(net3, inputs, TEST_LENGTH, targets, TEST_ACCEPTABILITY);
    nn_delete_network(net2);
    return 0;
}





int main(){

    debug("hello\n");
    test_opinion_speed();
#if 0
    nn_Network_t *net = test_backprop();
    test_save(net);
    test_duplicate(net);
    test_anneal();
#endif
    test_best_of_set();
    test_best_of_set_genetic();
    return 0;
}
