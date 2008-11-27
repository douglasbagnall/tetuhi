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
#include <Python.h>
#include "structmember.h"

#include "../lib/pyserf_helpers.c"
#include "libperceptron.h"




/* headers. */
typedef struct {
    PyObject_HEAD
    nn_Network_t *net;

} Network_object;

/* nnpy.c */
static PyObject *Network___new__(PyTypeObject *, PyObject *, PyObject *);
static void Network___del__(PyObject *);
static PyObject *Network_randomise(PyObject *, PyObject *);
static PyObject *Network_opinion(PyObject *, PyObject *);
static PyObject *Network_train_set(PyObject *, PyObject *, PyObject *);
static PyObject *Network_save_weights(PyObject *, PyObject *);
static PyObject *Network_load_weights(PyObject *, PyObject *);
void initnnpy(void);

#ifdef USE_DOUBLE_WEIGHTS
#define WEIGHT_T_TO_PYLIST new_list_from_double_vector
#define PYLIST_TO_WEIGHT_T double_vector_from_list
#else
#define WEIGHT_T_TO_PYLIST new_list_from_float32_vector
#define PYLIST_TO_WEIGHT_T float32_vector_from_list
#endif

/***** helper functions *****/


static inline void
py_debug(PyObject *o){
    PyObject_Print(o, stderr, 0);
}



/* Network constructor and methods  */

static PyObject *
Network___new__ (PyTypeObject *type, PyObject *args, PyObject *keywds)
{
    PyObject *layersizes;
    Network_object *self;
    unsigned int layersize_v[NN_MAX_LAYERS + 1];
    PyObject *weight_range = NULL;
    PyObject *bias_obj = NULL;
    int bias = 0;
    static char *kwlist[] = {"layer_sizes", "weight_range", "bias", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|OO", kwlist,
				     &layersizes, &weight_range, &bias_obj))
        return NULL;

    if (bias_obj != NULL){
	bias = PyObject_IsTrue(bias_obj);
    }
    int layers = PySequence_Size(layersizes);
    if (layers > NN_MAX_LAYERS){
	PyErr_Format(PyExc_ValueError, "Maximum depth is %d, but got %d layers", NN_MAX_LAYERS, layers);
	return NULL;
    }
    else if (layers < 2){
	PyErr_Format(PyExc_ValueError, "Minimum depth is 2, but got %d layers", layers);
	return NULL;
    }
    if(! int_vector_from_list((int *)layersize_v, layersizes, layers)){
	return NULL;
    }
    layersize_v[layers] = 0; //marks end.

    /**** got the data, now make the object ***/

    self = (Network_object *)(type->tp_alloc(type, 0));
    if (! self){
	PyErr_Format(PyExc_MemoryError, "failed to create object");
	return NULL;
    }
    self->net = nn_new_network(layersize_v, bias);
    if (! self->net){
	Py_XDECREF(self);
	PyErr_Format(PyExc_MemoryError, "failed to create network");
	return NULL;
    }
    /* if given weight range, initialise the net with random values */
    if (weight_range){
	PyObject * wrapped_weights = PyTuple_Pack(1, weight_range);
	if (! Network_randomise((PyObject *)self, wrapped_weights)){
	    Py_XDECREF(self);
	    return NULL;
	}
	Py_XDECREF(wrapped_weights);
    }
    return (PyObject *)self;
}



/* Frees the C structures. But only once!   */

static void
Network___del__ (PyObject *self)
{
    nn_delete_network(((Network_object *)self)->net);
    self->ob_type->tp_free((PyObject*)self);
}

/* reseed the random generator.
   This is accessible both as a module level function and a method.
*/

static PyObject *
Network_seed_random(PyObject *self, PyObject *args)
{
    int seed;
    PyObject *maybe = NULL;
    if (!PyArg_ParseTuple(args, "i|O", &seed, &maybe)){
        return NULL;
    }
    if (maybe && PyObject_IsTrue(maybe))
	nn_rng_maybe_init(seed);
    else
	nn_rng_init(seed);
    return Py_BuildValue("");
}



