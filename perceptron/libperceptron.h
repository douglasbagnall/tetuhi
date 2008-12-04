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
#ifndef _GOT_LIBPERCEPTRON_H
#define _GOT_LIBPERCEPTRON_H 1

#include <stddef.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>


/*constants */

//arbitrary limit on depth (too lazy to malloc)
#define NN_MAX_LAYERS 5

#define USE_DOUBLE_WEIGHTS 1

#define CUMULATIVE_MOMENTUM 1

#define BEST_OF_SET_SQUARE_ERROR 0

//make random sequences repeat
#define DETERMINISTIC 0

/*options are tanh/nn_detanh, nn_sigmoid/nn_desigmoid */
//#define DEFORM nn_elliot
//#define SLOPE nn_deelliot
#define DEFORM nn_sigmoid
#define SLOPE nn_desigmoid
//#define DEFORM nn_tanh
//#define SLOPE nn_detanh

enum NN_TRAIN_METHOD{
    NN_TRAIN_BACKPROP,
    NN_TRAIN_ANNEAL,
    NN_TRAIN_ADAPTIVE
};

/**************simulated annealing *****************/

// weights higher than this are likely to be offered downscaling jumps
#define NN_WEIGHT_CEILING 1000
#define NN_OVERWEIGHT_REDUCTION 0.95

#define ANNEAL_MUTATE_LUT_SIZE 256

/*********** set weight type (float or double) ************/

#ifdef USE_DOUBLE_WEIGHTS

#define ALIGNED_SIZE(x)  ((x + 1) & ~1)
#define FLOAT_TYPE double
#define TANH tanh
#define EXP  exp
#define FABS fabs

#else

#define ALIGNED_SIZE(x)  ((x + 3) & ~3)
#define FLOAT_TYPE float
#define TANH tanhf
#define EXP  expf
#define FABS fabsf

#endif

#define ALIGNED_BYTES(x)  ((x + 15) & ~15)

typedef FLOAT_TYPE weight_t;
typedef FLOAT_TYPE node_t;


/*************** typedefs for nnc.c ********************/

typedef struct nn_Layer_s{
    unsigned int insize;
    unsigned int outsize; //can be 1 bigger,  to include the bias
    node_t *values;
    node_t *deltas;
} nn_Layer_t;

typedef struct nn_Interlayer_s{
    nn_Layer_t *input;
    nn_Layer_t *output;
    unsigned int size; //length of weights array (input->size * output->size)
    weight_t *weights;
    weight_t *momentums;
} nn_Interlayer_t;

typedef struct nn_Network_s{
    nn_Layer_t *input;
    nn_Layer_t *output;
    nn_Layer_t layers[NN_MAX_LAYERS + 1];
    nn_Interlayer_t interlayers[NN_MAX_LAYERS];
    unsigned int depth;
    weight_t *weights;
    weight_t *nodes;
    weight_t *momentums;
    weight_t *deltas;
    size_t *weight_lut;
    unsigned int weight_lut_size;
    size_t weight_alloc_size;
    size_t node_alloc_size;
    size_t total_alloc_bytes;
    unsigned int bias;
    weight_t *real_inputs;
} nn_Network_t;

typedef struct nn_Network_pool_s{
    nn_Network_t *net;
    unsigned int state;
    unsigned int error_sum;
    unsigned int dead;
} nn_Network_pool_t;


/*************** functionlike macros ****************/


#define debug(format, ...) fprintf (stderr, (format),## __VA_ARGS__); fflush(stderr);
#define _warn debug
#define _error debug
#define debug_lineno() debug("%-25s  line %4d \n", __func__, __LINE__ ); fflush(stderr);

#ifndef max
#define max(a, b) ( ( (a) >= (b) ) ? (a) : (b)  )
#endif
#ifndef min
#define min(a, b) ( ( (a) < (b) ) ? (a) : (b)  )
#endif

/*deformation */

//nn_ prefix for symmetry
#define nn_tanh TANH

static inline weight_t nn_detanh(weight_t x){
    return (1.001 - x * x);
}

static inline weight_t nn_sigmoid(weight_t x){
    if (x < -36.0)
        return 0.0;
    if (x > 36.0)
        return 1.0;
    return 1.0 / (1.0 + EXP(-x));
}

static inline weight_t nn_desigmoid(weight_t x){
    return 0.001 + x * (1.0 - x);
}

static inline weight_t nn_elliot(weight_t x){
    return 0.5 + 0.5 * (x + 1.0) / (1.0 + FABS(x));
}

static inline weight_t nn_deelliot(weight_t x){
    return 1.0 / (2.0 * (1.0 + FABS(x)) * (1.0 + FABS(x)));
}


/******************* function declarations *********************/

/* perceptron.c */
void nn_random_reset(nn_Network_t *net, unsigned int seed);
nn_Network_t *nn_new_network(unsigned int layer_sizes[], int bias);
void nn_delete_network(nn_Network_t *net);
nn_Network_t *nn_duplicate_network(nn_Network_t *orig);
inline void nn_copy_network_details(nn_Network_t *orig, nn_Network_t *copy);
void nn_randomise_weights(nn_Network_t *net, double min_val, double max_val);
inline weight_t *nn_opinion(nn_Network_t *net, weight_t *inputs);
int nn_save_weights(nn_Network_t *net, char *filename);
int nn_dump_weights_formatted(nn_Network_t *net, char *filename);
int nn_load_weights(nn_Network_t *net, char *filename);
int nn_set_single_weight(nn_Network_t *net, int interlayer, int input, int output, weight_t value);
void nn_zero_weights(nn_Network_t *net);


/* training.c */
void nn_rng_init(unsigned int seed);
void nn_rng_maybe_init(unsigned int seed);
int nn_rng_uniform_int(int limit);
double nn_rng_uniform_double(double limit);

inline double nn_mean_weights_squared(nn_Network_t *);
double nn_train_set(nn_Network_t *, weight_t *, weight_t *, int, int, double, double, double, int);
double nn_anneal_set(nn_Network_t *, weight_t *, weight_t *, int, int, weight_t, int, double, double, double);
unsigned int nn_learn_best_of_sets(nn_Network_t *, weight_t *, int, int, int);
unsigned int nn_learn_best_of_sets_genetic(nn_Network_t *, weight_t *, int, int, int, unsigned int);
void nn_random_mutations(nn_Network_t *net, int n);
unsigned int nn_learn_generic_genetic (nn_Network_t *net, int (*evaluator)(nn_Network_t *),
				       int iterations, unsigned int population);




/* debug.c */
void nn_print_weights(nn_Network_t *);
void debug_interlayer(nn_Interlayer_t *);
void debug_network(nn_Network_t *);




#endif
