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
#include "libgamemap.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define debug_points(array, n) do{		\
    for (int _i = 0; _i < (n); _i++){		\
	debug("%d,%d, ", (array)[_i].x, (array)[_i].y);	\
    }						\
    debug("\n");				\
    } while(0)

#if ASM_PRESCAN
#include "asm_scan.c"
#endif

/** first, routines to find the region to scan.  **/

/*very simple selection sort -- sorts on y axis, lowest number (top) first */
static inline void
pairsort(tt_point_t pairs[], int n)
{
    int i, j;
    for (i = 0; i < n; i++){
	int m = i;
	for (j = i + 1; j < n; j++){
	    if (pairs[j].y < pairs[m].y){
		m = j;
	    }
	}
	tt_point_t tmp = pairs[m];
	pairs[m] = pairs[i];
	pairs[i] = tmp;
    }
}

/*find the points that are on the leftmost and rightmost edges.  This is
  almost the same as finding the convex hull, but not, because horizontal
  edges are ignored/implicit.

  the left and right lists will start and end on the same row as each other,
  but not necessarily at the same point.
*/

static void
sortpoints(tt_point_t pairs[], tt_point_t lefts[], tt_point_t rights[],
	   int *lsize, int *rsize, int points_n){
    int i;
    int lp = 0;/*indexing pairs for left*/
    int rp = 0;
    int lo = 0;/*indexing lefts*/
    int ro = 0;

    /*sort in place, by increasing y */
    pairsort(pairs, points_n);

    lefts[0] = pairs[0];
    rights[0] = pairs[0];
    /*find the left- and right-most top points. Any middle points on the top
      row will be skipped.*/
    for (i = 1; i < points_n; i++){
	if (pairs[i].y == lefts[0].y){
	    if (pairs[i].x < lefts[0].x){
		lefts[0].x = pairs[i].x;
	    }
	    else if (pairs[i].x > rights[0].x){
		rights[0].x = pairs[i].x;
	    }
	}
	else {
	    rp = lp = i;
	    break;
	}
    }
    /* search for the outermost angled points. if two are the same angle, pick
       the later (lower) one.

       XXX this algorithm is closely duplicated in find__outermost_angle in ../img-c/img.c 
    */
    while(lp < points_n){
	int dx = pairs[lp].x - lefts[lo].x;
	int dy = pairs[lp].y - lefts[lo].y;
	for (i = lp + 1; i < points_n; i++){
	    int ax = pairs[i].x - lefts[lo].x;
	    int ay = pairs[i].y - lefts[lo].y;
            if (ax * dy <= dx * ay){
                dx = ax;
		dy = ay;
                lp = i;
	    }
	}
	if(dy){
	    lo++;
	    lefts[lo] = pairs[lp];
	}
	lp++;
    }

    /* rights differs in the direction of the < comparison */
    while (rp < points_n){
	int dx = pairs[rp].x - rights[ro].x;
	int dy = pairs[rp].y - rights[ro].y;
	for (i = rp + 1; i < points_n; i++){
	    int ax = pairs[i].x - rights[ro].x;
	    int ay = pairs[i].y - rights[ro].y;
            if (ax * dy >= dx * ay){
                dx = ax;
		dy = ay;
                rp = i;
	    }
	}
	if(dy){
	    ro++;
	    rights[ro] = pairs[rp];
	}
	rp ++;
    }

    *lsize = lo + 1;
    *rsize = ro + 1;
}