/* Sets the weights to random values - if reset is non-zero,
 first reseeds the random generator.*/

static PyObject *
Network_randomise (PyObject *self, PyObject *args)
{
    double min_val, max_val;
    int reset = 0;
    nn_Network_t *net = ((Network_object *)self)->net;
    if (!PyArg_ParseTuple(args, "(dd)|i", &min_val, &max_val, &reset)){
	py_debug(args);
	PyErr_Format(PyExc_ValueError, "bad values - should be tuple or list of floats (min, max)");
        return NULL;
    }
    if(reset){
	nn_rng_init(reset);
    }
    if(min_val != 0 && max_val != 0)
	nn_randomise_weights(net, min_val, max_val);
    return Py_BuildValue("");
}


static PyObject *
Network_zero_weights(PyObject *self)
{
    nn_Network_t *net = ((Network_object *)self)->net;
    nn_zero_weights(net);
    return Py_BuildValue("");
}



/* Push some data through the network.
   It can be a list or a bufferable object
   Returns list containing output.   */

static PyObject *
Network_opinion(PyObject *self, PyObject *inputs)
{
    int inputsize = PySequence_Size(inputs);
    nn_Network_t *net = ((Network_object *)self)->net;
    weight_t *data = net->input->values;
    //py_debug(inputs);
    if(PyObject_CheckReadBuffer(inputs)){
	/* if it is a string or array, copy it in exactly. */
	const void *buffer;
	Py_ssize_t len;
	PyObject_AsReadBuffer(inputs, &buffer, &len);
	if (len != net->input->insize * sizeof(weight_t)){
	    debug("input size is %zu, len is %zu\n", net->input->insize * sizeof(weight_t), len);
	    return PyErr_Format(PyExc_ValueError, "wrong input size (got %zu, should be %zu).",
				len, (net->input->insize * sizeof(weight_t)));
	}
	memcpy(data, buffer, len); //re-pointing would not work well with bias.
    }
    else{
	if (inputsize != net->input->insize){
	    return PyErr_Format(PyExc_IndexError, "given %d inputs, wanted %d",
				inputsize, net->input->insize);
	}
	//shift data
	if (! PYLIST_TO_WEIGHT_T(data, inputs, inputsize)){
	    return NULL;
	}
    }
    weight_t *opinion = nn_opinion(net, data);
    return WEIGHT_T_TO_PYLIST(opinion, net->output->insize);
}




static inline int
arrays_from_pyset(PyObject *set, int pairs, int insize, int outsize, weight_t *iv, weight_t *tv)
{
    PyObject *pair;
    PyObject *inputs;
    PyObject *targets;
    int i;

    if (pairs == -1){
	PyErr_Format(PyExc_TypeError, "no length -- non-sequence?");
	return -1;
    }

    for ( i = 0; i < pairs; i++){
	pair = PySequence_GetItem(set, i);
	if (! pair){
	    PyErr_Format(PyExc_IndexError, "couldn't find %dth item", i);
	    return -1;
	}
	inputs = PySequence_GetItem(pair, 0);
	targets = PySequence_GetItem(pair, 1);
	if (! inputs || ! targets){
	    PyErr_Format(PyExc_TypeError, "couldn't parse data");
	    return -1;
	}
	if (insize != PySequence_Size(inputs)){
	    PyErr_Format(PyExc_IndexError, "given %zu inputs, wanted %d", PySequence_Size(inputs), insize);
	    return -1;
	}
	if (outsize != PySequence_Size(targets)){
	    PyErr_Format(PyExc_IndexError, "given %zu targets, wanted %d", PySequence_Size(targets), outsize);
	    return -1;
	}
	if (! PYLIST_TO_WEIGHT_T(iv, inputs, insize) ||
	    ! PYLIST_TO_WEIGHT_T(tv, targets, outsize)){
	    PyErr_Format(PyExc_ValueError, "couldn't coax data into proper form\n");
	    return -1;
	}
	iv += insize;
	tv += outsize;
	Py_DECREF(inputs);
	Py_DECREF(targets);
	Py_DECREF(pair);
    }
    return 0;
}



