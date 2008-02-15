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
#define GAPPY_SPRITES 1
#define BUCKET_BITS 6
#define BUCKETS (1 << BUCKET_BITS)
#define BYTE2BUCKET_BITS (8 - BUCKET_BITS)

#define POPULAR_HEAP_SIZE 256


#define INFLUENCE_RADIUS 8
#define INFLUENCE_DIAMETER (2 * INFLUENCE_RADIUS + 1)

#define min(a, b) (((a) < (b)) ? (a): (b))
#define max(a, b) (((a) > (b)) ? (a): (b))

#define DIFF2(a, b) (((a) - (b)) * ((a) - (b)))



/*from _imaging.c */
typedef struct {
    PyObject_HEAD
    Imaging image;
} ImagingObject;

typedef struct {
    PyObject_HEAD
    ImagingObject* image;
    int readonly;
} PixelAccessObject;


typedef struct img_point_s {
    int x;
    int y;
} img_point_t;


typedef struct colour_s {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} colour_t;

typedef struct colour_counter_s {
    uint32_t r;
    uint32_t g;
    uint32_t b;
    //uint32_t count;
} colour_counter_t;

typedef struct background_heap_s{
    uint32_t pop;
    uint32_t colour;
}background_heap_t;


typedef struct rect_s {
    int left;
    int top;
    int right;
    int bottom;
} rect_t;


static PyObject *Function_get_element (PyObject*, PyObject*);
static PyObject *Function_grow_background (PyObject*, PyObject*);
static PyObject *Function_find_background (PyObject*, PyObject*);
static PyObject *Function_expand_region (PyObject*, PyObject*);

static inline void debug_image(ImagingObject *im) __attribute__ ((unused));

/* functions  */

static inline void debug_image(ImagingObject *im)
{
    //Imaging img = im->image;
    debug("mode:      '%s'\n", im->image->mode);
    debug("type:      '%d'\n", im->image->type);
    debug("depth:     '%d'\n", im->image->depth);
    debug("bands:     '%d'\n", im->image->bands);
    debug("xsize:     '%d'\n", im->image->xsize);
    debug("ysize:     '%d'\n", im->image->ysize);
    debug("image8:    '%p'\n", im->image->image8);
    debug("image32:   '%p'\n", im->image->image32);
    debug("image:     '%p'\n", im->image->image);
    debug("block:     '%p'\n", im->image->block);
    debug("pixelsize: '%d'\n", im->image->pixelsize);
    debug("linesize:  '%d'\n", im->image->linesize);
}

