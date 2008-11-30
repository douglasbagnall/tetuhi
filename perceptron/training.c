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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libperceptron.h"
//#include "dSFMT/dSFMT.h"


/* static functions */

/* training.c */
static int nn_alloc_momentums(nn_Network_t *);
static void nn_free_momentums(nn_Network_t *);
static void nn_train_interlayer(nn_Interlayer_t *, weight_t, weight_t);
static int nn_alloc_weight_lut(nn_Network_t *);
static inline void nn_unmutate(weight_t **, weight_t *);
static inline double nn_mean_squared_error(nn_Network_t *, weight_t *, weight_t *, int);


/*include the innermost loop from inline assembly */
#ifdef USE_X86_64_SSE
#include "opinion_sse64.c"
#else
#include "opinion_c.c"
#endif


/* Random numbers. */
#ifdef USE_INT_RAND

#include "SFMT/SFMT.h"
static int rng_is_init = 0;

void nn_rng_init(unsigned int seed){
    if (seed == -1)
	seed = (unsigned int) time(0) + (unsigned int) clock();
    if (seed == 0)
	seed = 12345;
    init_gen_rand(seed);
    rng_is_init = 1;
}

void nn_rng_maybe_init(unsigned int seed){
    if (! rng_is_init)
	nn_rng_init(seed);
}

inline int nn_rng_uniform_int(int limit){
    uint32_t scale = UINT_MAX / limit;
    uint32_t answer;
    do{
	answer = gen_rand32() / scale;
    }
    while (answer >= limit);
    return answer;
}

inline double nn_rng_uniform_double(double limit){
    return genrand_real2() * limit;
}

#else

#include "dSFMT/dSFMT.h"

static dsfmt_t dsfmt __attribute__ ((aligned));
static int rng_is_init = 0;

void nn_rng_init(unsigned int seed){
    if (seed == -1)
	seed = (unsigned int) time(0) + (unsigned int) clock();
    if (seed == 0)
	seed = 12345;
    dsfmt_init_gen_rand(&dsfmt, seed);
    rng_is_init = 1;
}

void nn_rng_maybe_init(unsigned int seed){
    if (! rng_is_init)
	nn_rng_init(seed);
}

inline int nn_rng_uniform_int(int limit){
    return (int) (dsfmt_genrand_close_open(&dsfmt) * limit);
}

inline double nn_rng_uniform_double(double limit){
    return dsfmt_genrand_close_open(&dsfmt) * limit;
}

#endif



/*********** allocating and freeing momentum and delta arrays **/
/* this could be done at the time of network construction, but since the
   arrays are only used in backpropagated block training, it is wasteful
   to have them around always.
*/


static int
nn_alloc_momentums(nn_Network_t *net){
    unsigned int i;
    weight_t *m;
    if(net->momentums){
	return 1;
    }

    m = calloc(net->weight_alloc_size, sizeof(weight_t));
    debug("momentums %p,size %u\n", m, (uint32_t)net->weight_alloc_size);
    if(! m){
	debug("no memory for momentums!!\n");
	return -1;
    }
    net->momentums = m;

    for (i = 0; i < net->depth - 1; i++){
	net->interlayers[i].momentums = m;
	m += ALIGNED_SIZE(net->interlayers[i].size);
    }
    return 0;
}

static void
nn_free_momentums(nn_Network_t *net){
    unsigned int i;
    for (i = 0; i < net->depth - 1; i++){
	net->interlayers[i].momentums = NULL;
    }
    free(net->momentums);
    net->momentums = NULL;
}


static int
nn_alloc_deltas(nn_Network_t *net){
    unsigned int i;
    weight_t *m;
    if(net->deltas){
	return 1;
    }
    m = calloc(net->node_alloc_size, sizeof(weight_t));
    if(! m){
	debug("no memory for deltas!!\n");
	return -1;
    }
    net->deltas = m;
    for (i = 0; i < net->depth; i++){
	net->layers[i].deltas = m;
	m += ALIGNED_SIZE(net->layers[i].outsize);
    }
    return 0;
}

