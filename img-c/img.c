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
#include "../lib/dbdebug.h"
#include "../lib/pyserf_helpers.c"
#include "Imaging.h"

#include "imagescan.h"

#include "median9.c"



/* Function_get_element (binds to Function.get_element) */
#define GET_ELEMENT_DOC "Return a mask of the sprite image. "	\
    "Arguments: \n"\
    "PIL pixel access objects of a blank black greyscale image "\
    "and the alpha channel of the original picture. \n"\
    "X and Y coordinates for the sprite to grow from.\n"\
    "The mask is modified in place, and crop coordinates, "\
    "colour info and pixel count are returned.\n"
///XXX could perhaps do more, and more efficiently.
//XXX and improve obviousness calculations!
static PyObject *
Function_get_element (PyObject *self, PyObject *args)
{
    Imaging alpha;
    Imaging mask;
    Imaging rgb;
    PixelAccessObject *apix;
    PixelAccessObject *mpix;
    PixelAccessObject *rgbpix;
    PyObject *py_colours;
    int startx, starty;
    int width, height;
    int i, k;
    int n_starts, n_nexts;
    img_point_t *starts;
    img_point_t *nexts;
    img_point_t *tmp;
    int left;
    int top;
    int right;
    int bottom;
    uint32_t pixel_count = 0;
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint32_t obviousness_sum = 0;
    uint8_t obviousness_best = 0;
    if (!PyArg_ParseTuple(args, "OOOiiO", &apix, &mpix, &rgbpix, &startx, &starty, &py_colours))
        return NULL;

    alpha = apix->image->image;
    mask = mpix->image->image;
    rgb = rgbpix->image->image;
    if (alpha->pixelsize != 1 ||
	mask->pixelsize != 1 ||
	alpha->xsize != mask->xsize ||
	alpha->ysize != mask->ysize){
	PyErr_Format(PyExc_ValueError, "alpha size is (%d,%d); mask is (%d,%d); pixelsizes are %d and %d",
		     alpha->xsize, alpha->ysize,
		     mask->xsize, mask->ysize,
		     alpha->pixelsize, mask->pixelsize);
	return NULL;
    }
    width = mask->xsize;
    height = mask->ysize;

    starts = malloc( 2 * width * height * sizeof(img_point_t));
    nexts = starts + width * height;

    if (!starts)
	return PyErr_NoMemory();

    /*get colours out of python sequence -- there is probably only one but can be up to 256 */
    int n_colours = PySequence_Size(py_colours);
    PyObject *py_fast = PySequence_Fast(py_colours, "not a sequence");
    colour_t *bg_colours = calloc((n_colours + 1), sizeof(colour_t) + sizeof(colour_counter_t));
    colour_counter_t *bg_counts = (colour_counter_t *)(bg_colours + n_colours + 1);
    if (! bg_colours){
	free(starts);
	return PyErr_NoMemory();
    }
    for (i = 0; i < n_colours; i++){
	PyObject *py_rgb = PySequence_Fast_GET_ITEM(py_fast, i); /* borrowed reference */
	PyObject *py_rgb_fast = PySequence_Fast(py_rgb, "also not a sequence");
	bg_colours[i].r = PyInt_AsLong(PySequence_Fast_GET_ITEM(py_rgb_fast, 0));
	bg_colours[i].g = PyInt_AsLong(PySequence_Fast_GET_ITEM(py_rgb_fast, 1));
	bg_colours[i].b = PyInt_AsLong(PySequence_Fast_GET_ITEM(py_rgb_fast, 2));
	Py_DECREF(py_rgb_fast);
    }
    Py_DECREF(py_fast);


    left = startx;
    right = startx;
    top = starty;
    bottom = starty;
    /* start *beside* the one point referenced in corner
       the reason is this one won't necessarily be added otherwise.
     */
    starts[0].x = startx - 1;
    starts[0].y = starty;
    n_starts = 1;

    img_point_t offsets[] = {{1,0},   {-1,0},   {0,1},   {0,-1},
			     {1,1},   {-1,1},   {1,-1}, {-1,-1}
    };

    while (n_starts){
	n_nexts = 0;
	for (i = 0; i < n_starts; i++){
	    int xx, yy, j;
	    for (j = 0; j < sizeof(offsets)/sizeof(img_point_t); j++){
		xx = starts[i].x + offsets[j].x;
		yy = starts[i].y + offsets[j].y;
		if (yy < 0 || yy >= height ||
		    xx < 0 || xx >= width)
		    continue;
		if (alpha->image8[yy][xx]){
		    /* obviousness calculations */
		    if (alpha->image8[yy][xx] > obviousness_best)
			obviousness_best = alpha->image8[yy][xx];
		    obviousness_sum += alpha->image8[yy][xx];
		    //XXX compare to background colours.
		    if (pixel_count < 10000){
			/* too much (> 64k) could cause overflows, but the
			   point is proved much sooner anyway*/
			uint8_t *pixel = (uint8_t *) &(rgb->image32[yy][xx]);
			for (k = 0; k < n_colours; k++){
			    bg_counts[k].r += DIFF2(bg_colours[k].r, pixel[0]);
			    bg_counts[k].g += DIFF2(bg_colours[k].g, pixel[1]);
			    bg_counts[k].b += DIFF2(bg_colours[k].b, pixel[2]);
			}
		    }

		    /*end obviousness */
		    alpha->image8[yy][xx] = 0;
		    mask->image8[yy][xx] = 255;
		    nexts[n_nexts].x = xx;
		    nexts[n_nexts].y = yy;
		    n_nexts++;
		    uint8_t *pixel = (uint8_t *) &(rgb->image32[yy][xx]);
		    red += pixel[0];
		    green += pixel[1];
		    blue += pixel[2];
		    if (xx < left)
			left = xx;
		    else if (xx > right)
			right = xx;
		    if (yy < top)
			top = yy;
		    else if (yy > bottom)
			bottom = yy;
		}
	    }
	}
	pixel_count += n_nexts;
	tmp = starts;
	starts = nexts;
	nexts = tmp;
	n_starts = n_nexts;
    }

    int32_t obviousness_global = INT_MAX;
    for (k = 0; k < n_colours; k++){
        /* +1 to prevent division by 0, and to bias against insignificant colours and small things */
	uint32_t c = (bg_counts[k].r + bg_counts[k].g + bg_counts[k].b) / (pixel_count + 1);
	if (c < obviousness_global)
	    obviousness_global = c;
	/*debug("colour %d: (%d,%d, %d) diffs: %d, %d, %d, pixel_count: %d (+1) -> c: %u worst %u\n",
	      k, bg_colours[k].r, bg_colours[k].g, bg_colours[k].b,
	      bg_counts[k].r, bg_counts[k].g, bg_counts[k].b, pixel_count, c, obviousness_global);*/
    }


    free(bg_colours);
    free(starts < nexts ? starts : nexts);

    if (pixel_count){
	red = (red + pixel_count / 2) / pixel_count;
	green = (green + pixel_count / 2) / pixel_count;
	blue = (blue + pixel_count / 2) / pixel_count;
    }
    /*debug("corner: (%d,%d) r %d g %d b %d count %d \n",
      left, top, red, green, blue, pixel_count);*/
    return Py_BuildValue("(iiii)(iii)iiii", left, top, right + 1, bottom + 1,
			 red, green, blue, pixel_count, obviousness_best, obviousness_sum, obviousness_global);
}



/*return 1 if the indicated point is valid and has alpha 0, otherwise 0.
 */
static inline int
is_background(int x, int y, Imaging im){
    if (x >= 0 && y >= 0 &&
	x < im->xsize &&
	y < im->ysize){
	UINT8 *p = (UINT8*) &im->image32[y][x];
	if (p[3] == 0)
	    return 1;
    }
    return 0;
}



/* Function_grow_background (binds to Function.grow_background) */
/* fill in the bits where the background was obscured by sprites   */