/* Train with a set of data, a number of times. Set should be   */
/* sequence of (inputs, targets) pairs.   */


static PyObject *
Network_train_set (PyObject *self, PyObject *args, PyObject *keywds)
{
    PyObject *set;
    int count;
    double threshold = 0.0;
    double learn_rate = 0.2;
    double momentum = 0.0;
    int print_every = 0;

    nn_Network_t *net = ((Network_object *)self)->net;
    int insize = net->input->insize;
    int outsize = net->output->insize;
    weight_t *vectors, *input_v, *target_v;
    int pairs;
    double diff;

    static char *kwlist[] = {"set", "count", "threshold", "learn_rate", "momentum",
			     "print_every", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "Oi|dddi", kwlist, &set, &count, &threshold,
				     &learn_rate, &momentum, &print_every))
        return NULL;

    pairs = PySequence_Size(set);
    debug("getting %u bytes\n", (uint32_t)((insize + outsize) * pairs * sizeof(weight_t)));
    vectors = malloc((insize + outsize) * pairs * sizeof(weight_t));
    if (!vectors)
	return PyErr_NoMemory();
    input_v = vectors;
    target_v = vectors + insize * pairs;

    if (arrays_from_pyset(set, pairs, insize, outsize,
			  input_v, target_v)){
	free(vectors);
	return NULL;
    }
    /* Now there are two vectors in the format expected by nn_train_set. */

    diff = nn_train_set (net, input_v, target_v, pairs,
			 count, learn_rate, momentum, threshold, print_every);
    free(vectors);
    return Py_BuildValue("d", diff);
}

static PyObject *
Network_anneal_set (PyObject *self, PyObject *args, PyObject *keywds)
{
    PyObject *set;
    int count;
    double threshold = 0.0;
    double temperature = 55.0;
    double sustain = 0.9992;
    double min_temp = 0.0001;
    int print_every;
    double diff;
    nn_Network_t *net = ((Network_object *)self)->net;
    int insize = net->input->insize;
    int outsize = net->output->insize;
    weight_t *vectors, *input_v, *target_v;
    int pairs;

    static char *kwlist[] = {"set", "count", "threshold", "temperature", "sustain",  "min_temp",
			     "print_every", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "Oi|ddddi", kwlist, &set, &count, &threshold,
				     &temperature, &sustain, &min_temp, &print_every))
        return NULL;

    pairs = PySequence_Size(set);
    vectors = malloc((insize + outsize) * pairs * sizeof(weight_t));
    if (!vectors)
	return PyErr_NoMemory();
    input_v = vectors;
    target_v = vectors + insize * pairs;
    if (arrays_from_pyset(set, pairs, insize, outsize,
			  input_v, target_v)){
	free(vectors);
	return NULL;
    }
    /* Now there are two vectors in the format expected by nn_train_set. */

    diff = nn_anneal_set(net, input_v, target_v, pairs,
			 count, threshold, print_every,
			 temperature, sustain, min_temp);
    free(vectors);
    return Py_BuildValue("d", diff);
}






/* Network_best_of_set
   takes a sequence of sequences of sequences of PyFloats
   or
   a sequence of array.array objects
   each array.array represents n input vectors -- where n can be calculated by
   len(array.array obj) /  net->input->insize

 */


