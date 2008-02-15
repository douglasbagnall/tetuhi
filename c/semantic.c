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
#include <Python.h>
#include "structmember.h"
#include <string.h>

#include "libgamemap.h"
#include "../lib/pyserf_helpers.c"

/* headers. */

typedef struct {
    PyObject_HEAD
    tt_gamemap_t *map;
    int clumpiness;
    int n_bits;
} Gamemap_object;

static PyObject *Gamemap_region_contents (Gamemap_object *, PyObject*);
static PyObject *Gamemap_add_element (Gamemap_object *, PyObject*);
static PyObject *Gamemap_clear_figure (Gamemap_object *, PyObject*);
static PyObject *Gamemap_set_figure (Gamemap_object *, PyObject*);


#define mapdoc "The constructor takes two arguments, the width and height of the map"

/* functions and methods  */
static PyObject *
Gamemap___new__ (PyTypeObject *type, PyObject *args, PyObject *keywds)
{
    unsigned int height;
    unsigned int width;
    unsigned int block_size = 1;
    Gamemap_object *self;

    if (!PyArg_ParseTuple(args, "ii|i", &width, &height, &block_size))
        return NULL;

    self = (Gamemap_object *)(type->tp_alloc(type, 0));
    if (! self){
	PyErr_Format(PyExc_MemoryError, "failed to create object");
	return NULL;
    }
    self->clumpiness = 0;
    self->n_bits = 0;
    while (block_size > 1){
	block_size >>= 1;
	self->clumpiness++;
    }
    width >>= self->clumpiness;
    height >>= self->clumpiness;
    debug("set clumpiness to %d\n", self->clumpiness);
    self->map = tt_new_map(width, height);
    if (! self->map){
	Py_XDECREF(self);
	PyErr_Format(PyExc_MemoryError, "failed to create map");/*not necesarily memory error*/
	return NULL;
    }
    return (PyObject *)self;
}

/* Frees the C structures. But only once!   */

static void
Gamemap___del__ (PyObject *self)
{
    tt_delete_map(((Gamemap_object *)self)->map);
    self->ob_type->tp_free((PyObject*)self);
}


/* Gamemap_region_contents (binds to Gamemap.region_contents) */

#define region_contents_doc \
    "Find the things enclosed by the convex hull surrounding a\n"\
    "set of points. The points need to be in a flattened sequence\n" \
    "(ie: [x1,y1, x2,y2, ...]).\n"\
    "The returned integer is a bit map of the things present\n"

static PyObject *
Gamemap_region_contents(Gamemap_object *self, PyObject *args)
{
    PyObject *py_points;
    tt_point_t points[POINTS_MAX];
    bitmap_t found;
    int n_points, i;
    if (!PyArg_ParseTuple(args, "O", &py_points))
        return NULL;

    n_points = PySequence_Size(py_points) / 2;
    if (n_points > POINTS_MAX)
	return PyErr_Format(PyExc_ValueError, "too many points!");

    if(! int_vector_from_list((int *)points, py_points, n_points * 2)){
	return NULL;
    }
    for (i = 0; i < n_points; i++){
	points[i].x >>= self->clumpiness;
	points[i].y >>= self->clumpiness;
    }

    found = tt_search_region(self->map, points, n_points);
    //debug("found %d\n", found);
    return Py_BuildValue("i", (int)found);
}


#define scan_zones_doc \
    "Look for elements in a partially predefined pattern of regions \n"\
    "around the given rectangle.  The rectangle is defined in PyGame  \n"\
    "style (x, y, width, height). Arguments are:"\
    "rectangle   as explained\n"\
    "zones       a sequence of integers determining the size and number of \n"\
    "            scanned regions\n"\
    "[mode]      a string ('string', 'list', 'lists', 'numbers'). how  \n"\
    "            python gets the data. [default: 'numbers']\n"\
    "[orientation] which way is up (clockwise from 0-7, 0 is natural)\n"\
    "[verbose]\n"	\
    "otherwise not).\n"