static PyObject *
Function_grow_background (PyObject *self, PyObject *args)
{
    Imaging im;
    PixelAccessObject *pix;
    PyObject *debug_function = NULL;
    int width;
    int height;
    int n_starts = 0;
    int n_nexts = 0;
    img_point_t *starts;
    img_point_t *nexts;
    img_point_t *tmp;
    int r, g, b, n, i;
    uint32_t y, x;
    if (!PyArg_ParseTuple(args, "O|O", &pix, &debug_function))
        return NULL;
    im = pix->image->image;
    if (im->pixelsize != 4)
	return PyErr_Format(PyExc_TypeError, "the image needs to have 4 bands\n");
    width = im->xsize;
    height = im->ysize;

    starts = malloc(2 * width * height * sizeof(img_point_t));
    nexts = starts + width * height;
    if (!starts)
	return PyErr_NoMemory();
    /* start with the points just beyond the edge of the background
       (background has *alpha 0*, bits to erase have alpha 255
    */
#if 0
    for (y = 0; y < height; y++){
	for (x = 0; x < width; x++){
	    UINT8 *p = (UINT8*) &im->image32[y][x];
	    if (p[3])
		p[3] = 255;
	}
    }
#endif
    for (y = 0; y < height; y++){
	for (x = 0; x < width; x++){
	    UINT8 *p = (UINT8*) &im->image32[y][x];
	    if (p[3]){
		p[3] = 255;
		if (is_background(x - 1, y, im) ||
		    is_background(x + 1, y, im) ||
		    is_background(x, y - 1, im) ||
		    is_background(x, y + 1, im)){
		    starts[n_starts].x = x;
		    starts[n_starts].y = y;
		    n_starts ++;
		}
	    }
	}
    }
    /* Wavefront looks at surrounding pixels. The colours of any that are
       marked as background are blended to make the new colour of this pixel;
       those that aren't background are added to the next wave.

       A pixel in the wavefront which has no neighbouring background pixels,
       will cause a division by zero, but I think this condition is
       impossible.

    */
#if 0
    img_point_t steps[4] = {{1,0},   {-1,0},   {0,1},   {0,-1}};
#elif 0
    img_point_t steps[12] = {{1,0},   {-1,0},   {0,1},   {0,-1},
			     {3,1},   {3,-1},   {-3,1},   {-3,-1},
			     {1,3},   {1,-3},   {-1,3},   {-1,-3}};
#elif 1
    img_point_t steps[12] = {{1,0},   {-1,0},   {0,1},   {0,-1},
			     {2,2},   {2,-2},   {-2,2},   {-2,-2},
			     {5,5},   {5,-5},   {-5,5},   {-5,-5}};
#elif 0
    img_point_t steps[8] = {{1,0},   {-1,0},   {0,1},   {0,-1},
			    {5,5},   {5,-5},   {-5,5},   {-5,-5}};
#else
    img_point_t steps[8] = {{1,0},   {-1,0},   {0,1},   {0,-1},
			    {1,1},   {1,-1},   {-1,1},   {-1,-1}};
#endif
    while(n_starts){

	if (debug_function){
	    if(! PyObject_CallFunctionObjArgs(debug_function, im, NULL)){
		return NULL;//hopefully exception has already been set
	    }
	}
	n_nexts = 0;
	for (i = 0; i < n_starts; i++){
	    x = starts[i].x;
	    y = starts[i].y;
	    r = 0;
	    g = 0;
	    b = 0;
	    n = 0;
	    int xx, yy, j;
	    for (j = 0; j < sizeof(steps)/sizeof(img_point_t); j++){
		xx = starts[i].x + steps[j].x;
		yy = starts[i].y + steps[j].y;
		if (yy < 0 || yy >=height ||
		    xx < 0 || xx >= width){
		    continue;
		}
		UINT8 *q =  (UINT8*) &im->image32[yy][xx];
		if (q[3] == 0){ //this is background, blend in
		    r += (int)q[0];
		    g += (int)q[1];
		    b += (int)q[2];
		    n++;
		}
		else if (q[3] == 255 &&
			 (steps[j].x == 0 || steps[j].y == 0)){
		    /* add to nexts list, if it is a small single jump.*/
		    nexts[n_nexts].x = xx;
		    nexts[n_nexts].y = yy;
		    n_nexts++;
		    q[3] = 128;
		}
	    }

	    if (n == 0){
		debug("n is 0 at (%d,%d)\n", x, y);
	    }
	    else{
		UINT8 *p = (UINT8*) &im->image32[y][x];
		p[0] = (r + n/2) / n;
		p[1] = (g + n/2) / n;
		p[2] = (b + n/2) / n;
		p[3] = 0;
	    }
	}
	tmp = starts;
	starts = nexts;
	nexts = tmp;
	n_starts = n_nexts;
    }
    free(starts < nexts ? starts : nexts);
    return Py_BuildValue("");
}

/* Function_find_background (binds to Function.find_background) */
/* make a three dimensional histogram, return a sorted list of the most popular colours   */


/****** heap operations for find_background ************/

static inline void downheap(background_heap_t *heap, int size, int pos){
    asm("/***********downheap starts*****/\n\t");
    uint32_t colour = heap[pos].colour;
    uint32_t pop = heap[pos].pop;
    int child;
    while ((child = 2 * pos + 1) < size){
	child += (heap[child].pop >
		  heap[child + 1].pop);

	if (pop <= heap[child].pop){
	    break;
	}
	heap[pos] = heap[child];
	pos = child;
    }
    heap[pos].pop = pop;
    heap[pos].colour = colour;
    asm("/***********downheap ends*****/\n\t");
}

/*sort heap assumes a heap is already in heap order;
 afterwards it is in reverse sorted order.. */

static inline void
sort_heap(background_heap_t *heap, unsigned int n){
    n--;
    background_heap_t *top = heap + n;
    background_heap_t tmp;
    for (; heap < top; top--, n--){
	tmp.pop = heap->pop;
	tmp.colour = heap->colour;
	heap->pop = top->pop;
	heap->colour = top->colour;
	top->pop = INT_MAX;            // necessary for downheap to work.
	downheap(heap, n, 0);
	top->pop = tmp.pop;
	top->colour = tmp.colour;
    }
}

/*count the popularity of different colours in a (decimated) colour cube.
 */
static inline void
histogram3d(ImagingObject *im, uint32_t *counts)
{
    int width = im->image->xsize;
    int height = im->image->ysize;
    uint32_t r, g, b;
    uint32_t y, x;
    for (y = 0; y < height; y++){
	for (x = 0; x < width; x++){
	    UINT8 *p = (UINT8*) &im->image->image32[y][x];
	    r = p[0] >> BYTE2BUCKET_BITS;
	    g = p[1] >> BYTE2BUCKET_BITS;
	    b = p[2] >> BYTE2BUCKET_BITS;
	    counts[(r << 2 * BUCKET_BITS) + (g << BUCKET_BITS) + b]++;
        }
    }
}

/* look into the image and find the most popular colours, returning these as a
  sorted list, along with their popularity */

