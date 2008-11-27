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
#define NN_WEIGHT_CEILING 2000
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


/* table of factors by which to multiply while mutating */
/*for no particular reason,
   [ 1.0 + tan(tan((x+0.5) * 0.0054)) for x in range(1, 129) ] #and inverses, so it multiplies out to 1.0
*/
#define MUTATE_LUT_MUL  2.09764391, 2.07778557, 2.05848347, 2.03971047, 2.02144119, \
                        2.00365184, 1.98632012, 1.96942508, 1.95294706, 1.93686755, \
                        1.92116911, 1.90583533, 1.89085071, 1.87620062, 1.86187124, \
                        1.84784949, 1.83412300, 1.82068004, 1.80750950, 1.79460083, \
                        1.78194402, 1.76952954, 1.75734837, 1.74539188, 1.73365187, \
                        1.72212055, 1.71079046, 1.69965450, 1.68870588, 1.67793814, \
                        1.66734508, 1.65692078, 1.64665958, 1.63655605, 1.62660501, \
                        1.61680147, 1.60714065, 1.59761799, 1.58822908, 1.57896970, \
                        1.56983579, 1.56082346, 1.55192896, 1.54314866, 1.53447911, \
                        1.52591695, 1.51745896, 1.50910204, 1.50084317, 1.49267948, \
                        1.48460817, 1.47662654, 1.46873199, 1.46092200, 1.45319414, \
                        1.44554605, 1.43797546, 1.43048016, 1.42305802, 1.41570696, \
                        1.40842498, 1.40121013, 1.39406054, 1.38697436, 1.37994983, \
                        1.37298521, 1.36607884, 1.35922908, 1.35243434, 1.34569310, \
                        1.33900384, 1.33236511, 1.32577549, 1.31923359, 1.31273808, \
                        1.30628762, 1.29988094, 1.29351680, 1.28719396, 1.28091124, \
                        1.27466747, 1.26846151, 1.26229226, 1.25615862, 1.25005953, \
                        1.24399395, 1.23796086, 1.23195927, 1.22598818, 1.22004665, \
                        1.21413373, 1.20824851, 1.20239006, 1.19655751, 1.19074998, \
                        1.18496661, 1.17920656, 1.17346900, 1.16775310, 1.16205807, \
                        1.15638311, 1.15072744, 1.14509029, 1.13947090, 1.13386852, \
                        1.12828242, 1.12271186, 1.11715611, 1.11161447, 1.10608624, \
                        1.10057070, 1.09506717, 1.08957496, 1.08409340, 1.07862181, \
                        1.07315952, 1.06770588, 1.06226021, 1.05682188, 1.05139022, \
                        1.04596459, 1.04054435, 1.03512886, 1.02971748, 1.02430957, \
                        1.01890450, 1.01350164, 1.00810035, 0.99196473, 0.98667823, \
                        0.98144625, 0.97626736, 0.97114016, 0.96606330, 0.96103544, \
                        0.95605531, 0.95112165, 0.94623325, 0.94138893, 0.93658752, \
                        0.93182792, 0.92710901, 0.92242975, 0.91778908, 0.91318599, \
                        0.90861950, 0.90408864, 0.89959246, 0.89513004, 0.89070049, \
                        0.88630292, 0.88193647, 0.87760030, 0.87329358, 0.86901552, \
                        0.86476531, 0.86054219, 0.85634540, 0.85217420, 0.84802785, \
                        0.84390563, 0.83980686, 0.83573083, 0.83167687, 0.82764431, \
                        0.82363250, 0.81964079, 0.81566855, 0.81171515, 0.80777998, \
                        0.80386243, 0.79996190, 0.79607781, 0.79220956, 0.78835660, \
                        0.78451834, 0.78069422, 0.77688370, 0.77308621, 0.76930122, \
                        0.76552819, 0.76176658, 0.75801587, 0.75427552, 0.75054502, \
                        0.74682385, 0.74311149, 0.73940743, 0.73571116, 0.73202217, \
                        0.72833996, 0.72466403, 0.72099386, 0.71732896, 0.71366883, \
                        0.71001297, 0.70636087, 0.70271204, 0.69906597, 0.69542216, \
                        0.69178011, 0.68813930, 0.68449924, 0.68085941, 0.67721931, \
                        0.67357840, 0.66993619, 0.66629213, 0.66264572, 0.65899640, \
                        0.65534366, 0.65168694, 0.64802570, 0.64435939, 0.64068745, \
                        0.63700930, 0.63332438, 0.62963209, 0.62593186, 0.62222308, \
                        0.61850513, 0.61477740, 0.61103926, 0.60729006, 0.60352916, \
                        0.59975587, 0.59596953, 0.59216943, 0.58835487, 0.58452512, \
                        0.58067944, 0.57681707, 0.57293724, 0.56903914, 0.56512196, \
                        0.56118486, 0.55722698, 0.55324744, 0.54924533, 0.54521971, \
                        0.54116962, 0.53709407, 0.53299204, 0.52886248, 0.52470430, \
                        0.52051638, 0.51629757, 0.51204665, 0.50776240, 0.50344352, \
                        0.49908870, 0.49469656, 0.49026566, 0.48579453, 0.48128162