inline static void
scale_points(tt_point_t orig_lefts[], tt_point_t orig_rights[], int ls, int rs,
	     tt_point_t lefts[], tt_point_t rights[], int *const lsize, int *const rsize,
	     int resolution){
    //debug_points(orig_lefts, ls);
    //debug_points(orig_rights, rs);

    /*some points may have ended up reducing to the same lower resolution square */
    /*ignore the innermost ones */
    int i, j;
    int x, y;
    lefts[0].x = orig_lefts[0].x >> resolution;
    lefts[0].y = orig_lefts[0].y >> resolution;
    for (i = 1, j = 0; i < ls; i++){
	x = orig_lefts[i].x >> resolution;
	y = orig_lefts[i].y >> resolution;
	if (y == lefts[j].y && x != lefts[j].x){
	    lefts[j].x = min(lefts[j].x, x);
	}
	else{
	    j++;
	    lefts[j].y = y;
	    lefts[j].x = x;
	}
    }
    if (j == 0 && 0){
	lefts[1] = lefts[0];
	j = 1;
    }
    *lsize = j + 1;
    /* rights needs rounding up at some point, but it is done in lowres_scan, not here*/

    rights[0].x = orig_rights[0].x >> resolution;
    rights[0].y = orig_rights[0].y >> resolution;
    for (i = 1, j = 0; i < rs; i++){
	x = orig_rights[i].x >> resolution;
	y = orig_rights[i].y >> resolution;
	if (y == rights[j].y && x != rights[j].x){
	    rights[j].x = max(rights[j].x, x);
	}
	else{
	    j++;
	    rights[j].y = y;
	    rights[j].x = x;
	}
    }
    if (j == 0 && 0){
	rights[1] = rights[0];
	j = 1;
    }
    *rsize = j + 1;



}



/*** now stuff to do with actual scanning ***/



/*naive scanning routine -- a byte at a time */

inline static void
slowscan(bitmap_t *r, bitmap_t *const found, uint32_t left, uint32_t right){
    asm("/********* slowscan starts *****/\n\t");
    int i;
    //bitmap_t found = 0;
    for (i = left; i <= right; i++){
	*found |= r[i];
    }
    asm("/********* slowscan ends *****/\n\t");
    //return found;
}

/*Accelerated scanning for wide sweeps. */
//left and right are cast as unsigned for shift-division.

inline static void
fastscan(bitmap_t *r, bitmap_t *const found, uint32_t left, uint32_t right){
    asm("/********* fastscan starts *****/\n\t");
    uint32_t i;
    uint32_t *row32 = (uint32_t *)r;
    uint32_t left32 = CEIL_DIV(left, sizeof(uint32_t));
    /*add 1 to right because eg 23 is < 6*4 */
    uint32_t right32 = (right + 1) / sizeof(uint32_t);
    uint32_t f = 0;
    for (i = left32; i < right32; i++){
	f |= row32[i];
    }
    /* consolidate the 32 bits down to 8*/
    f |= f >> 16;
    f |= f >> 8;
    *found |= f;
    /* Catch the beginning and end.  This is likely to be partially redundant,
    but the loops/checks are probably more expensive.*/
    *found |= r[left];
    *found |= r[left + 1];
    *found |= r[left + 2];
    *found |= r[right - 2];
    *found |= r[right - 1];
    *found |= r[right];
    asm("/********* fastscan ends *****/\n\t");
    //return found;
}

inline static void
adaptive_scan(bitmap_t *r, bitmap_t *const found, uint32_t left, uint32_t right){
    asm("/********* adaptive_scan starts *****/\n\t");
#if FAST_SWEEP
    if (right - left > FAST_SWEEP_SIZE)
	fastscan(r, found, left, right);
    else
	slowscan(r, found, left, right);
#else
    slowscan(r, found, left, right);
#endif
    asm("/********* adaptive_scan ends *****/\n\t");
}



/*find the place at which the line between two points crosses y = zero.
This is useful in eliminating areas above the top of the map.

Numerically unstable! (integer division), but it shouldn't matter too much in
practice.
*/

inline static tt_point_t find_zero_crossing(tt_point_t a, tt_point_t b) __attribute__((const));

inline static tt_point_t
find_zero_crossing(tt_point_t a, tt_point_t b){
    asm("/********* find zero start *****/\n\t");
    tt_point_t c;
    if (a.y == b.y) // no zero crossing! nothing sensible to do. it won't happen, though.
	return a;
    int top =  a.y * b.x - a.x * b.y;
    int bottom = a.y - b.y;
    c.y = 0;
    c.x = top / bottom;
    asm("/********* find zero end *****/\n\t");
    return c;
}