static PyObject *
Function_find_background (PyObject *self, PyObject *args)
{
    ImagingObject *im;
    PixelAccessObject *pix;
    PyObject *results = PyList_New(POPULAR_HEAP_SIZE);
    int i, r, g, b;
    if (!PyArg_ParseTuple(args, "O", &pix))
        return NULL;
    im = pix->image;
    if (im->image->pixelsize != 4)
	return PyErr_Format(PyExc_TypeError, "the image needs to have 4 bands\n");

    /* each band is split into BUCKETS groups, which needs an array of BUCKETS ** 3
      BUCKETS = 64 -> 1MByte
      BUCKETS = 128 -> 8MByte
    */
    uint32_t *counts = calloc(BUCKETS * BUCKETS * BUCKETS, sizeof(uint32_t));
    if (! counts)
	return PyErr_NoMemory();

    histogram3d(im, counts);

    /*find the histogram peaks using a heap */
    background_heap_t popular[POPULAR_HEAP_SIZE];
    for (i = 0; i < POPULAR_HEAP_SIZE; i++){
	popular[i].pop = 0;
    }
    for (i = 0; i < BUCKETS * BUCKETS *BUCKETS; i++){
	if (counts[i] > popular->pop){
	    popular->pop = counts[i];
	    popular->colour = i;
	    downheap(popular, POPULAR_HEAP_SIZE, 0);
	}
    }
    sort_heap(popular, POPULAR_HEAP_SIZE);

    /*convert colour back into 24bit, and put in a python structure */
    for (i = 0; i < POPULAR_HEAP_SIZE; i++){
	r = popular[i].colour >> (2 * BUCKET_BITS);
	g = (popular[i].colour >> BUCKET_BITS) & (BUCKETS - 1);
	b = popular[i].colour & (BUCKETS - 1);
	UINT8 *p = (UINT8*) &popular[i].colour;
	//add half of bucket size to make up for rounding
	p[0] = (r << BYTE2BUCKET_BITS) + (1 << (BYTE2BUCKET_BITS - 1));
	p[1] = (g << BYTE2BUCKET_BITS) + (1 << (BYTE2BUCKET_BITS - 1));
	p[2] = (b << BYTE2BUCKET_BITS) + (1 << (BYTE2BUCKET_BITS - 1));
	p[3] = 0;
	PyObject *t = Py_BuildValue("ii", popular[i].pop, popular[i].colour);
	PyList_SET_ITEM(results, i, t);
    }

    free(counts);
    return results;
}


/* if point (x,y) is close enough to colour (r,g,b) in Euclidean RGB space,
   add it to the nexts list. otherwise, not.*/

static inline int
expand_one(int x, int y, int r, int g, int b,
	   int threshold, img_point_t *nexts, int *n_nexts, ImagingObject *im){
    UINT8 *p = (UINT8*) &im->image->image32[y][x];
    if (p[3]){
	int diff = ((p[0] - r) * (p[0] - r) +
		    (p[1] - g) * (p[1] - g) +
		    (p[2] - b) * (p[2] - b));
	if (diff  <= threshold){
	    p[3] = 0;
	    nexts[*n_nexts].x = x;
	    nexts[*n_nexts].y = y;
	    (*n_nexts)++;
	    return 1;
	}
	if (diff <= 255) /*for obviousness calculations, later */
	    p[3] = diff;
    }
    return 0;
}


/* Function_expand_region (binds to Function.expand_region) */
/* expand a region with pixels near to <colour>, varying by at most   */
/* <threshold> from each other, and <abs_threshold> from <colour>   */

static PyObject *
Function_expand_region (PyObject *self, PyObject *args)
{
    ImagingObject *im;
    PixelAccessObject *pix;
    PyObject *colourlist;
    PyObject *debug_function = NULL;
    uint32_t colours[POPULAR_HEAP_SIZE];
    int n_colours;
    int threshold;
    int init_threshold;
    int width;
    int height;
    int n_starts = 0;
    int n_nexts = 0;
    img_point_t *starts;
    img_point_t *nexts;
    int r, g, b, i;
    int y, x;
    if (!PyArg_ParseTuple(args, "OOii|O", &pix, &colourlist, &threshold, &init_threshold,
			  &debug_function))
        return NULL;
    im = pix->image;

    if (im->image->pixelsize != 4){
	PyErr_Format(PyExc_ValueError, "pixelsizes is %d, want 4", im->image->pixelsize);
	return NULL;
    }

    n_colours = PySequence_Size(colourlist);
    if (n_colours > POPULAR_HEAP_SIZE){
	PyErr_Format(PyExc_ValueError, "got  %d colours, want %d", n_colours, POPULAR_HEAP_SIZE);
	return NULL;
    }
    width = im->image->xsize;
    height = im->image->ysize;
    //malloc 2 lists of points. These *could* be as large as the image (but never should be)
    starts = malloc(width * height * 2 * sizeof(img_point_t));
    nexts = starts + width * height;

    if(! starts)
	return PyErr_NoMemory();

    int_vector_from_list((int*)colours, colourlist, n_colours);

    //threshold *= threshold;  //for simpler euclidean measure
    //init_threshold *= init_threshold;

    /*  find the initial points. These are the points within the init_threshold
	distance of one of the background colours.  This list of points will
	later grow.
     */
    for (y = 0; y < height; y++){
	for (x = 0; x < width; x++){
	    UINT8 *p = (UINT8*) &im->image->image32[y][x];
	    r = (int)p[0];
	    g = (int)p[1];
	    b = (int)p[2];
	    for (i = 0; i < n_colours; i++){
		UINT8 *q = (UINT8*) &colours[i];
		if ((r - (int)q[0]) * (r - (int)q[0]) +
		    (g - (int)q[1]) * (g - (int)q[1]) +
		    (b - (int)q[2]) * (b - (int)q[2]) <= init_threshold){
		    p[3] = 0;
		    starts[n_starts].x = x;
		    starts[n_starts].y = y;
		    n_starts++;
		}
	    }
        }
    }

    /* expand with a wavefront, each point consuming its neighbours that
       differ from it by less than the threshold. If the neighbour is too
       different, it tries jumping ahead in that direction, to see if there is
       more on the other side. ("wavefront with sparks")*/
    while(n_starts){
	if (debug_function){
	    PyObject_CallFunctionObjArgs(debug_function, im, NULL);
	}
	n_nexts = 0;
	int i;
	for (i = 0; i < n_starts; i++){
	    int x = starts[i].x;
	    int y = starts[i].y;
	    UINT8 *p = (UINT8*) &im->image->image32[y][x];
	    r = (int)p[0];
	    g = (int)p[1];
	    b = (int)p[2];
#define JUMPAHEAD 10
#define JA_THRESHOLD (threshold - 2)
	    if (x > 0)
		if (! expand_one(x - 1, y, r, g, b, threshold, nexts, &n_nexts, im) &&
		    x > JUMPAHEAD - 1)
		    expand_one(x - JUMPAHEAD, y, r, g, b, JA_THRESHOLD, nexts, &n_nexts, im);
	    if (x < width - 1)
		if (! expand_one(x + 1, y, r, g, b, threshold, nexts, &n_nexts, im) &&
		    x < width - JUMPAHEAD - 1)
		    expand_one(x + JUMPAHEAD, y, r, g, b, JA_THRESHOLD, nexts, &n_nexts, im);
	    if (y > 0)
		if (! expand_one(x, y - 1, r, g, b, threshold, nexts, &n_nexts, im) &&
		    y > JUMPAHEAD - 1)
		    expand_one(x, y - JUMPAHEAD, r, g, b, JA_THRESHOLD, nexts, &n_nexts, im);
	    if (y < height - 1)
		if (! expand_one(x, y + 1, r, g, b, threshold, nexts, &n_nexts, im) &&
		    y < height - JUMPAHEAD - 1)
		    expand_one(x, y + JUMPAHEAD, r, g, b, JA_THRESHOLD, nexts, &n_nexts, im);

	}
	img_point_t *tmp = starts;
	starts = nexts;
	nexts = tmp;
	n_starts = n_nexts;
    }

    if (debug_function){
	PyObject_CallFunctionObjArgs(debug_function, im, NULL);
    }
    free(starts < nexts ? starts : nexts);
    return Py_BuildValue("O", im);
}

/*uses stuff in median9.c to get a median of a pixel and its 8 neighbours*/
static PyObject *
Function_fast_median(PyObject *self, PyObject *args)
{
    PixelAccessObject *pix_in;
    PixelAccessObject *pix_out;
    if (!PyArg_ParseTuple(args, "OO", &pix_in, &pix_out))
        return NULL;

    blob_fast_median9(pix_in->image->image->image32,
		      pix_out->image->image->image32,
		      pix_out->image->image->xsize,
		      pix_out->image->image->ysize);
    return Py_BuildValue("");
}



#define SURFACE_AREA_DOC \
    "Count the pixels which are on the edge of the drawing, " \
    "and those that are on (surface, and area). "\
    "It expects an alpha channel, and counts b/w transitions. " \
    "for surface, and non-zero pixels for area"
/* XXX area duplicates volume, clacluated during element finding */
/*  ####            ####
   #######         #....##       surface is count of outer edges.
   ########        ##.....#      area is count of all pixels.
     #######  -->    ###...#  ->
        ####	        #.##
   #   ###  	   #   ###
   ####      	   ####
 */
