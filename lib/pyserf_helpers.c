/* This file is part of Te Tuhi Video Game System.
 *
 * Copyright (C) 2008 Douglas Bagnall
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/************** turning python lists into C arrays and vice versa  ****************/

//#include "Python.h"
#include <stddef.h>

#define INDEX_ERROR(format, ...) PyErr_Format(PyExc_IndexError, (format), ## __VA_ARGS__);


static inline int *
int_vector_from_list(int *vector, PyObject *list, int len){
    unsigned int j;
    PyObject *item;
    if (PySequence_Size(list) != len){
	INDEX_ERROR("vector wrong size %d != %d\n", PySequence_Size(list), len);
	return NULL;
    }
    for (j = 0; j < len; j++){
	if (! (item = PySequence_GetItem(list, j))){
	    INDEX_ERROR("could not find %dth item!? ( of %d)\n", j, len);
	    return NULL;
	}
	vector[j] = (int)PyInt_AsLong(item);
	Py_DECREF(item);
    }
    return vector;
}

static inline double *
double_vector_from_list(double *vector, PyObject *list, int len){
    unsigned int j;
    PyObject *item;
    if (PySequence_Size(list) != len){
	INDEX_ERROR("vector wrong size %d != %d\n", PySequence_Size(list), len);
	return NULL;
    }
    for (j = 0; j < len; j++){
	if (! (item = PySequence_GetItem(list, j))){
	    INDEX_ERROR("could not find %dth item!? ( of %d)\n", j, len);
	    return NULL;
	}
	vector[j] = (double)PyFloat_AsDouble(item);
	Py_DECREF(item);
    }
    return vector;
}

static inline float *
float32_vector_from_list(float *vector, PyObject *list, int len){
    unsigned int j;
    PyObject *item;
    if (PySequence_Size(list) != len){
	INDEX_ERROR("vector wrong size %d != %d\n", PySequence_Size(list), len);
	return NULL;
    }
    for (j = 0; j < len; j++){
	if (! (item = PySequence_GetItem(list, j))){
	    INDEX_ERROR("could not find %dth item!? ( of %d)\n", j, len);
	    return NULL;
	}
	vector[j] = (float)PyFloat_AsDouble(item);
	Py_DECREF(item);
    }
    return vector;
}



static inline PyObject *
new_list_from_int_vector(int *a, int len){
    PyObject * list = PyList_New(len);
    int i;
    for (i = 0; i < len; i++){
	PyList_SetItem(list, i, PyInt_FromLong(a[i]));
    }
    return list;
}

static inline PyObject *
new_list_from_uint8_vector(uint8_t *a, int len){
    PyObject * list = PyList_New(len);
    int i;
    for (i = 0; i < len; i++){
	PyList_SetItem(list, i, PyInt_FromLong((long)a[i]));
    }
    return list;
}

static inline PyObject *
new_list_from_double_vector(double *a, int len){
    PyObject *list = PyList_New(len);
    int i;
    for (i = 0; i < len; i++){
	PyList_SetItem(list, i, PyFloat_FromDouble(a[i]));
    }
    return list;
}

static inline PyObject *
new_list_from_float32_vector(float *a, int len){
    PyObject *list = PyList_New(len);
    int i;
    for (i = 0; i < len; i++){
	PyList_SetItem(list, i, PyFloat_FromDouble((double)a[i]));
    }
    return list;
}