static void
nn_free_deltas(nn_Network_t *net){
    unsigned int i;
    for (i = 0; i < net->depth; i++){
	net->layers[i].deltas = NULL;
    }
    free(net->deltas);
    net->deltas = NULL;
}


/************** mean error and weight functions **************/

/* nn_mean_squared_error uses a short cut:
   net->inputs->values is redirected to positions on the actual input array,
   rather than memcpying them into place.
   The proper inputs must be restored later
*/

static inline double
nn_mean_squared_error(nn_Network_t *net,
		      weight_t *inputs,
		      weight_t *targets,
		      int pairs)
{
    double d, diff = 0.0;
    int j, i;
    weight_t *opinion;
    size_t i_size = net->input->outsize;
    size_t o_size = net->output->insize;

    for (j = 0; j < pairs; j++){
	opinion = inline_opinion(net, inputs);

	for (i = 0; i < o_size; i++){
	    d = targets[i] - opinion[i] ;
	    diff += d * d;
	}
	inputs += i_size;
	targets += o_size;
    }
    return diff / (double)(pairs * o_size);
}


inline double
nn_mean_weights_squared(nn_Network_t *net)
{
    double count = 0.0, sum = 0.0;
    int i, j;
    nn_Interlayer_t *il;
    for (i = 0; i < net->depth - 1; i++){
	il = net->interlayers + i;
	count += il->size;
	for (j = 0; j < il->size; j++){
	    sum += il->weights[j] * il->weights[j];
	}
    }
    return sum / count;
}


/**************** Back propagation training ******************/

static void
nn_train_interlayer(nn_Interlayer_t *il, weight_t learn_rate, weight_t momentum){
    /* backpropagate, returning nothing */
    int rows = il->output->insize;
    int cols = il->input->outsize;
    int x, y, z = 0;
    weight_t back_delta;
    weight_t d;
    if (! momentum){
	for (x = 0; x < cols; x++){
	    back_delta = 0.0;
	    for (y = 0; y < rows; y++){
		z = y * cols + x;
		d = (il->output->deltas[y] * il->input->values[x]) * learn_rate;
		il->weights[z] += d;
		back_delta += il->output->deltas[y] * il->weights[z];
	    }
	    il->input->deltas[x] = SLOPE(il->input->values[x]) * back_delta;
	}
    }
    else{
	for (x = 0; x < cols; x++){
	    back_delta = 0.0;
	    for (y = 0; y < rows; y++){
#ifdef CUMULATIVE_MOMENTUM
		z = y * cols + x;
		d = (il->output->deltas[y] * il->input->values[x] + il->momentums[z]) * learn_rate;
		il->weights[z] += d;
		il->momentums[z] = d * momentum;
		back_delta += il->output->deltas[y] * il->weights[z];
#else
		d = (il->output->deltas[y] * il->input->values[x]) * learn_rate;
		il->weights[z] += d + il->momentums[z] * learn_rate;
		il->momentums[z] = d * momentum;
		back_delta += il->output->deltas[y] * il->weights[z];
#endif
	    }
	    il->input->deltas[x] = SLOPE(il->input->values[x]) * back_delta;
	}
    }
}

/* nn_train_set
 see nn_test for examples.*/

double
nn_train_set (nn_Network_t *net, weight_t *inputs, weight_t *targets,
	      int pairs, int count, double learn_rate, double momentum,
	      double target_diff, int print_every)
{
    int i, j, z;
    int insize = net->input->insize;
    int outsize = net->output->insize;
    double diff = 0.0;
    weight_t d = 0.0;

    /*set up deltas */
    if(nn_alloc_deltas(net) < 0){
	debug("error allocating memory for layer deltas");
	return -1;
    }

    /* set up momentums arrays */
    if (momentum){
	if (nn_alloc_momentums(net) < 0){
	    printf("error allocating memory for momentums");
	    return -1;
	}
    }

    for (i = 0;  i < count; i++){
	diff = 0.0;
	for (j = 0; j < pairs; j++){
	    //shortcut that works without bias:
	    //net->input->values = inputs + (insize * j);
	    for (z = 0; z < insize; z++){
	    	net->input->values[z] = inputs[insize * j + z];
	    }

	    for (z = 0; z < net->depth - 1; z++){
		nn_calculate_interlayer(net->interlayers + z);
	    }

	    for (z = 0; z < outsize; z++){
		d = targets[outsize * j + z] - net->output->values[z];
		net->output->deltas[z] = d;
		diff += d * d;
	    }
	    for (z = net->depth - 2; z >= 0; z--){
		nn_train_interlayer(net->interlayers + z, (weight_t)learn_rate, (weight_t)momentum);
	    }
	}
	diff /= (double)pairs;
	if(diff < target_diff){
	    //printf("after %d cycles, diff is %f - good enough\n", i, diff);
	    break;
	}
	if (print_every && !(i % print_every)){
	    printf("after %d cycles, diff is %f\n", i, diff);
	}
    }

    if(net->momentums)
	nn_free_momentums(net);
    if(net->deltas)
	nn_free_deltas(net);

    net->input->values = net->real_inputs;
    return diff;
}