static PyObject *
Function_surface_area(PyObject *self, PyObject *args)
{
    PixelAccessObject *pix;
    ImagingObject *map;
    int x, y;
    if (!PyArg_ParseTuple(args, "O", &pix))
        return NULL;
    map = pix->image;
    int lastx = map->image->xsize - 1;
    int lasty = map->image->ysize - 1;
    uint8_t **im = map->image->image8;

    /*bottom corner won't counted any other way. it has 2 edges for surface
      count.*/
    int area = (im[lasty][lastx] != 0);
    int surface = area + area;

    for (y = 0; y < lasty; y++){
	for (x = 0; x < lastx; x++){
	    surface += (im[y][x] != im[y + 1][x]);
	    surface += (im[y][x] != im[y][x + 1]);
	    area += im[y][x] != 0;
	}
	surface += im[y][lastx] != im[y + 1][lastx];
	//edge pieces are assumed to border black
	surface += im[y][lastx] != 0;
	surface += im[y][0] != 0;
	area += im[y][lastx] != 0;
    }
    for (x = 0; x < lastx; x++){
	surface += im[lasty][x] != im[lasty][x + 1];
	surface += im[lasty][x] != 0;
	surface += im[0][x] != 0;
	area += im[lasty][x] != 0;
    }
    return Py_BuildValue("ii", surface, area);
}


/* next two are helper functions for convex hulls */

static inline void
find_row_extrema(uint8_t **im, int lefts[], int rights[],
		 int cols, int rows)
{
    int x, y, x2;
    /* find the leftmost and rightmost points in each row */
    for (y = 0; y < rows; y++){
	for (x = 0; x <= cols; x++){
	    if (im[y][x]){
		lefts[y] =  x;
		x2 = cols;
		while(--x2 >= 0){
		    if (im[y][x2]){
			rights[y] = x2;
			break;
		    }
		}
		break;
	    }
	}
    }
}

/*direction should be -1 for left side, 1 for right */
#define RIGHT_CONVEXITY 1
#define LEFT_CONVEXITY -1

static inline void
find_outermost_angle(int points[], int rows, int i, int direction,
		     int *next, double *ddx)
{
    int dx = -direction;    /*start out pointing straight in */
    int dy = 0;
    int j;
    for (j = i + 1; j < rows; j++){
	if (points[j] >= 0){
	    int ax = points[j] - points[i];
	    int ay = j - i;
	    if (ax * dy * direction >= dx * ay * direction){/* this is the new best next point */
		dx = ax;
		dy = ay;
		*next = j;
	    }
	}
    }
    if (dy != 0) /*should only be possible on last row if even there*/
	*ddx = (double)dx / dy;
}




#define INTERSECT_DOC \
    "return true if the two images have overlapping convex hulls."\
    "Takes 2 pixel access objects, followed by 2 containing rectangles."
static PyObject *
Function_intersect(PyObject *self, PyObject *args)
{
    PixelAccessObject *pix1;
    PixelAccessObject *pix2;
    Imaging map1;
    Imaging map2;
    int x, y;
    rect_t b1;
    rect_t b2;
    if (!PyArg_ParseTuple(args, "OO(iiii)(iiii)", &pix1, &pix2,
			  &b1.left, &b1.top, &b1.right, &b1.bottom,
			  &b2.left, &b2.top, &b2.right, &b2.bottom))
        return NULL;
    if (b1.left > b2.right || b1.top > b2.bottom ||
	b2.left > b1.right || b2.top > b1.bottom){
	//debug("no shared rectangle! can't intersect\n");
	Py_RETURN_FALSE;
    }

    /*make pix1 the smallest */
    if ((b2.right - b2.left) * (b2.bottom - b2.top) <
	(b1.right - b1.left) * (b1.bottom - b1.top)){
	rect_t btmp = b1;
	b1 = b2;
	b2 = btmp;
	PixelAccessObject *pixtmp = pix1;
	pix1 = pix2;
	pix2 = pixtmp;
    }

    map1 = pix1->image->image;
    map2 = pix2->image->image;
    uint8_t **im1 = map1->image8;
    uint8_t **im2 = map2->image8;
    int rows = map1->ysize;
    int *lefts = malloc(rows * 2 * sizeof(int));
    if (!lefts) //as if
	return PyErr_NoMemory();
    memset(lefts, -1, rows * 2 * sizeof(int));
    int *rights = lefts + rows;
    /* find the leftmost and rightmost points in each row */
    find_row_extrema(im1, lefts, rights, map1->xsize, rows);

    /*so outer points in each scan line are known
      now,
      1. look for the widest angled point
      2. move there, steadily, scanning each line for the other thing.
      3. goto 1
    */
    int i;
    double xl = lefts[0] + 0.5;
    double xr = rights[0] + 0.5;
    double dxl = 0;
    double dxr = 0;
    int il = 0;
    int ir = 0;

    int intertwined = 0;
    for(i = 0; i < rows; i++){
	if (i == il && i != rows - 1){ /*time to look for a new direction */
	    xl = (double)lefts[i] + 0.5;
	    find_outermost_angle(lefts, rows, i, LEFT_CONVEXITY, &il, &dxl);
	}
	if (i == ir && i != rows - 1){ /*same again for other side */
	    xr = (double)rights[i] + 0.5;
	    find_outermost_angle(rights, rows, i, RIGHT_CONVEXITY, &ir, &dxr);
	}
	/*now scan */
	xl += dxl;
	xr += dxr;
	/*everything so far has been relative to map1, b1;
	  now need to covert to map2, b2 */
	y = i + b1.top - b2.top;
	if (y >= 0 && y < map2->ysize){
	    int _xl = (int)xl + b1.left - b2.left;
	    int _xr = (int)xr + b1.left - b2.left;
	    if (_xl < 0)
		_xl = 0;
	    if (_xr > map2->xsize)
		_xr = map2->xsize - 1;
	    for (x = _xl; x < _xr; x++){
		if(im2[y][x]){
		    intertwined = 1;
		    goto end;
		}
	    }
	}
    }
  end:
    free(lefts);
    if (intertwined)
	Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}
/*find_extreme_points
   for each left:
      find the furthest right.

  there are no doubt non-quadratic methods, but some simple greedy algorithms
  can get stuck in local maxima.

 */


static inline double
find_extreme_points(img_point_t left_hull[], img_point_t right_hull[],
		    int n_left, int n_right,
		    int *left_extreme, int *right_extreme){

    inline int dist(int _l, int _r){
	return ((left_hull[_l].y - right_hull[_r].y) * (left_hull[_l].y - right_hull[_r].y) +
		(left_hull[_l].x - right_hull[_r].x) * (left_hull[_l].x - right_hull[_r].x));
    }

    int best_d = 0;
    int best_l = 0;
    int best_r = 0;

    for (int i = 0; i < n_left; i++){
	for (int j = 0; j < n_right; j++){
	    int d = dist(i, j);
	    if (d > best_d){
		best_l = i;
		best_r = j;
		best_d = d;
	    }
	}
    }
    *left_extreme = best_l;
    *right_extreme = best_r;
    return sqrt((double)best_d);
}