static PyObject *
Gamemap_scan_zones (Gamemap_object *self, PyObject *args)
{
    PyObject *py_rect;
    PyObject *py_zones;
    const char *mode = "numbers";
    int orientation = 0;
    int rect[4];
    int zones[ZONES_MAX];
    unsigned int n_zones;
    bitmap_t finds[ZONES_MAX * ZONES_SEGMENTS];
    int verbose = 0;
    int i;
    if (!PyArg_ParseTuple(args, "OO|siii", &py_rect, &py_zones, &mode, &orientation, &verbose) ||
	PySequence_Size(py_rect) != 4){
	return NULL;
    }
    n_zones = PySequence_Size(py_zones);
    if (n_zones > ZONES_MAX ||
	! int_vector_from_list(rect, py_rect, 4) ||
	! int_vector_from_list(zones, py_zones, n_zones)){
	return PyErr_Format(PyExc_ValueError, "too many zones!");
    }
    /*round down to semantic size */
    for (i = 0; i < 4; i++){
	rect[i] >>= self->clumpiness;
    }
    for (i = 0; i < n_zones; i++){
	zones[i] >>= self->clumpiness;
    }

    tt_scan_zones(self->map, finds, rect, zones, n_zones, orientation, verbose);
    /* the return format can vary */
    size_t len = (n_zones - 1) * ZONES_SEGMENTS;

    if (strcmp(mode, "string") == 0){
	return Py_BuildValue("s#", finds, len);
    }
    if (strcmp(mode, "list") == 0){
	PyObject *list = PyList_New(len * self->n_bits);
	int i, bit;
	for (bit = 0; bit < self->n_bits; bit++){
	    for (i = 0; i < len; i++){
		int x = (finds[i] >> bit) & 1;
		PyList_SetItem(list, bit * len + i, PyInt_FromLong(x));
	    }
	}
	return list;
    }
    if (strcmp(mode, "lists") == 0){
	PyObject *list = PyList_New(self->n_bits);
	int i, bit;
	for (bit = 0; bit < self->n_bits; bit++){
	    PyObject *sublist = PyList_New(len);
	    for (i = 0; i < len; i++){
		int x = (finds[i] >> bit) & 1;
		PyList_SetItem(sublist, i, PyInt_FromLong(x));
	    }
	    PyList_SetItem(list, bit, sublist);
	}
	return list;
    }

    // "numbers"
    return new_list_from_uint8_vector(finds, len);

}




/* Gamemap_add_element (binds to Gamemap.add_element) */

static PyObject *
Gamemap_add_element (Gamemap_object *self, PyObject *args)
{
    int ID;
    int width;
    int height;
    int bit;
    char *bitmap;
    int length;
    /*element needs
      width, height
      bitmap --  a string, one byte per pixel, non zero means 1.
    */

    if (!PyArg_ParseTuple(args, "iiis#", &width, &height, &bit, &bitmap, &length))
        return NULL;
    if (length != width * height){
	return PyErr_Format(PyExc_ValueError, "width %d, height %d doesn't match string length %d\n",
			    width, height, length);
    }

    ID = tt_add_element(self->map, bitmap, bit, width, height, BITMAP_OF_BYTES, self->clumpiness);

    /*save number of bits used for returning in scan list. a skipped bit is counted*/
    self->n_bits = max(self->n_bits, bit + 1);

    return Py_BuildValue("i", ID);
}

/* Gamemap_clear_figure (binds to Gamemap.clear_figure) */

static PyObject *
Gamemap_clear_figure (Gamemap_object *self, PyObject *args)
{
    int ID;
    int x;
    int y;
    if (!PyArg_ParseTuple(args, "iii", &ID, &x, &y))
        return NULL;
    x >>= self->clumpiness;
    y >>= self->clumpiness;
	tt_clear_figure(self->map, ID, x, y);
    return Py_BuildValue("");
}