/************** simulated annealing ***********************/



/* nn_alloc_weight_lut creates Look Up Tables for weight mutation
   (used in simulated annealing, best-of-set training). It sticks
   around until the network is freed.
   the LUT avoids the chance of wastefully mutating padding bytes.
 */

static int
nn_alloc_weight_lut(nn_Network_t *net){
    unsigned int i, j, p = 0;
    if (net->weight_lut){
	debug("weight_lut already exists at %p -- not reallocing\n", net->weight_lut);
	return 1;
    }
    net->weight_lut = calloc(net->weight_alloc_size, sizeof(size_t));
    if(! net->weight_lut){
	debug("no memory for weight LUT\n");
	return -1;
    }
    for (i = 0; i < net->depth - 1; i++){
	for (j = 0;  j < net->interlayers[i].size; j++){
	    net->weight_lut[p++] = net->interlayers[i].weights + j - net->weights;
	}
    }
    net->weight_lut_size = p;
    return 0;
}

/* mutates one weight, storing the original values in
   passed in locations */

static inline void
nn_mutate(nn_Network_t *net,
	  weight_t ** location,
	  weight_t *value){
    int wo = nn_rng_uniform_int(net->weight_lut_size);
    weight_t *x = net->weights + net->weight_lut[wo];
    *location = x;
    *value = *x;
    /* multiplying by a uniformly random number between -e and e retains
       scale, but values get stuck near zero, whereas large values can fly
       away.  So: add a constant, and choose from a slightly smaller range.
    */
    double absx = fabs(*x) + 10.0;
    if (absx > NN_WEIGHT_CEILING){
	*x = absx * (nn_rng_uniform_double(4.2) - 2.1);
    }
    else {
	*x = absx * (nn_rng_uniform_double(5.4) - 2.7);
    }
}

static inline void
nn_unmutate(weight_t **location,
	    weight_t *value){
    **location = *value;
}


/* bias_inputs puts biases into an array of raw inputs.
   which makes things easier later.
*/

static weight_t *
bias_inputs(nn_Network_t *net, weight_t *inputs, int n){
    int i;
    int pre = net->input->insize;
    int post = net->input->outsize;
    weight_t *m = malloc(post * n * sizeof(weight_t));
    if (!m){
	return NULL;
    }
    int s = pre * sizeof(weight_t);
    for (i = 0; i < n; i++){
	m[i * post + pre] = 1.0;
	memcpy(m + i * post, inputs + i * pre, s);
    }
    return m;
}