static inline double
find_width(img_point_t left_hull[], img_point_t right_hull[],
	   int n_left, int n_right,
	   int left_extreme, int right_extreme, uint8_t **dbim){
    /* find the widest out points on either side.  this is essentially the
       same as finding the largest triangle. The derivation of this formula is
       shown in ./triangle.py.
     */
    img_point_t p1 = left_hull[left_extreme];
    img_point_t p2 = right_hull[right_extreme];
    /*These constants are use by both sides. */
    int cx = p2.y - p1.y;
    int cy = p1.x - p2.x;
    int cc = p2.x * p1.y - p2.y * p1.x;
    double hypotenuse = sqrt((double)((p1.x - p2.x) * (p1.x - p2.x)  + (p1.y - p2.y) * (p1.y - p2.y)));
    img_point_t p_top;
    img_point_t p_bottom;
    /* in some circumstances, can end up uninitiated!?*/
    p_top.x = left_hull[0].x;
    p_top.y = left_hull[0].y;
    p_bottom.x = p1.x;
    p_bottom.y = p1.y;

    int d_top = -1;
    int d_bottom = -1;

    inline void scan (int start, int end, img_point_t hull[],
		      img_point_t *p, int *best)
    {
	for (int i = start; i < end; i++){
	    //perhaps cc can come out of the loop? abs() is the problem
	    int d = abs(hull[i].x * cx + hull[i].y * cy + cc);
	    if (d > *best){
		*best = d;
		p->x = hull[i].x;
		p->y = hull[i].y;
	    }
	}
    }
    /* scan top */
    scan(0, left_extreme, left_hull, &p_top, &d_top);
    scan(0, right_extreme, right_hull, &p_top, &d_top);
    //debug("top is %d,%d  %d. extremes %d, %d; n %d,%d\n", p_top.x, p_top.y, d_top, left_extreme, right_extreme,n_left, n_right);
    /*bottom*/
    scan(left_extreme + 1, n_left, left_hull, &p_bottom, &d_bottom);
    scan(right_extreme + 1, n_right, right_hull, &p_bottom, &d_bottom);
    //debug("bottom is %d,%d  %d\n", p_bottom.x, p_bottom.y, d_bottom);
    if (dbim){
	dbim[p_top.y][p_top.x] = 110;
	dbim[p_bottom.y][p_bottom.x] = 110;
    }
    return (d_top + d_bottom) / hypotenuse;
}




#define CONVEX_HULL_SIZE_DOC \
    "Various calculations using the convex hull. These are together "\
    "to avoid repeating the hull calculations too many times. \n"\
    "1. count the pixels included in the convex hull. \n" \
    "2. measure the convex perimeter. \n" \
    "3. measure length and width. \n" \
    "4. find the orientation \n" \
    "The calculation is done with floating point, so fractional pixels are counted."
static PyObject *
Function_convex_hull_size(PyObject *self, PyObject *args)
{
    PixelAccessObject *pix = NULL;
    PixelAccessObject *dbpix = NULL;
    uint8_t **dbim = NULL;
    Imaging map;

    if (!PyArg_ParseTuple(args, "O|O", &pix, &dbpix))
        return NULL;

    if (dbpix){/*draw the findings on a debug image*/
	Imaging dbmap = dbpix->image->image;
	dbim = dbmap->image8;
    }


    map = pix->image->image;
    uint8_t **im = map->image8;
    int rows = map->ysize;
    int *lefts = malloc(rows * 2 * (sizeof(int) + sizeof(img_point_t)));
    if (!lefts)
	return PyErr_NoMemory();
    memset(lefts, -1, rows * 2 * sizeof(int));
    int *rights = lefts + rows;
    img_point_t *left_hull = (img_point_t *)(rights + rows);
    img_point_t *right_hull = left_hull + rows;

    /* find the leftmost and rightmost points in each row */
    find_row_extrema(im, lefts, rights, map->xsize, rows);

    int i;
    double xl = lefts[0] + 0.5;
    double xr = rights[0] + 0.5;
    double dxl = 0;
    double dxr = 0;
    int il = 0;
    int ir = 0;
    int left_ctr = 0;
    int right_ctr = 0;
    double sum = rows; /* + cols ? count the outsides */
    double perimeter = xr - xl;
    for(i = 0; i < rows; i++){
	if (i == il && i != rows - 1){ /*time to look for a new direction */
	    left_hull[left_ctr].x = lefts[i];
	    left_hull[left_ctr].y = i;
	    left_ctr++;
	    xl = (double)lefts[i] + 0.5;
	    find_outermost_angle(lefts, rows, i, LEFT_CONVEXITY, &il, &dxl);
	    perimeter += sqrt((il - i) * (il - i) * (dxl * dxl + 1));
	}
	if (i == ir && i != rows - 1){ /*same again for other side */
	    right_hull[right_ctr].x = rights[i];
	    right_hull[right_ctr].y = i;
	    right_ctr++;
	    xr = (double)rights[i] + 0.5;
	    find_outermost_angle(rights, rows, i, RIGHT_CONVEXITY, &ir, &dxr);
	    perimeter += sqrt((ir - i) * (ir - i) * (dxr * dxr + 1));
	}
	/*now add up the included pixels */
	sum += xr - xl;
	if (dbim){
	    dbim[i][(int)xl] = 150;
	    dbim[i][(int)xr] = 150;
	}
	xl += dxl;
	xr += dxr;
    }
    perimeter += xr - xl;

    left_hull[left_ctr].x = lefts[rows - 1];
    left_hull[left_ctr].y = rows - 1;
    left_ctr++;
    right_hull[right_ctr].x = rights[rows - 1];
    right_hull[right_ctr].y = rows - 1;
    right_ctr++;
    int left_extreme, right_extreme;
    double length = find_extreme_points(left_hull, right_hull, left_ctr, right_ctr,
					&left_extreme, &right_extreme);

    img_point_t le = left_hull[left_extreme];
    img_point_t re = right_hull[right_extreme];

    /*now, find the width at right angles to the main axis */
    double width = find_width(left_hull, right_hull, left_ctr, right_ctr,
			      left_extreme, right_extreme, dbim);

    double slope;
    if (le.x == re.x)
	slope = 10000.0;
    else
	slope = (double)abs(le.y - re.y) / abs(le.x - re.x);

    if (dbim){
	dbim[le.y][le.x] = 210;
	dbim[re.y][re.x] = 210;
	dbim[le.y + ((re.y - le.y) >> 1)][le.x + ((re.x - le.x) >> 1)] = 210;
    }

    free(lefts);
    return Py_BuildValue("iiddd", (int)sum, (int)perimeter, length, width, slope);
}


static inline int32_t
rgb_similarity(uint8_t *p1, uint8_t *p2, uint8_t *target1, uint8_t *target2)
{
    int r = (p1[0] - p2[0]) * (p1[0] - p2[0]);
    int g = (p1[1] - p2[1]) * (p1[1] - p2[1]);
    int b = (p1[2] - p2[2]) * (p1[2] - p2[2]);
    /* how to do this? solidity is inversely proportional to difference. euclidean? */
    int total = 256 / (4 + r + g + b);
    *target1 += total;
    *target2 += total;
    return total;
}

#define SOLIDITY_DOC \
    "estimate how densely connected each pixel is."

static PyObject *
Function_solidity(PyObject *self, PyObject *args)
{
    PixelAccessObject *pix = NULL; /*want an RGBA image*/
    PixelAccessObject *spix = NULL;

    if (!PyArg_ParseTuple(args, "OO", &pix, &spix))
        return NULL;

    Imaging smap = spix->image->image;
    Imaging image = pix->image->image;
    int32_t **im = image->image32;
    uint8_t **sim = smap->image8;
    int y, x;
    uint32_t count = 0;
    uint32_t total = 0;

    uint8_t *p;
    for (y = 0; y < image->ysize - 1; y++){
	for (x = 0; x < image->xsize - 1; x++){
	    //debug("y %d x %d \n", y, x);
	    p = (uint8_t*) &im[y][x];
	    if (p[3]){
		/*only process if there is alpha */
		uint8_t *px = (uint8_t*) &im[y][x + 1];
		uint8_t *py = (uint8_t*) &im[y + 1][x];
		if (px[3]){
		    total += rgb_similarity(p, px,
					    &sim[y][x],
					    &sim[y][x + 1]);
		    count++;
		}
		if (py[3]){
		    total += rgb_similarity(p, py,
					    &sim[y][x],
					    &sim[y + 1][x]);
		    count++;
		}
	    }
	    /* else, if alpha is zero outside, aim for maximum contrast ? */
	}
	/*now, last column, y only */
	//x = image->xsize - 1;
	p = (uint8_t*) &im[y][x];
	if (p[3]){
	    uint8_t *py = (uint8_t*) &im[y + 1][x];
	    if (py[3]){
		total += rgb_similarity(p, py,
					&sim[y][x],
					&sim[y + 1][x]);
		count++;
	    }
	}
    }
    /*last row */
    //y = image->ysize - 1;
    for (x = 0; x < image->xsize - 1; x++){
	p = (uint8_t*) &im[y][x];
	if (p[3]){
	    uint8_t *px = (uint8_t*) &im[y][x + 1];
	    if (px[3]){
		total += rgb_similarity(p, px,
					&sim[y][x],
					&sim[y][x + 1]);
		count++;
	    }
	}
    }

    /* so now, find paths through valleys that break off one bit or another.
       possibly:

       a) slowly raise water level until islands appear. or

       b) seek paths through (fairly greedily), starting from every edge
       point. most paths can be abandoned at first step.
       can inhibit cutting off toes rather than knees, etc.
       go in roughly perpendicular.

     */
    //debug("total is %u, count is %u\n", total, count);
    if (count == 0){
	total = 128;
	count = 1;
    }
    return Py_BuildValue("d", (double)total / count);
}