static PyObject *
Network_best_of_set (PyObject *self, PyObject *args, PyObject *keywds)
{
    PyObject *set, *group, *inputs;
    int i, j;
    int count;
    int population = 1;
    int diff;
    nn_Network_t *net = ((Network_object *)self)->net;
    int insize = net->input->insize;
    weight_t *input_v, *iv;
    int groups;
    unsigned int groupsize = 0; //size of each group
    unsigned int bytesize = 0;
    const void *buffer;
    Py_ssize_t len;


    static char *kwlist[] = {"set", "count", "population", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "Oi|i", kwlist, &set, &count, &population))
        return NULL;


    groups = PySequence_Size(set);
    if (groups == -1){
	return PyErr_Format(PyExc_TypeError, "no length -- non-sequence?");
    }
    /* get the size of the first one, for allocing purposes. */
    group = PySequence_ITEM(set, 0);
    if (! group)
	return PyErr_Format(PyExc_IndexError, "couldn't find first item, giving up.");

    if(PyObject_CheckReadBuffer(group)){ /*buffer */
	PyObject_AsReadBuffer(group, &buffer, &len);
	bytesize = len;
	groupsize = bytesize / (sizeof(weight_t) * insize);
    }
    else{
	groupsize = PySequence_Size(group);
	bytesize = groupsize * sizeof(weight_t) * insize;
    }

    input_v = malloc(insize * groups * bytesize);
    if (!input_v)
	return PyErr_NoMemory();
    iv = input_v;
    //XXX could be pre-biasing here

    for (i = 0; i < groups; i++){
	if (i)//already have first one
	    group = PySequence_ITEM(set, i);

	if(PyObject_CheckReadBuffer(group)){ /*buffer */
	    if (PyObject_AsReadBuffer(group, &buffer, &len) < 0
		|| len != bytesize){
		Py_DECREF(group);
		free(input_v);
		return PyErr_Format(PyExc_ValueError, "impossible thing happened, "
				    "or vector size is wrong (%zu instead of %d)",
				    len, bytesize);
	    }
	    memcpy(iv, buffer, bytesize);
	    iv += groupsize * insize;
	}
	else{ /*list (of lists of weight_t; or of buffers)*/
	    if (groupsize != PySequence_Size(group)){
		PyErr_Format(PyExc_ValueError, "group %d is the wrong size( %zu, should be %d)",
			     i, PySequence_Size(group), groupsize);
		goto outer_error;
	    }
	    for (j = 0; j < groupsize; j++){
		inputs = PySequence_ITEM(group, j);
		if (! inputs){
		    PyErr_Format(PyExc_ValueError, "improper inputs");
		    goto inner_error;
		}
		if(PyObject_CheckReadBuffer(inputs)){ /*buffer */
		    if (PyObject_AsReadBuffer(inputs, &buffer, &len) < 0 || len != insize * sizeof(weight_t)){
			PyErr_Format(PyExc_ValueError, "input vector size is wrong (%zu instead of %u)",
				     len, (uint32_t)(insize * sizeof(weight_t)));
			goto inner_error;
		    }
		    memcpy(iv, buffer, len);
		}
		if (insize != PySequence_Size(inputs)){
		    PyErr_Format(PyExc_IndexError, "wrong size (%zu inputs, wanted %d)", PySequence_Size(inputs), insize);
		    goto inner_error;
		}

		else if (! PYLIST_TO_WEIGHT_T(iv, inputs, insize)){
		    PyErr_Format(PyExc_ValueError, "couldn't coax data into proper form\n");
		    goto inner_error;
		}
		iv += insize;
		Py_DECREF(inputs);
	    }
	}
	Py_DECREF(group);
    }

    if (population < 2){
	diff = nn_learn_best_of_sets(net, input_v, groupsize, groups, count);
    }
    else{
	diff = nn_learn_best_of_sets_genetic(net, input_v, groupsize, groups, count, population);
    }
    return Py_BuildValue("i", diff);

 inner_error:
    Py_DECREF(inputs);
 outer_error:
    Py_DECREF(group);
    free(input_v);
    return PyErr_NoMemory();
}