inline static bitmap_t
square_lowres_scan(tt_gamemap_t *gm, int top, int left, int bottom, int right, bitmap_t expected){
    int row;
    bitmap_t *map = gm->lowres;
    int width = gm->lowres_w;
    bitmap_t found = expected & BITMAP_EDGE_MASK; //push walls through

    top >>= LOWRES_BITS;
    bottom >>= LOWRES_BITS;
    left >>= LOWRES_BITS;
    right >>= LOWRES_BITS;
    left = max(left, 0);
    right = min(right, width -1);
    //debug ("top %d, bottom %d, left %d, right %d\n", top, bottom, left, right);
    for (row = top; row <= bottom; row++){
	slowscan(map + width * row, &found, (uint32_t)left, (uint32_t)right);
	if (found == expected){
	    break;	
	}
    }
    //debug("expected %2x, found %2x\n", expected, found);
    return found;
}


inline static bitmap_t
lowres_scan(tt_gamemap_t *gm, tt_point_t hr_lefts[], tt_point_t hr_rights[], int hr_lsize, int hr_rsize,
	    bitmap_t expected){
    tt_point_t lefts[POINTS_MAX]; //XXX could be hr_lsize
    tt_point_t rights[POINTS_MAX];
    int lsize;
    int rsize;
    int row;
    bitmap_t *map = gm->lowres;
    int width = gm->lowres_w;
    bitmap_t found = expected & BITMAP_EDGE_MASK;
    scale_points(hr_lefts, hr_rights, hr_lsize, hr_rsize,
		 lefts, rights, &lsize, &rsize, LOWRES_BITS);

    //debug_points(lefts, lsize);
    //debug_points(rights, rsize);


    double xl = xl;
    double xr = xr;
    double dxl = 0;
    double dxr = 0;
    int il = 0;
    int ir = 0;
    for (row = lefts[0].y; row <= lefts[lsize - 1].y; row++, xl += dxl, xr += dxr){
	asm("/********* scan loop starts *****/\n\t");
	/* check for turning points */
        if (row == lefts[il].y && il < lsize){
	    xl = lefts[il].x;
	    il++;
	    if (il < lsize){ // don't want to accidentally divide by zero on last one
		dxl = (double)(xl - lefts[il].x) / (row - lefts[il].y);
	    }
	}
        if (row == rights[ir].y && ir < rsize){
	    xr = rights[ir].x;
	    ir++;
	    if (ir < rsize)
		dxr = (double)(xr - rights[ir].x) / (row - rights[ir].y);
	}

	int _xl = (int)xl;
	int _xr = (int)xr + 1;

	if (_xl < width && _xr >= 0){
	    if (_xl < 0){
		_xl = 0;
	    }
	    if (_xr >= width){
		_xr = width - 1;
	    }
	    slowscan(map + width * row, &found, (uint32_t)_xl, (uint32_t)_xr);
	    //debug("after row %d, found is %x, expected %x\n", row, found, expected);
	    if (found == expected){
		break;
	    }
	}
    }
#if DEBUG_POINTS
    debug("lowres scan found %x (down from %x) in points:\n", found, expected);
    debug_points(lefts, lsize);
    debug_points(rights, rsize);
#endif

    return found;
}



/* find which bits are set within the polygon defined by the arrays lefts[]
   and rights[].  These arrays are corner points on respective sides. They
   must both start and end on the same row (y coordinate), and be in ascending
   Y order.  It its only designed for convex polygons, but some concave ones
   will also work.
*/