#define SYMMETRY_DOC \
    "count the pixels in each corner."

/* other better ways, might be to subtract opposite sides, or something, and
   weight the outside */
/* perhaps use gaussian? */

static PyObject *
Function_symmetry(PyObject *self, PyObject *args)
{
    PixelAccessObject *pix;
    int x, y;
    if (!PyArg_ParseTuple(args, "O", &pix))
        return NULL;

    uint32_t width = pix->image->image->xsize;
    uint32_t height = pix->image->image->ysize;
    if (width < 2 || height < 2){
	return Py_BuildValue("iiii", 0, 0, 0, 0);
    }
    uint8_t **im = pix->image->image->image8;
    uint32_t x2 = width / 2;
    uint32_t y2 = height / 2;
    uint32_t w2 = x2 + (width & 1);
    uint32_t h2 = y2 + (height & 1);
    int quarters[4] = {0, 0, 0, 0};
    for (y = 0; y < height; y++){
	for (x = 0; x < width; x++){
	    /*surely optimisable, but the tricky thing is the difference
	    between odd and even sizes. on odd sized ones, the middle line is
	    counted in both sides. Another way would be for it to be counted
	    by neither side.
	    */
	    if (im[y][x]){
		if (y < h2){
		    if (x < w2){
			quarters[0]++;
		    }
		    if (x >= x2){
			quarters[1]++;
		    }
		}
		else if (y > y2){
		    if (x < w2){
			quarters[2]++;
		    }
		    if (x >= x2){
			quarters[3]++;
		    }
		}
	    }
	}
    }
    return Py_BuildValue("iiii", quarters[0], quarters[1], quarters[2], quarters[3]);
}

#define SCRAMBLE_DOC \
    "shuffle the pixels in an rgba image, slightly."
//XXX should test this outside the game
static PyObject *
Function_scramble(PyObject *self, PyObject *args)
{
    PixelAccessObject *inpix = NULL;
    PixelAccessObject *outpix = NULL;
    if (!PyArg_ParseTuple(args, "OO", &outpix, &inpix))
        return NULL;
    Imaging image = inpix->image->image;
    int32_t **orig = inpix->image->image->image32;
    int32_t **new = outpix->image->image->image32;
    int x, y;
    int ix = 0;
    int iy = 0;
    int circuit[] = {-1, -1, 0, -1, 1, 0, 0, 1, 1};
    for (y = 1; y < image->ysize - 2; y++){
	for (x = 1; x < image->xsize - 2; x++){
	    new[y][x] = orig[y + circuit[iy]][x + circuit[iy]];
	    ix += 1;
	    iy += 4;
	    if (ix > 8)
		ix -= 9;
	    if (iy > 8)
		iy -= 9;
	}
    }
    return Py_BuildValue("");
}







#define OBVIOUSNESS_DOC \
    "how different in the picture from the given background colours?"

// mean is perhaps not correct. just so long as some points are really obvious, the thing will show up
// the minimum obviousness, out of all background colours is what matters.
// contrast with edge pixels also matters.
static PyObject *
Function_obviousness(PyObject *self, PyObject *args)
{
    PixelAccessObject *pix;
    PyObject *py_colours;
    int x, y, i;
    if (!PyArg_ParseTuple(args, "OO", &pix, &py_colours))
        return NULL;
    int width = pix->image->image->xsize;
    int height = pix->image->image->ysize;
    int32_t **im = pix->image->image->image32;
    int n_colours = PySequence_Size(py_colours);
    PyObject *py_fast = PySequence_Fast(py_colours, "not a sequence");
    uint32_t worst = INT_MAX;
    for (i = 0; i < n_colours; i++){
	PyObject *py_rgb = PySequence_Fast_GET_ITEM(py_fast, i);
	PyObject *py_rgb_fast = PySequence_Fast(py_rgb, "also not a sequence");
	int r = PyInt_AsLong(PySequence_Fast_GET_ITEM(py_rgb_fast, 0));
	int g = PyInt_AsLong(PySequence_Fast_GET_ITEM(py_rgb_fast, 1));
	int b = PyInt_AsLong(PySequence_Fast_GET_ITEM(py_rgb_fast, 2));
	Py_DECREF(py_rgb_fast);
	uint32_t red = 0;
	uint32_t green = 0;
	uint32_t blue = 0;
	uint32_t count = 1; /* bias against tiny ones */
	for (y = 0; y < height; y++){
	    for (x = 0; x < width; x++){
		uint8_t *pixel = (uint8_t *) &(im[y][x]);
		if (pixel[3]){
		    count++;
		    red += DIFF2(r, pixel[0]);
		    green += DIFF2(g, pixel[1]);
		    blue += DIFF2(b, pixel[2]);
		}
	    }
	    if (count > 10000){
		/*got enough; too much could cause overflows */
		break;
	    }
	}
	uint32_t c = (red + green + blue) / count;
	if (c < worst)
	    worst = c;
    }
    Py_DECREF(py_fast);
    return Py_BuildValue("d", sqrt((double)worst));
}


#if 0
#define ASPECT_DOC \
    "work out the ratio of longest axis to shortest, and angle."

static PyObject *
Function_aspect(PyObject *self, PyObject *args)
{
    PixelAccessObject *pix = NULL;
    PixelAccessObject *spix = NULL;

    if (!PyArg_ParseTuple(args, "OO", &pix, &spix))
        return NULL;

    Imaging smap = spix->image->image;
    Imaging image = pix->image->image;
    uint8_t **im = image->image8;
    uint8_t **sim = smap->image8;

    int tw = image->xsize + 2 * ASPECT_EDGE;
    int th = image->ysize + 2 * ASPECT_EDGE;
    debug("tw is %d, th is %d\n", tw, th);
    uint8_t *working = calloc(2 * tw * (th + 1), sizeof(uint8_t));
    if (! working)
	return PyErr_NoMemory();
    uint8_t *map1 = working + (tw + 2) * ASPECT_EDGE;
    uint8_t *map2 = map1 + tw * th;
    int y, x;
    /*set up the working copy, with soft edges for overflow */
    uint8_t *row = map1;
    for (y = 0; y < image->ysize - 1; y++){
	for (x = 0; x < image->xsize - 1; x++){
	    row[x] = im[y][x];
	}
	row += tw;
    }
    int round = 0;
    int changed = 1;
    while(changed){
	changed = 0;
	memset(map2, 0, tw * th);
	uint8_t *row1 = map1;
	uint8_t *row2 = map2;
	for (y = 0; y < image->ysize; y++){
	    for (x = 0; x < image->xsize; x++){
		if(row1[x])
		    changed |= aspect_one(row1 + x, row2 + x, tw);
	    }
	    row1 += tw;
	    row2 += tw;
	}
	uint8_t *tmp = map1;
	map1 = map2;
	map2 = tmp;
	round++;
	if (round > 100)
	    break;
#if 1
	char *filename;
	asprintf(&filename, "/tmp/_aspect-%04d.pgm", round);
	FILE *fp = fopen(filename, "w");
	fprintf(fp, "P5\n%d %d\n255\n", tw, th);
	fwrite(map1, sizeof(uint8_t), tw * th, fp);
	fclose(fp);
#endif
    }

    /* copy map1 into  the other PIL image */
    row = map1;
    for (y = 0; y < image->ysize - 1; y++){
	for (x = 0; x < image->xsize - 1; x++){
	    sim[y][x] = row[x];
	}
	row += tw;
    }
    free(working);
    return Py_BuildValue("");
}