static PyObject *
Gamemap_set_figure (Gamemap_object *self, PyObject *args)
{
    int ID;
    int x;
    int y;
    if (!PyArg_ParseTuple(args, "iii", &ID, &x, &y))
        return NULL;
    x >>= self->clumpiness;
    y >>= self->clumpiness;
    tt_set_figure(self->map, ID, x, y);
    return Py_BuildValue("");
}

#define touching_figure_doc "find the bits that this figure overlaps"

static PyObject *
Gamemap_touching_figure(Gamemap_object *self, PyObject *args)
{
    int ID;
    int x;
    int y;
    if (!PyArg_ParseTuple(args, "iii", &ID, &x, &y))
        return NULL;
    x >>= self->clumpiness;
    y >>= self->clumpiness;
    bitmap_t touching = tt_touching_figure(self->map, ID, x, y);
    return Py_BuildValue("i", touching);
}


/* get the value of a single point in the map */
static PyObject *
Gamemap_get_point (Gamemap_object *self, PyObject *args)
{
    int answer;
    int x;
    int y;
    int preclumped = 0;
    tt_gamemap_t * gm = self->map;
    if (!PyArg_ParseTuple(args, "ii|i", &x, &y, &preclumped))
        return NULL;
    if (! preclumped){
	x >>= self->clumpiness;
	y >>= self->clumpiness;
    }
    if (x < 0 || y < 0 || x >= gm->width || y >= gm->height)
	return Py_BuildValue("i", BITMAP_EDGE_MASK);
    answer = (int)gm->map[y * gm->padded_width + x];
    return Py_BuildValue("i", answer);
}


static PyObject *
Gamemap_save_map (Gamemap_object *self, PyObject *args)
{
    char * filename;
    int padding = 0;
    int format = IMAGE_FORMAT_PGM;
    if (!PyArg_ParseTuple(args, "s|ii", &filename, &padding, &format))
        return NULL;
    tt_save_map(self->map, filename, format, padding);
    return Py_BuildValue("");
}

/*clear the map */
static PyObject *
Gamemap_clear (Gamemap_object *self)
{
    tt_clear_map(self->map);
    return Py_BuildValue("");
}

/* get the map as a python string */
static PyObject *
Gamemap_as_buffer (PyObject *self)
{
    tt_gamemap_t *gm = ((Gamemap_object *)self)->map;
    char *buffer = (char *)gm->map;
    size_t size = gm->padded_width * gm->height * sizeof(bitmap_t);
    return Py_BuildValue("s#", buffer, size);
}


static PyObject *
Gamemap_prescan_clear (PyObject *self)
{
/* do nothing if prescan is not used */
#if PRESCAN
    tt_gamemap_t *gm = ((Gamemap_object *)self)->map;
    tt_prescan_clear(gm);
#endif
    return Py_BuildValue("");
}

/*print options used in compilation */
static PyObject *
Gamemap_print_options(PyObject *self)
{
    tt_print_options();
    return Py_BuildValue("");
}




/*attribute access (different signature!)*/

static PyObject *
Gamemap_get_size(Gamemap_object *self, void* context)
{
    tt_gamemap_t *gm = self->map;
    return Py_BuildValue("(ii)", gm->width << self->clumpiness,
			 gm->height << self->clumpiness);
}

/*clumpiness */
static PyObject *
Gamemap_get_clumpiness(Gamemap_object *self, void* context)
{
    return Py_BuildValue("i", self->clumpiness);
}

/*edge bit mask*/

static PyObject *
Gamemap_get_edge_bit(PyObject *self, void* context)
{
    return Py_BuildValue("i", BITMAP_EDGE_BIT);
}

/*all bits *except* edge bit mask. would be nice to be able to do this
  better.*/
static PyObject *
Gamemap_get_bits(PyObject *self, void* context)
{
#if BITMAP_EDGE_BIT == 0
    return Py_BuildValue("iiiiiii", 1, 2, 3, 4, 5, 6, 7);
#else
#error need to change things in python to reflect new bitmap edge bit
#endif
}