double
nn_anneal_set (nn_Network_t *net, weight_t *inputs, weight_t *targets,
	       int pairs, int count, weight_t target_diff,
	       int print_every, double temperature, double sustain, double min_temp)
{
    int i;
    double current_state, new_state;
    /* variables to store mutated values */
    weight_t *location;
    weight_t *biased = NULL;
    weight_t value;
    if(!net->weight_lut && nn_alloc_weight_lut(net) < 0){
	debug("allocating weight_lut failed!\n");
	return 1e100;
    }

    if(net->bias){
	/*the nn_mean_squared_error short cut depends on inputs being complete with biases */
	biased = bias_inputs(net, inputs, pairs);
	if(!biased){
	    debug("allocating aligned inputs failed!\n");
	    return UINT_MAX;
	}
	inputs = biased;
    }

    current_state = nn_mean_squared_error(net, inputs, targets, pairs);

    for (i = 0;  i < count; i++){
	nn_mutate(net, &location, &value);
	new_state = nn_mean_squared_error(net, inputs, targets, pairs);

	if (new_state < current_state ||
	    (nn_rng_uniform_double(1.0) < exp((current_state - new_state) / temperature))){
	    current_state = new_state;
	}
	else{
	    nn_unmutate(&location, &value);
	}
	if (temperature > min_temp)
	    temperature *= sustain;

	if (current_state < target_diff){
	    //debug("after %d cycles, diff is %f - good enough temp is %f\n", i, current_state, temperature);
	    break;
	}
	if (print_every && !(i % print_every)){
	    printf("after %d cycles, diff is %f, temp is %f, chance of 0.2 upjump %f\n",
		   i, current_state, temperature,
		   exp((-0.2)/ temperature)
		   );
	}
    }
    if(biased)
	free(biased);
    net->input->values = net->real_inputs;
    return current_state;
}


/***************** monte carlo ordering ******************/

/* training to learn the best of a set of items.
   inputs -> an array of input vectors, arranged
     ( a1, b1,[ b1, ..], a2, b2, ... )
     where aX is better than any of bX.
     There is one aX and /n/ - 1 bXs -- they are only
     differentiated by their psoitions in the array.
     There are /sets/ lots of these sets.


 */


static inline unsigned int
nn_best_of_set_error(nn_Network_t *net,
		      weight_t *inputs,
		     int n, int sets)
{
    unsigned int i, j;
    unsigned int err, total = 0;
    weight_t *opinion, top;
    for (j = 0; j < sets; j++){
	err = 0;
	/* find the value of the favoured one */
	top = *inline_opinion(net, inputs);
	/* check whether any others are higher. (note: i starts at 1) */
	for (i = 1; i < n; i++){
	    inputs += net->input->outsize;
	    opinion = inline_opinion(net, inputs);
	    err += (*opinion >= top);
	}
	//printf("top %f, opinion %f err %d\n", top, *opinion, err);
	inputs += net->input->outsize;
#if BEST_OF_SET_SQUARE_ERROR
	total += err * err;
#else
	total += err;
#endif
    }
    return total;
}


#if BEST_OF_SET_SQUARE_ERROR
#define UPDRIFT_CHANCE(net, size, sets, old, new)\
    (nn_rng_uniform_int(((size) * (sets) *(size) * (sets) *	\
			 (size) * (sets) * ((new) - (old)))) < (old))
#else
#define UPDRIFT_CHANCE(net, size, sets, old, new)\
    (nn_rng_uniform_int(((size) * (sets) *(size) * (sets)	\
			 * ((new) - (old)))) < (old))
#endif

#define UPDRIFT_CHANCE2(net, size, sets, old, new)	\
    (nn_rng_uniform_int(((sets)  * (4 + (new) - (old)))) < (old) \
	)


unsigned int
nn_learn_best_of_sets (nn_Network_t *net,
		       weight_t *inputs, //want first one to evaluate higher than any others
		       int size, int sets, int iterations)
{
    int i;
    unsigned int current_state, new_state;
    /* variables to store mutated values */
    weight_t *location;
    weight_t value;
    weight_t *biased = NULL;
    if(!net->weight_lut && nn_alloc_weight_lut(net) < 0){
	debug("allocating weight_lut failed!\n");
	return UINT_MAX;
    }

    if(net->bias){
	/*the nn_mean_squared_error short cut depends on inputs being complete with biases */
	biased = bias_inputs(net, inputs, size * sets);
	if(!biased){
	    debug("allocating aligned inputs failed!\n");
	    return UINT_MAX;
	}
	inputs = biased;
    }

    current_state = nn_best_of_set_error(net, inputs, size, sets);
    //debug("original state %d\n", current_state);

    for (i = 0;  i < iterations; i++){
	nn_mutate(net, &location, &value);
	new_state = nn_best_of_set_error(net, inputs, size, sets);
	if (new_state <= current_state ||
      	    UPDRIFT_CHANCE(net, size, sets, current_state, new_state)){
	    current_state = new_state;
	    if (new_state == 0){
		break; //done it!
	    }
	}
	else{
	    nn_unmutate(&location, &value);
	}
    }
    //debug("finishing after %d iterations \n", i);
    if(biased)
	free(biased);
    net->input->values = net->real_inputs;
    return current_state;
}