static bitmap_t
scan(tt_gamemap_t *gm, tt_point_t lefts[], tt_point_t rights[], int lsize, int rsize,
     bitmap_t expected){
    bitmap_t *map = gm->map;
    int width = gm->width;
    int height = gm->height;
    bitmap_t found = 0;
    int i;
    int row;
    int top = lefts[0].y;
    int bottom = lefts[lsize - 1].y;
    int il = 0;
    int ir = 0;

    //debug ("width %d height %d\n", width, height);
    /* shortcut exits if the shape is entirely off the edge. Left and right
       exits are more complicated than top and bottom, but the reward is
       greater. */
    if (bottom < 0 || top >= height)
	return BITMAP_EDGE_MASK;

    //catch the side bits
    int leftmost = width;
    int rightmost = -1;

    for (i = 0; i < lsize; i++){
	if (lefts[i].x < leftmost){
	    leftmost = lefts[i].x;
	}
    }
    for (i = 0; i < rsize; i++){
	if (rights[i].x > rightmost){
	    rightmost = rights[i].x;
	}
    }
    if (leftmost >= width || rightmost < 0)
	return BITMAP_EDGE_MASK;

    if (leftmost < 0 || rightmost >= width)
	found = BITMAP_EDGE_MASK;


    /*clip off top bit if y < 0.

      lefts[0].y and rights[0].y both equal top. If it is < 0, look through
      the lefts and rights arrays until a 0 crossing is found, and reshape
      accordingly.

      the bottom points are guaranteed to be >= 0, due to the shortcut exit
      above, and top is < 0, so the scan can safely start at length - 2, and go
      back.
    */
    if (top < 0){
	asm("/********* scan top < 0 *****/\n\t");
	il = lsize - 2;
	ir = rsize - 2;
	while(lefts[il].y > 0)
	    il--;
	while(rights[ir].y > 0)
	    ir--;
	/* now, zero must occur somewhere between this one an the next. Set
	   the new x coordinate to correspond with a y of 0.  If this point is
	   actually on 0, the algorithm works anyway, but it would possibly be
	   quicker to special case it.
	*/
	//debug_points(lefts, lsize);
	//debug_points(rights, rsize);
	lefts[il] = find_zero_crossing(lefts[il], lefts[il + 1]);
	rights[ir] = find_zero_crossing(rights[ir], rights[ir + 1]);
	top = 0;
	found = BITMAP_EDGE_MASK;
	asm("/********* end scan top < 0 *****/\n\t");
	//debug_points(lefts, lsize);
	//debug_points(rights, rsize);
    }

    /* bottom can just be truncated */
    if (bottom >= height){
	bottom = height - 1;
	found = BITMAP_EDGE_MASK;
    }
    //expected doesn't include edges, so put them in.
    expected |= found;

#if LOWRES
    /* check to see what is in the lowres version.  The lefts and rights
     arrays may have been implicitly shifted by finding the top, just above,
     so add and subtract il, ir.*/
    if (gm->prescan_dirty < 2){
#if ! SQUARE_LOWRES
	expected = lowres_scan(gm, lefts + il, rights + ir,
			       lsize - il, rsize - ir, expected);
#else
	expected = square_lowres_scan(gm, top, leftmost, bottom, rightmost, expected);
#endif
    }
    else{
	debug("prescan is dirty!\n");
    }
#endif

    /*There are probably algorithms that can find the edges all in integers,
      but the simple line-drawing ones won't do -- they need to be able to
      switch between increasing x and y montonically, whereas here it has to
      always be y. (because we sweep each row).
     */
    double xl = lefts[il].x;
    double xr = rights[ir].x;
    double dxl = ((double)(xl - lefts[il + 1].x)) / (lefts[il].y - lefts[il + 1].y);
    double dxr = ((double)(xr - rights[ir + 1].x)) / (rights[ir].y - rights[ir + 1].y);
    il++;
    ir++;
    for (row = top; row <= bottom; row++, xl += dxl, xr += dxr){
	asm("/********* scan loop starts *****/\n\t");
	/* check for turning points */
        if (row == lefts[il].y && il < lsize){
	    xl = lefts[il].x;
	    il++;
	    if (il < lsize){ // don't want to accidentally divide by zero on last one
		dxl = (double)(xl - lefts[il].x) / (row - lefts[il].y);
	    }
	}
        if (row == rights[ir].y && ir < rsize){
	    xr = rights[ir].x;
	    ir++;
	    if (ir < rsize)
		dxr = (double)(xr - rights[ir].x) / (row - rights[ir].y);
	}

	int _xl = (int)xl;
	int _xr = (int)xr;
#if LINESCAN
	if (_xl < width && _xr >= 0 &&
	    (gm->prescan_dirty > 2 || ((found | (gm->linescan[row] & expected)) != found)))
#else
	if (_xl < width && _xr >= 0)
#endif
	{
	    if (_xl < 0){
		_xl = 0;
	    }
	    if (_xr >= width){
		_xr = width - 1;
	    }
	    adaptive_scan(map + gm->padded_width * row, &found, (uint32_t)_xl, (uint32_t)_xr);
	    if (found == expected){
		break;
	    }
	}	
    }
#if 0    
    /*XXX probably due to dirtiness outside the visible gamemap area ?*/
    if (found & ~ expected){
	debug("found %2x (%2x more than expected %2x)\n", found, found & ~ expected, expected);
    }
#endif	
#if DEBUG_POINTS
    debug("real scan found %x (down from %x) in points:\n", found, expected);
    debug_points(lefts, lsize);
    debug_points(rights, rsize);
#endif

    return found;
}