/**********************************************************************/
/* method binding structs                                             */
/**********************************************************************/
/* bindings for top_level */
static PyMethodDef top_level_functions[] = {
    {NULL}
};

/* bindings for Gamemap */
static PyMethodDef Gamemap_methods[] = {
    {"region_contents", (PyCFunction)Gamemap_region_contents, METH_VARARGS,
    region_contents_doc},
    {"scan_zones", (PyCFunction)Gamemap_scan_zones, METH_VARARGS,
    scan_zones_doc},
    {"add_element", (PyCFunction)Gamemap_add_element, METH_VARARGS,
    ""},
    {"clear_figure", (PyCFunction)Gamemap_clear_figure, METH_VARARGS,
    ""},
    {"set_figure", (PyCFunction)Gamemap_set_figure, METH_VARARGS,
    ""},
    {"touching_figure", (PyCFunction)Gamemap_touching_figure, METH_VARARGS,
    touching_figure_doc},
    {"get_point", (PyCFunction)Gamemap_get_point, METH_VARARGS,
    "get a single point of the map"},
    {"save_map", (PyCFunction)Gamemap_save_map, METH_VARARGS,
    ""},
    {"as_buffer", (PyCFunction)Gamemap_as_buffer, METH_NOARGS,
    ""},
    {"prescan_clear", (PyCFunction)Gamemap_prescan_clear, METH_NOARGS,
    ""},
    {"print_options", (PyCFunction)Gamemap_print_options, METH_NOARGS,
    ""},
    {"clear", (PyCFunction)Gamemap_clear, METH_NOARGS,
    "clear the map of all sprites."},
    {NULL}
};

static PyGetSetDef Gamemap_getsets[] = {
    {"size", (getter)Gamemap_get_size,
     NULL, "", NULL},
    {"clumpiness", (getter)Gamemap_get_clumpiness,
     NULL, "", NULL},
    {"edge_bit", (getter)Gamemap_get_edge_bit,
     NULL, "", NULL},
    {"bits", (getter)Gamemap_get_bits,
     NULL, "", NULL},
    {NULL}
};


/**********************************************************************/
/* type definition structs                                            */
/**********************************************************************/
static PyTypeObject Gamemap_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                             /*          ob_size */
    "semanticCore.Gamemap",                            /*          tp_name */
    sizeof(Gamemap_object),                        /*     tp_basicsize */
    0,                                             /*      tp_itemsize */
    (destructor)Gamemap___del__,                   /*       tp_dealloc */
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
    mapdoc,                                        /*           tp_doc */
    0,                                             /*      tp_traverse */
    0,                                             /*         tp_clear */
    0,                                             /*   tp_richcompare */
    0,                                             /*tp_weaklistoffset */
    0,                                             /*          tp_iter */
    0,                                             /*      tp_iternext */
    (struct PyMethodDef *)Gamemap_methods,         /*       tp_methods */
    0,                                             /*       tp_members */
    (struct PyGetSetDef *)Gamemap_getsets,         /*        tp_getset */
    0,                                             /*          tp_base */
    0,                                             /*          tp_dict */
    0,                                             /*     tp_descr_get */
    0,                                             /*     tp_descr_set */
    0,                                             /*    tp_dictoffset */
    0,                                             /*          tp_init */
    0,                                             /*         tp_alloc */
    (newfunc)Gamemap___new__,                      /*           tp_new */
    0,                                             /*           tp_new */
    0,                                             /*          tp_free */
};

/**********************************************************************/
/* initialisation.                                                    */
/**********************************************************************/
#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initsemanticCore(void)
{
    PyObject* m;
    //Gamemap_type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&Gamemap_type) < 0)
        return;

    m = Py_InitModule3("semanticCore", top_level_functions,
		       "Contains a semantic bitmap of the game.  "	\
		       "The relative positions and overlap of objects are stored, " \
		       "rather than actual images."
	);

    if (m == NULL)
        return;

    Py_INCREF(&Gamemap_type);
    PyModule_AddObject(m, "Gamemap", (PyObject *)&Gamemap_type);

}