static PyObject *
Network_generic_genetic (PyObject *self, PyObject *args, PyObject *keywds)
{
    PyObject *py_evaluator;
    int count;
    int population;
    unsigned int diff;
    nn_Network_t *net = ((Network_object *)self)->net;

    static char *kwlist[] = {"evaluator", "count", "population", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "Oii", kwlist, &py_evaluator, &count, &population))
        return NULL;

    //nested function to wrap py function (GCC specific?)
    int evaluator(nn_Network_t *p){
	((Network_object *)self)->net = p;
	PyObject *r = PyObject_CallFunctionObjArgs(py_evaluator, self, NULL);
	((Network_object *)self)->net = net;
	return PyInt_AsLong(r);
    }

    diff = nn_learn_generic_genetic(net, evaluator, count, population);
    if (diff == UINT_MAX){
	// a malloc error
	return PyErr_NoMemory();
    }

    return PyInt_FromLong(diff);
}



/************************i/o ****************/

/* saves the weight arrays to disk   */

static PyObject *
Network_save_weights (PyObject *self, PyObject *args)
{
    char *filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;

    if (nn_save_weights(((Network_object *)self)->net, filename) < 0){
	return PyErr_Format(PyExc_IOError, "error saving network to %s",
			    filename);
    }
    return Py_BuildValue("");
}


/* loads weights from disk  */

static PyObject *
Network_load_weights (PyObject *self, PyObject *args)
{
    char *filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;
    if (nn_load_weights(((Network_object *)self)->net, filename) < 0){
	return PyErr_Format(PyExc_IOError, "error loading network from %s",
			    filename);
    }
    return Py_BuildValue("");
}


static PyObject *
Network_dump_weights_formatted(PyObject *self, PyObject *args)
{
    char *filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;

    if (nn_dump_weights_formatted(((Network_object *)self)->net, filename) < 0){
	return PyErr_Format(PyExc_IOError, "error saving network to %s",
			    filename);
    }
    return Py_BuildValue("");
}

/*--------*/

/* setting individual weights. useful for designed networks. */

static PyObject *
Network_set_single_weight(PyObject *self, PyObject *args)
{
    int interlayer;
    int input;
    int output;
    double value;
    if (!PyArg_ParseTuple(args, "iiid", &interlayer, &input, &output, &value))
        return NULL;
    //nn_set_single_weight returns -1 on failure, 0 on success
    if (nn_set_single_weight(((Network_object *)self)->net, interlayer, input, output, value))
	return PyErr_Format(PyExc_RuntimeError, "couldn't set weight il: %d in:%d out:%d to %d.%d\n",
			    interlayer, input, output, (int)value, (int)(1000 * (value - trunc(value))));
    return Py_BuildValue("");
}


/***** getting and setting weights ************/
/* weights are accessed and set as a string attribute */

static PyObject *
Network_get_weights (PyObject *self, void *context)
{
    nn_Network_t *net = ((Network_object *)self)->net;
    return PyString_FromStringAndSize((char *)(net->weights), net->weight_alloc_size * sizeof(weight_t));
}


static int
Network_set_weights (PyObject *self, PyObject *obj, void *context)
{
    nn_Network_t *net = ((Network_object *)self)->net;
    char *weights;
    Py_ssize_t len = 1;
    if (!obj) {
	PyErr_SetString(PyExc_TypeError, "eh?");
	return -1;
    }
    if(PyObject_AsReadBuffer(obj, (const void **)&weights, &len) < 0){
	return -1;
    }

    //debug("py length is %d, c length is %u\n", len, (uint32_t)(net->weight_alloc_size * sizeof(weight_t)));
    memcpy(net->weights, weights, net->weight_alloc_size * sizeof(weight_t));
    return 0;
}



/* shape attribute - read only
   returns a list reflecting list the shape of the network */

static PyObject *
Network_get_shape(PyObject *self, void* context)
{
    int i;
    nn_Network_t *net = ((Network_object *)self)->net;
    int shape[NN_MAX_LAYERS];
    for (i = 0; i < net->depth; i++){
	shape[i] = net->layers[i].insize;
    }
    return new_list_from_int_vector(shape, net->depth);
}