#endif



#define CONNECTEDNESS_DOC \
    "How connected are these two elements?  "	\
    "uses gaussian influence map."
static PyObject *
Function_connectedness(PyObject *self, PyObject *args)
{
    PixelAccessObject *pix1;
    PixelAccessObject *pix2;
    Imaging map1;
    Imaging map2;
    rect_t b1;
    rect_t b2;
    int w, h;
    int top, left, right, bottom;
    //rect_t overlap;
    if (!PyArg_ParseTuple(args, "OO(iiii)(iiii)(ii)", &pix1, &pix2,
			  &b1.left, &b1.top, &b1.right, &b1.bottom,
			  &b2.left, &b2.top, &b2.right, &b2.bottom, &w, &h))
        return NULL;

    map1 = pix1->image->image;
    map2 = pix2->image->image;
    b1.left -= INFLUENCE_RADIUS;
    b1.top -= INFLUENCE_RADIUS;
    b1.right += INFLUENCE_RADIUS;
    b1.bottom += INFLUENCE_RADIUS;
    b2.left -= INFLUENCE_RADIUS;
    b2.top -= INFLUENCE_RADIUS;
    b2.right += INFLUENCE_RADIUS;
    b2.bottom += INFLUENCE_RADIUS;

    left =   max(b1.left, b2.left);
    top =    max(b1.top, b2.top);
    right =  min(b1.right, b2.right);
    bottom = min(b1.bottom, b2.bottom);

    left =   max(left, 0);
    top =    max(top, 0); //XXX -1?
    right =  min(right, w);
    bottom = min(bottom, h);

    /*bale early if it just won't work */
    if (left >= right || top >= bottom)
	return Py_BuildValue("i", 0);

    int x, y;
    int best = 0;
    uint8_t **im1 = map1->image8;
    uint8_t **im2 = map2->image8;
    for (y = top; y < bottom; y++){
	int y1 = y - b1.top;
	int y2 = y - b2.top;
	if (y1 < 0 || y1 >= w || y2 < 0 || y2 >= h)
	    continue;
	for (x = left; x < right; x++){
	    int x1 = x - b1.left;
	    int x2 = x - b2.left;
	    if (im1[y1][x1] * im2[y2][x2] > best){
		best = im1[y1][x1] * im2[y2][x2];
	    }
	}
    }
    return Py_BuildValue("i", best);
}



#if  INFLUENCE_RADIUS == 6
//13 * 19 == 247
static uint8_t influence_matrix[INFLUENCE_DIAMETER] = {1,2,4,8,13,17,19,17,13,8,4,2,1};
#elif INFLUENCE_RADIUS == 7
//15 * 17 == 255
static uint8_t influence_matrix[INFLUENCE_DIAMETER] = {1,1,2,4,7,11,15,17,15,11,7,4,2,1,1};
#elif INFLUENCE_RADIUS == 8
//17 * 15 == 255
static uint8_t influence_matrix[INFLUENCE_DIAMETER] = {1,1,2,3,4,6,10,13,15,13,10,6,4,3,2,1,1};
#endif

static uint8_t *influence_centre = influence_matrix + INFLUENCE_RADIUS;

/* [int(520.0 / (1.0 + exp(-x/128.0))) -260 for x in range(512)]
 starts off linear, ends up almost flat */
static uint8_t influence_squash[512] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 55, 56, 57,
    58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 74, 75,
    76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 86, 87, 88, 89, 90, 91, 92, 93,
    94, 94, 95, 96, 97, 98, 99, 100, 101, 101, 102, 103, 104, 105, 106, 107, 107,
    108, 109, 110, 111, 112, 112, 113, 114, 115, 116, 116, 117, 118, 119, 120, 120,
    121, 122, 123, 124, 124, 125, 126, 127, 127, 128, 129, 130, 131, 131, 132, 133,
    134, 134, 135, 136, 137, 137, 138, 139, 139, 140, 141, 142, 142, 143, 144, 144,
    145, 146, 146, 147, 148, 149, 149, 150, 151, 151, 152, 153, 153, 154, 155, 155,
    156, 157, 157, 158, 158, 159, 160, 160, 161, 162, 162, 163, 163, 164, 165, 165,
    166, 166, 167, 168, 168, 169, 169, 170, 171, 171, 172, 172, 173, 173, 174, 175,
    175, 176, 176, 177, 177, 178, 178, 179, 179, 180, 180, 181, 181, 182, 183, 183,
    184, 184, 185, 185, 186, 186, 187, 187, 187, 188, 188, 189, 189, 190, 190, 191,
    191, 192, 192, 193, 193, 194, 194, 194, 195, 195, 196, 196, 197, 197, 198, 198,
    198, 199, 199, 200, 200, 200, 201, 201, 202, 202, 202, 203, 203, 204, 204, 204,
    205, 205, 206, 206, 206, 207, 207, 207, 208, 208, 208, 209, 209, 210, 210, 210,
    211, 211, 211, 212, 212, 212, 213, 213, 213, 214, 214, 214, 215, 215, 215, 216,
    216, 216, 216, 217, 217, 217, 218, 218, 218, 219, 219, 219, 219, 220, 220, 220,
    221, 221, 221, 221, 222, 222, 222, 223, 223, 223, 223, 224, 224, 224, 224, 225,
    225, 225, 225, 226, 226, 226, 226, 227, 227, 227, 227, 228, 228, 228, 228, 228,
    229, 229, 229, 229, 230, 230, 230, 230, 230, 231, 231, 231, 231, 232, 232, 232,
    232, 232, 233, 233, 233, 233, 233, 234, 234, 234, 234, 234, 234, 235, 235, 235,
    235, 235, 236, 236, 236, 236, 236, 236, 237, 237, 237, 237, 237, 237, 238, 238,
    238, 238, 238, 238, 239, 239, 239, 239, 239, 239, 239, 240, 240, 240, 240, 240,
    240, 241, 241, 241, 241, 241, 241, 241, 241, 242, 242, 242, 242, 242, 242, 242,
    243, 243, 243, 243, 243, 243, 243, 243, 244, 244, 244, 244, 244, 244, 244, 244,
    244, 245, 245, 245, 245, 245, 245, 245, 245, 245, 246, 246, 246, 246, 246, 246,
    246, 246, 246, 247, 247, 247, 247, 247, 247, 247, 247, 247, 247, 247, 248, 248,
    248, 248, 248, 248, 248, 248, 248, 248, 248, 249, 249, 249, 249, 249, 249, 249,
    249, 249, 249, 249, 249, 249, 250, 250, 250, 250, 250, 250, 250, 250
};

/*helper for influence_map */
static inline void
spread_influence(uint8_t *row, int x){
    int i;
    for (i =  - INFLUENCE_RADIUS; i <=  INFLUENCE_RADIUS; i++){
	row[x + i] += influence_centre[i];
    }
}

#define INFLUENCE_MAP_DOC \
    "alter a mask of the sprite to indicate how far from the "	\
    "sprite each nearby point is, using a kind of gaussian blur. "