bitmap_t
tt_search_region(tt_gamemap_t* gm, tt_point_t points[], int n_points){
    bitmap_t found;
    tt_point_t lefts[POINTS_MAX]; //could be n_points
    tt_point_t rights[POINTS_MAX];
    int lsize;
    int rsize;
    bitmap_t expected = gm->used_bits;
    if (n_points > POINTS_MAX){
	// should truncate instead?
	debug("polygon is too complex (%d points); ignoring silently!\n", n_points);
	return 0;
    }

    sortpoints(points, lefts, rights, &lsize, &rsize, n_points);
    //debug_points(lefts, lsize);
    //debug_points(rights, rsize);
    //debug("\n");
    found = scan(gm, lefts, rights, lsize, rsize, expected);

    return found;
}

static inline bitmap_t
scan_quadrangle(tt_gamemap_t* gm, int x1, int y1, int x2, int y2,
		int x3, int y3, int x4, int y4 ){
    tt_point_t r[4];
    r[0].x = x1;
    r[0].y = y1;
    r[1].x = x2;
    r[1].y = y2;
    r[2].x = x3;
    r[2].y = y3;
    r[3].x = x4;
    r[3].y = y4;
    return tt_search_region(gm, r, 4);
}


void
tt_scan_zones(tt_gamemap_t* gm, bitmap_t *finds, int rect[4],
	      int zones[], unsigned int n_zones, int orientation, int verbose){

    int left = rect[0];
    int top = rect[1];
    int right = rect[0] + rect[2];
    int bottom = rect[1] + rect[3];
    int cx = rect[0] + rect[2] / 2;
    int cy = rect[1] + rect[3] / 2;
    int i, j, k;

    int tops[ZONES_MAX];
    int lefts[ZONES_MAX];
    int bottoms[ZONES_MAX];
    int rights[ZONES_MAX];

    for (i = 0;  i < n_zones; i++){
	tops[i] = top - zones[i];
	lefts[i] = left - zones[i];
	bottoms[i] = bottom + zones[i];
	rights[i] = right + zones[i];
    }

    for (i = 0; i < n_zones - 1; i++){
	j = 8 - orientation;
	k = i * 8;
	/*start at top left, inner, and go out */
	finds[k + (j & 7)] = scan_quadrangle(gm, cx, tops[i],
					     cx, tops[i+1],
					     rights[i+1], tops[i+1],
					     rights[i], tops[i]);
	j++;
	finds[k + (j & 7)] = scan_quadrangle(gm, rights[i], cy,
					     rights[i], tops[i],
					     rights[i+1], tops[i+1],
					     rights[i+1], cy);
	j++;
	//continue;
	finds[k + (j & 7)] = scan_quadrangle(gm, rights[i], cy,
					     rights[i], bottoms[i],
					     rights[i+1], bottoms[i+1],
					     rights[i+1], cy);
	j++;
	finds[k + (j & 7)] = scan_quadrangle(gm, cx, bottoms[i],
					     cx, bottoms[i+1],
					     rights[i+1], bottoms[i+1],
					     rights[i], bottoms[i]);
	j++;
	finds[k + (j & 7)] = scan_quadrangle(gm, cx, bottoms[i],
					     cx, bottoms[i+1],
					     lefts[i+1], bottoms[i+1],
					     lefts[i], bottoms[i]);
	j++;
	//continue;
	finds[k + (j & 7)] = scan_quadrangle(gm, lefts[i], cy,
					     lefts[i], bottoms[i],
					     lefts[i+1], bottoms[i+1],
					     lefts[i+1], cy);
	j++;
	finds[k + (j & 7)] = scan_quadrangle(gm, lefts[i], cy,
					     lefts[i], tops[i],
					     lefts[i+1], tops[i+1],
					     lefts[i+1], cy);
	j++;
	finds[k + (j & 7)] = scan_quadrangle(gm, cx, tops[i],
					     cx, tops[i+1],
					     lefts[i+1], tops[i+1],
					     lefts[i], tops[i]);
    }
}