/* weight_size attribute - read only
   returns sizeof(weight_t) */

static PyObject *
Network_get_weight_size(PyObject *self, void* context)
{
    return PyInt_FromLong(sizeof(weight_t));
}

/* __reduce__() -> information suitable for pickling
   (see http://docs.python.org/lib/pickle-protocol.html).
 */
static PyObject *
Network___reduce__(PyObject *self)
{
    //debug("in reduce for object %p\n", self);
    nn_Network_t *net = ((Network_object *)self)->net;
    /* get shape and weights */
    PyObject *shape = Network_get_shape(self, NULL);
    PyObject *weights = Network_get_weights(self, NULL);
    /* get args for __new__ constructor */
    PyObject *weight_range = Py_BuildValue("(ii)", 0, 0);
    PyObject *bias = Py_BuildValue("i", net->bias);
    PyObject *args = Py_BuildValue("(NNN)", shape, weight_range, bias);

    return Py_BuildValue("ONN", self->ob_type, args, weights);
}

/* __setstate__() --> nothing more than setting weights.
 */
static PyObject *
Network___setstate__(PyObject *self, PyObject *obj)
{
    Network_set_weights(self, obj, NULL);
    return Py_BuildValue("");
}



/**** top level test and utility functions ***/

static PyObject *
test_buffer(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
        return NULL;

    py_debug(obj);

    if(! PyObject_CheckReadBuffer(obj)){
	return PyErr_Format(PyExc_TypeError, "not a bufferable object");
    }
    /*given list of array.arrays */
    debug("using buffer interface\n");
    const void *buffer;
    Py_ssize_t len;
    if (PyObject_AsReadBuffer(obj, &buffer, &len)){//never never should fail?
	return PyErr_Format(PyExc_ValueError, "impossible thing happened");
    }
    void *a = alloca(len);
    memcpy(a, buffer, len);
    return WEIGHT_T_TO_PYLIST(a, len/sizeof(weight_t));
}

/* mutate weights */

static PyObject *
Network_random_mutations (PyObject *self, PyObject *args)
{
    int number;
    if (!PyArg_ParseTuple(args, "i", &number))
        return NULL;
    nn_random_mutations(((Network_object *)self)->net, number);
    return Py_BuildValue("");
}





/* method binding structs                                             */
static PyMethodDef top_level_functions[] = {
    {"test_buffer", (PyCFunction)test_buffer, METH_VARARGS,
     "tries turning a buffer into a list"},
    {"seed_random", (PyCFunction)Network_seed_random, METH_VARARGS,
     "Seed the random number generator"},
    {NULL}
};

static PyMethodDef Network_methods[] = {
    {"randomise", (PyCFunction)Network_randomise, METH_VARARGS,
     "Sets the weights to random values "},
    {"seed_random", (PyCFunction)Network_seed_random, METH_VARARGS,
     "Seed the random number generator"},
    {"zero_weights", (PyCFunction)Network_zero_weights, METH_NOARGS,
     "set all weights for zero. probably you want to follow this with a series"\
     " of calls to set_single_weight."},
    {"opinion", (PyCFunction)Network_opinion, METH_O,
     "Push some data through the network. Returns list containing "\
     "output. "},
    {"train_set", (PyCFunction)Network_train_set, METH_VARARGS | METH_KEYWORDS,
     "Train with a set of data, a number of times, using back-propagation."\
     " The set should be a sequence of (inputs, targets) pairs. "},
    {"anneal_set", (PyCFunction)Network_anneal_set, METH_VARARGS | METH_KEYWORDS,
     "Train with a set of data, a number of times, using simulated annealing."\
     " Set should be sequence of (inputs, targets) pairs. "},
    {"best_of_set", (PyCFunction)Network_best_of_set, METH_VARARGS | METH_KEYWORDS,
     "Train the network to find the best item of a number of sets."\
     " Set should be sequence of [best inputs, lesser inputs, lesser inputs,... ]. "},
    {"generic_genetic", (PyCFunction)Network_generic_genetic, METH_VARARGS | METH_KEYWORDS,
     "Genetic learning with the passed in fitness function. "\
     "net.generic_genetic(function, interations, population)."},
    {"set_single_weight", (PyCFunction)Network_set_single_weight, METH_VARARGS,
     "Set a single weight, indexed by weight layer, input, output. bias weight is"\
     "tacked onto end of inputs."},
    {"save_weights", (PyCFunction)Network_save_weights, METH_VARARGS,
     "saves the weight arrays to disk "},
    {"dump_weights", (PyCFunction)Network_dump_weights_formatted, METH_VARARGS,
     "save the weight arrays to disk in a human readable form"},
    {"load_weights", (PyCFunction)Network_load_weights, METH_VARARGS,
     "loads weights from disk."},
    {"random_mutations", (PyCFunction)Network_random_mutations, METH_VARARGS,
     "mutate the thing randomly."},
    {"__reduce__", (PyCFunction)Network___reduce__, METH_NOARGS,
     "for pickling."},
    {"__setstate__", (PyCFunction)Network___setstate__, METH_O,
     "for unpickling. x.__setstate__(w) is equivalent to x.weights = w"},
    {NULL}
};