unsigned int
nn_learn_best_of_sets_genetic (nn_Network_t *net,
			       weight_t *inputs, //want first one to evaluate higher than any others
			       int size, int sets, int iterations, unsigned int population)
{
    int i, h, j, best;
    unsigned int new_state;
    unsigned int error_sum;
    unsigned int hole = 0;
    /* variables to store mutated values */
    weight_t *location;
    weight_t *biased = NULL;
    weight_t value;
    if(population & 1){
	population++;
	_warn("genetic population should be even (adding 1, now %d)\n", population);
    }
    if(!net->weight_lut && nn_alloc_weight_lut(net) < 0){
	debug("allocating weight_lut failed!\n");
	return UINT_MAX;
    }
    nn_Network_t *p;
    nn_Network_pool_t *pool = malloc(population * (sizeof(nn_Network_pool_t) + net->total_alloc_bytes) + 16);
    if(! pool){
	debug("allocating network pool failed!\n");
	return UINT_MAX;
    }


    void *v = (void *)pool + ALIGNED_BYTES(population * sizeof(nn_Network_pool_t));

    for(i = 0; i < population; i++){
	pool[i].net = (nn_Network_t *)v;
	nn_copy_network_details(net, pool[i].net);
	v += net->total_alloc_bytes;
	pool[i].state = INT_MAX; //mark them all for mutation (they're copies after all)
	pool[i].dead = 0;
    }

    if(net->bias){
	/*the nn_mean_squared_error short cut depends on inputs being complete with biases */
	biased = bias_inputs(net, inputs, size * sets);
	if(!biased){
	    debug("allocating aligned inputs failed!\n");
	    return UINT_MAX;
	}
	inputs = biased;
    }

    for (i = 0;  i < iterations; i++){
	/* 1: try mutations */
	error_sum = 0;
	for (j = 0; j < population; j++){
	    p = pool[j].net;
	    //XXX prefetch not well tested.
	    //__builtin_prefetch(pool + j + 1, 0, 1);
	    nn_mutate(p, &location, &value);
	    new_state = nn_best_of_set_error(p, inputs, size, sets);
	    if (new_state <= pool[j].state ||
		UPDRIFT_CHANCE2(p, size, sets, pool[j].state, new_state)
		){
		pool[j].state = new_state;
		if (pool[j].state == 0){
		    best = j;
		    goto found_best;
		}
	    }
	    else{
		nn_unmutate(&location, &value);
	    }
	    error_sum += pool[j].state;
	    pool[j].error_sum = error_sum;
	}
	/* 2: shoot bad ones */
	for(h = 0; h < population / 2; h++){
	    unsigned int r = nn_rng_uniform_int(error_sum);
	    j = (r > pool[population / 2].error_sum) * (population / 2); /* simple bisecting */
	    for (; j < population; j++){
		if (r < pool[j].error_sum){
		    pool[j].dead = 1;
		    hole = pool[j].state;
		    break;
		}
	    }
	    //take out the dead one's slot
	    for (; j < population; j++){
		pool[j].error_sum -= hole;
	    }
	    error_sum -= hole;
	}

	/*now half are live, half are dead */
	/* 3: copy live ones into dead neighbours */
	j = 0;
	for(h = 0; h < population ; h++){
	    if (pool[h].dead){
		while(pool[j].dead){
		    j++;
		}
		memcpy(pool[h].net->weights, pool[j].net->weights, net->weight_alloc_size * sizeof(weight_t));
		pool[h].state = pool[j].state;
		pool[h].dead = 0;
		j++;
	    }
	}
    }
    best = 0;
    for (j = 1; j < population; j++){
	if (pool[j].state < pool[best].state){
	    best = j;
	}
    }

 found_best:
    new_state = pool[best].state;
    net->input->values = net->real_inputs;
    /* copy the best weights back to the real net */
    memcpy(net->weights, pool[best].net->weights, net->weight_alloc_size * sizeof(weight_t));

    free(pool);
    if (biased)
	free(biased);
    return new_state;
}