static PyObject *
Function_influence_map(PyObject *self, PyObject *args)
{
    PixelAccessObject *mask_pix;
    PixelAccessObject *map_pix;
    int x, y, i;
    Imaging mask;
    Imaging map;
    if (!PyArg_ParseTuple(args, "OO", &mask_pix, &map_pix))
        return NULL;
    mask = mask_pix->image->image;
    map = map_pix->image->image;
    int w = mask->xsize;
    int h = mask->ysize;
    int tw = w + INFLUENCE_DIAMETER + 2;
    int th = h + INFLUENCE_DIAMETER;
    int alloc_h = th + INFLUENCE_DIAMETER * 2 + 1;
    uint8_t *tmp_alloc = calloc(tw * alloc_h, 1);
    if (! tmp_alloc)
	return NULL;

    /*set zero point far enough in for negative values */
    uint8_t *tmp_outer = tmp_alloc + INFLUENCE_DIAMETER + INFLUENCE_RADIUS * tw;
    uint8_t *tmp_inner = tmp_outer + INFLUENCE_RADIUS + INFLUENCE_RADIUS * tw;
    uint8_t **mask8 = mask->image8;
    uint8_t **map8 = map->image8;

    uint8_t *row = tmp_inner;
    for (y = 0; y < h; y++){
	for (x = 0; x < w; x++){
	    if (mask8[y][x]){
		spread_influence(row, x);
	    }
	}
	row += tw;
    }

    for (y = 0; y < map->ysize; y++){
	for (x = 0; x < map->xsize; x++){
	    int sum = 0;
	    for (i = - INFLUENCE_RADIUS; i <= INFLUENCE_RADIUS; i++){
		sum += tmp_outer[(y + i) * tw + x] * influence_centre[i];
	    }

	    //tmp8 should be up to INFLUENCE_DIAMETER * max(influence_matrix)
	    // which is just under 256
	    //sum can be max(tmp8) * INFLUENCE_DIAMETER * max(influence_matrix)
	    // -> under 2 ** 16.  so >> 8 is proper, but as it never really gets that
	    //high, and the lower values are the interesting one, keep detail in the shadows.
	    //Use a nonlinear transform.
	    if (sum < 4096)
		map8[y][x] = influence_squash[sum >> 3];
	    else if (sum < 12 * 1024)
		map8[y][x] = 249 + (sum >> 11);
	    else
		map8[y][x] = 255;
	}
    }
    free(tmp_alloc);

    return Py_BuildValue("");
}


#if 0

#define FIND_APPENDAGES_DOC \
    "Walk inwards from points on the convex hull, "\
    "until a suitable hinge is found."

#define APPENDAGE_MIN 4
#define APPENDAGE_MAX 30

/* grow an appndage using a wavefront, stopping when the front has reached far
   enough and is widening */

static inline int
grow_appendage(int x, int y, Imaging src, Imaging dest)
{
    int width = src->xsize;
    int height = src->ysize;
    uint8_t **im = src->image8;
    uint8_t **im2 = dest->image8;
    img_point_t *starts = malloc( 3 * width * height * sizeof(img_point_t));
    img_point_t *nexts = starts + width * height;
    //img_point_t *candidates = nexts + width * height;
    int n_starts = 1;
    int n_nexts;
    starts[0].x = x;
    starts[0].y = y;
    im2[y][x] = 200;
    int dx[4] = {-1, 1, 0, 0};
    int dy[4] = {0, 0, -1, 1};
    int rounds = 0;
    while (n_starts){
	n_nexts = 0;
	for (int i = 0; i < n_starts; i++){
	    for (int j = 0; j < 4; j++){
		x = starts[i].x + dx[j];
		y = starts[i].y + dy[j];
		if (y >= 0 && y < height &&
		    x >= 0 && x < width &&
		    im[y][x]){
		    im2[y][x] = 150;
		    nexts[n_nexts].x = x;
		    nexts[n_nexts].y = y;
		    n_nexts++;
		}
	    }
	}
	img_point_t *tmp = starts;
	starts = nexts;
	nexts = tmp;
	n_starts = n_nexts;
	rounds ++;

    }
}

static PyObject *
Function_find_appendages(PyObject *self, PyObject *args)
{
    PixelAccessObject *pix = NULL;
    PixelAccessObject *pix2 = NULL;


    if (!PyArg_ParseTuple(args, "OO", &pix, &pix2))
        return NULL;

    Imaging map = pix->image->image;
    Imaging map2 = pix2->image->image;
    int rows = map->ysize;

    int *lefts = malloc(rows * 2 * sizeof(int));
    if (!lefts)
	return PyErr_NoMemory();
    memset(lefts, -1, rows * 2 * sizeof(int));
    int *rights = lefts + rows;

    /* find the leftmost and rightmost points in each row */
    find_row_extrema(map->image8, lefts, rights, map->xsize, rows);

    int i;
    double xl = lefts[0] + 0.5;
    double xr = rights[0] + 0.5;
    double dxl = 0;
    double dxr = 0;
    int il = 0;
    int ir = 0;
    int count = 0;
    for(i = 0; i < rows; i++){
	/*XXX look at combining this with other implementations of convex hull*/
	if (i == il){
	    count += grow_appendage(lefts[i], i, map, map2);
	}
	if (i == ir){
	    count += grow_appendage(rights[i], i, map, map2);
	}

	if (i == il && i != rows - 1){ /*time to look for a new direction */
	    xl = (double)lefts[i] + 0.5;
	    find_outermost_angle(lefts, rows, i, LEFT_CONVEXITY, &il, &dxl);
	}
	if (i == ir && i != rows - 1){ /*same again for other side */
	    xr = (double)rights[i] + 0.5;
	    find_outermost_angle(rights, rows, i, RIGHT_CONVEXITY, &ir, &dxr);
	}

	xl += dxl;
	xr += dxr;
    }
    free(lefts);
    return Py_BuildValue("i", count);
}

#endif



/**********************************************************************/
/* method binding structs                                             */
/**********************************************************************/
/* bindings for top_level */
static PyMethodDef top_level_functions[] = {
    {"get_element", (PyCFunction)Function_get_element, METH_VARARGS,
    GET_ELEMENT_DOC},
    {"grow_background", (PyCFunction)Function_grow_background, METH_VARARGS,
    "fill in the bits where the background was obscured by sprites "},
    {"find_background", (PyCFunction)Function_find_background, METH_VARARGS,
    "make a three dimensional histogram, return a sorted list of the most popular colours "},
    {"fast_median", (PyCFunction)Function_fast_median, METH_VARARGS,
     "Set each pixel of second image to be the median value of the "	\
     "corresponding pixel and its immediate neighbours, of the first pixel."},
    {"expand_region", (PyCFunction)Function_expand_region, METH_VARARGS,
    "expand a region with pixels near to <colour>, varying by at most "\
    "<threshold> from each other, and <abs_threshold> from <colour> "},
    {"surface_area", (PyCFunction)Function_surface_area, METH_VARARGS,
     SURFACE_AREA_DOC},
    {"intersect", (PyCFunction)Function_intersect, METH_VARARGS,
     INTERSECT_DOC},
    {"convex_hull_size", (PyCFunction)Function_convex_hull_size, METH_VARARGS,
     CONVEX_HULL_SIZE_DOC},
    {"connectedness", (PyCFunction)Function_connectedness, METH_VARARGS,
    CONNECTEDNESS_DOC},
    {"solidity", (PyCFunction)Function_solidity, METH_VARARGS,
    SOLIDITY_DOC},
    {"scramble", (PyCFunction)Function_scramble, METH_VARARGS,
    SCRAMBLE_DOC},
    {"symmetry", (PyCFunction)Function_symmetry, METH_VARARGS,
     SYMMETRY_DOC},
    {"obviousness", (PyCFunction)Function_obviousness, METH_VARARGS,
     OBVIOUSNESS_DOC},
#if 0
    {"aspect", (PyCFunction)Function_aspect, METH_VARARGS,
    ASPECT_DOC},
#endif
    {"influence_map", (PyCFunction)Function_influence_map, METH_VARARGS,
    INFLUENCE_MAP_DOC},
    {NULL}
};



/**********************************************************************/
/* initialisation.                                                    */
/**********************************************************************/
#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initimagescan(void)
{
    PyObject* m;
    m = Py_InitModule3("imagescan", top_level_functions,
        "Faster routines to find things in images.");

    if (m == NULL){
	debug("problems initialising imagescan!\n");
        return;
    }

    int i = (PyModule_AddIntConstant(m, "INFLUENCE_RADIUS", INFLUENCE_RADIUS) ||
	     PyModule_AddIntConstant(m, "INFLUENCE_DIAMETER", INFLUENCE_DIAMETER));
    if (i < 0){
	debug("can't add things to module!\n");
	return;
    }
}