static PyGetSetDef Network_getsets[] = {
    {"shape", (getter)Network_get_shape,
     NULL, "", NULL},
    {"weights", (getter)Network_get_weights,
     (setter)Network_set_weights, "", NULL},
    {"weight_size", (getter)Network_get_weight_size,
     NULL, "", NULL},
    {NULL}

};


/* type definition structs                                            */
static PyTypeObject Network_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                             /*          ob_size */
    "nnpy.Network",                                /*          tp_name */
    sizeof(Network_object),                   /*     tp_basicsize */
    0,                                             /*      tp_itemsize */
    (destructor)Network___del__,                   /*       tp_dealloc */
    0,                                             /*         tp_print */
    0,                                             /*       tp_getattr */
    0,                                             /*       tp_setattr */
    0,                                             /*       tp_compare */
    0,                                             /*          tp_repr */
    0,                                             /*     tp_as_number */
    0,                                             /*   tp_as_sequence */
    0,                                             /*    tp_as_mapping */
    0,                                             /*          tp_hash */
    0,                                             /*          tp_call */
    0,                                             /*           tp_str */
    0,                                             /*      tp_getattro */
    0,                                             /*      tp_setattro */
    0,                                             /*     tp_as_buffer */
    (long)Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*         tp_flags */
    "Wraps perceptron in a pythonic object",       /*           tp_doc */
    0,                                             /*      tp_traverse */
    0,                                             /*         tp_clear */
    0,                                             /*   tp_richcompare */
    0,                                             /*tp_weaklistoffset */
    0,                                             /*          tp_iter */
    0,                                             /*      tp_iternext */
    (struct PyMethodDef *)Network_methods,         /*       tp_methods */
    0,                                             /*       tp_members */
    (struct PyGetSetDef *)Network_getsets,         /*        tp_getset */
    0,                                             /*          tp_base */
    0,                                             /*          tp_dict */
    0,                                             /*     tp_descr_get */
    0,                                             /*     tp_descr_set */
    0,                                             /*    tp_dictoffset */
    0,                                             /*          tp_init */
    0,                                             /*         tp_alloc */
    (newfunc)Network___new__,                      /*           tp_new */
    0,                                             /*          tp_free */
};

/* initialisation.                                                    */
#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initnnpy(void)
{
    PyObject* m;
    m = Py_InitModule3("nnpy", top_level_functions,
        "None");

    if (m == NULL)
        return;

    if (PyType_Ready(&Network_type) < 0)
        return;

    Py_INCREF(&Network_type);
    PyModule_AddObject(m, "Network", (PyObject *)&Network_type);
    /* initialise the RNG */
    nn_rng_init(-1);
}