/* introduce a certain number of mutations */

void
nn_random_mutations(nn_Network_t *net, int n){
    int i;
    weight_t *location; /*not actually used, but wanted by nn_mutate */
    weight_t value;
    if(!net->weight_lut && nn_alloc_weight_lut(net) < 0){
	debug("allocating weight_lut failed!\n");
	return; //XXX no message
    }
    for (i = 0;  i < n; i++){
	nn_mutate(net, &location, &value);
    }
}

/*evaluator has to return a value >= 0. 0 means perfect. */


unsigned int
nn_learn_generic_genetic (nn_Network_t *net,
			  int (*evaluator)(nn_Network_t *),
			  int iterations, unsigned int population)
{
    int i, h, j, best;
    unsigned int new_state;
    unsigned int error_sum;
    unsigned int hole = 0;
    /* variables to store mutated values */
    weight_t *location;
    weight_t value;
    if(population & 1){
	population++;
	_warn("genetic population should be even (adding 1, now %d)\n", population);
    }
    if(!net->weight_lut && nn_alloc_weight_lut(net) < 0){
	debug("allocating weight_lut failed!\n");
	return UINT_MAX;
    }
    nn_Network_t *p;
    nn_Network_pool_t *pool = malloc(population * (sizeof(nn_Network_pool_t) + net->total_alloc_bytes) + 16);
    if(! pool){
	debug("allocating network pool failed!\n");
	return UINT_MAX;
    }

    void *v = (void *)pool + ALIGNED_BYTES(population * sizeof(nn_Network_pool_t));

    for(i = 0; i < population; i++){
	pool[i].net = (nn_Network_t *)v;
	nn_copy_network_details(net, pool[i].net);
	v += net->total_alloc_bytes;
	pool[i].state = INT_MAX; //mark them all for mutation (they're copies after all)
	pool[i].dead = 0;
    }

    for (i = 0;  i < iterations; i++){
	/* 1: try mutations */
	error_sum = 0;
	for (j = 0; j < population; j++){
	    p = pool[j].net;
	    nn_mutate(p, &location, &value);
	    new_state = evaluator(p);
	    if (new_state <= pool[j].state ||
		nn_rng_uniform_int(new_state) < max(pool[j].state, new_state / 2)
		){
		pool[j].state = new_state;
		if (pool[j].state <= 0){
		    best = j;
		    goto found_best;
		}
	    }
	    else{
		nn_unmutate(&location, &value);
	    }
	    error_sum += pool[j].state;
	    pool[j].error_sum = error_sum;
	}
	/* 2: shoot bad ones */
	for(h = 0; h < population / 2; h++){
	    //debug("error sum: %d\n", error_sum);
	    unsigned int r = nn_rng_uniform_int(error_sum);
	    j = (r > pool[population / 2].error_sum) * (population / 2); /* simple bisecting */
	    for (; j < population; j++){
		if (r < pool[j].error_sum){
		    pool[j].dead = 1;
		    hole = pool[j].state;
		    break;
		}
	    }
	    //take out the dead one's slot
	    for (; j < population; j++){
		pool[j].error_sum -= hole;
	    }
	    error_sum -= hole;
	}

	/*now half are live, half are dead */
	/* 3: copy live ones into dead neighbours */
	j = 0;
	for(h = 0; h < population ; h++){
	    if (pool[h].dead){
		while(pool[j].dead){
		    j++;
		}
		memcpy(pool[h].net->weights, pool[j].net->weights, net->weight_alloc_size * sizeof(weight_t));
		pool[h].state = pool[j].state;
		pool[h].dead = 0;
		j++;
	    }
	}
    }
    best = 0;
    for (j = 1; j < population; j++){
	if (pool[j].state < pool[best].state){
	    best = j;
	}
    }

 found_best:
    new_state = pool[best].state;
    /* copy the best weights back to the real net */
    memcpy(net->weights, pool[best].net->weights, net->weight_alloc_size * sizeof(weight_t));

    free(pool);
    return new_state;
}