/*
numbers to be added in random mutation
 +/- [ tan(tan(tan((x) * 0.006))) for x in range(1, 129) ]
 (shuffled, in positive/negative pairs, because less bits might be used for add
 ie: only first 64 might matter.
*/

#define MUTATE_LUT_ADD  -4.36024529, 4.36024529, -1.00288142, 1.00288142, -0.41720260, \
                        0.41720260, -2.81008629, 2.81008629, -0.28440746, 0.28440746, \
                        -0.25504548, 0.25504548, -0.31506157, 0.31506157, -0.40797965, \
                        0.40797965, -1.24302597, 1.24302597, -0.18611019, 0.18611019, \
                        -0.04811095, 0.04811095, -0.67529323, 0.67529323, -0.48647251, \
                        0.48647251, -0.15993071, 0.15993071, -0.33903183, 0.33903183, \
                        -0.53048380, 0.53048380, -0.87745442, 0.87745442, -0.16641420, \
                        0.16641420, -0.74006652, 0.74006652, -0.83441247, 0.83441247, \
                        -0.63157706, 0.63157706, -0.05415811, 0.05415811, -0.56611006, \
                        0.56611006, -1.06214765, 1.06214765, -0.13435759, 0.13435759, \
                        -0.17950205, 0.17950205, -0.07847863, 0.07847863, -0.85549916, \
                        0.85549916, -0.72309218, 0.72309218, -0.17293736, 0.17293736, \
                        -0.20621534, 0.20621534, -0.03604674, 0.03604674, -0.09689631, \
                        0.09689631, -0.38117176, 0.38117176, -5.10904220, 5.10904220, \
                        -0.32295212, 0.32295212, -6.20716560, 6.20716560, -1.33315176, \
                        1.33315176, -0.92424607, 0.92424607, -0.01800583, 0.01800583, \
                        -0.38997014, 0.38997014, -0.55396043, 0.55396043, -0.35553713, \
                        0.35553713, -3.81583660, 3.81583660, -0.47601844, 0.47601844, \
                        -0.66030216, 0.66030216, -0.34722913, 0.34722913, -0.54208950, \
                        0.54208950, -0.29956000, 0.29956000, -0.44582159, 0.44582159, \
                        -0.10307691, 0.10307691, -0.46576438, 0.46576438, -0.77578421, \
                        0.77578421, -1.70650586, 1.70650586, -0.60437953, 0.60437953, \
                        -0.01200173, 0.01200173, -0.43611668, 0.43611668, -0.14070022, \
                        0.14070022, -0.42657937, 0.42657937, -0.49713591, 0.49713591, \
                        -0.24788407, 0.24788407, -0.00600022, 0.00600022, -0.37250357, \
                        0.37250357, -0.59130373, 0.59130373, -1.20231556, 1.20231556, \
                        -0.10928065, 0.10928065, -0.29194180, 0.29194180, -1.62973980, \
                        1.62973980, -0.97541678, 0.97541678, -0.21301759, 0.21301759, \
                        -0.51913075, 0.51913075, -0.12176361, 0.12176361, -0.50801849, \
                        0.50801849, -0.27695372, 0.27695372, -0.30726541, 0.30726541, \
                        -0.07237598, 0.07237598, -0.69073657, 0.69073657, -0.75761778, \
                        0.75761778, -0.70665963, 0.70665963, -1.99189775, 1.99189775, \
                        -2.59106040, 2.59106040, -1.09422559, 1.09422559, -0.64573794, \
                        0.64573794, -0.21987373, 0.21987373, -3.07467926, 3.07467926, \
                        -0.02401384, 0.02401384, -0.14707548, 0.14707548, -3.40137057, \
                        3.40137057, -0.19276376, 0.19276376, -1.49607588, 1.49607588, \
                        -0.08459862, 0.08459862, -0.79460760, 0.79460760, -1.88591281, \
                        1.88591281, -0.22678605, 0.22678605, -0.94923925, 0.94923925, \
                        -1.16410631, 1.16410631, -2.24856526, 2.24856526, -0.06628926, \
                        0.06628926, -0.03002703, 0.03002703, -0.19946479, 0.19946479, \
                        -1.38329567, 1.38329567, -0.45570153, 0.45570153, -0.90034530, \
                        0.90034530, -1.55991814, 1.55991814, -0.33094088, 0.33094088, \
                        -0.09073737, 0.09073737, -1.79140394, 1.79140394, -0.26227568, \
                        0.26227568, -0.15348506, 0.15348506, -0.61779768, 0.61779768, \
                        -2.11174626, 2.11174626, -0.23375692, 0.23375692, -0.57855271, \
                        0.57855271, -2.40647505, 2.40647505, -0.06021709, 0.06021709, \
                        -7.97940740, 7.97940740, -0.12804593, 0.12804593, -0.11550904, \
                        0.11550904, -0.26957746, 0.26957746, -1.28652614, 1.28652614, \
                        -1.03174846, 1.03174846, -0.04207427, 0.04207427, -0.81413364, \
                        0.81413364, -0.39890418, 0.39890418, -1.43742066, 1.43742066, \
                        -0.36396036, 0.36396036, -1.12814864, 1.12814864, -0.24078875



#endif
