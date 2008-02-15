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
#ifndef _GOT_LIBGAMEMAP_H
#define _GOT_LIBGAMEMAP_H 1

#include <stdint.h>
#include <stddef.h>
#include <math.h>

/*optimisation rules. these can be set with -D*/
#ifndef LINESCAN
#define LINESCAN 1
#endif
#ifndef LOWRES
#define LOWRES 1
#endif
#ifndef FAST_SWEEP
#define FAST_SWEEP 1
#endif

#define SQUARE_LOWRES 1
#define VERBOSE 0
#define DEBUG_IMG 0
#define DEBUG_POINTS 0
#define SAVE_MANY_PGMS DEBUG_IMG


#include "dbdebug.h"

#define MAX_ELEMENTS 4096

//typedef uint32_t bitmap_t;
typedef uint8_t bitmap_t;

typedef struct tt_point_s {
    int x;
    int y;
} tt_point_t;

typedef struct tt_element_s{
    int width;
    int true_width; /*for detecting edge of stage */
    int height;
    bitmap_t *image_mem;     
    bitmap_t *image[4];
    bitmap_t *imagemask[4];
    int bit;
    bitmap_t mask;
} tt_element_t;

typedef struct tt_gamemap_s{
    bitmap_t* memory;
    bitmap_t* map;
    bitmap_t* lowres;
#if LINESCAN
    bitmap_t* linescan;
#endif
    uint32_t block_bits;
    int prescan_dirty;
    size_t lowres_w;
    size_t lowres_h;
    int width;
    int padded_width;
    int height;
    tt_element_t elements[MAX_ELEMENTS]; /*how many? maybe should be configurable at
					   set up time, or auto-growing*/
    int element_n;
#if  DEBUG_IMG
    bitmap_t* dbimg;
    int dbimg_shade;
#endif
    bitmap_t used_bits;
} tt_gamemap_t;


/* gamemap.c */
tt_gamemap_t *tt_new_map(unsigned int width, unsigned int height);
void tt_delete_map(tt_gamemap_t *gm);
int tt_add_element(tt_gamemap_t *map, void *bitmap, int bit, int width, int height, int bitsize, int decimate);
void tt_set_figure(tt_gamemap_t *gm, int shape, int x, int y);
void tt_clear_figure(tt_gamemap_t *gm, int shape, int x, int y);
void tt_save_map(tt_gamemap_t *gm, char *filename, int format, int padding);
void tt_clear_map(tt_gamemap_t* gm);
void tt_prescan_clear(tt_gamemap_t* gm);
void tt_print_options(void);
bitmap_t tt_search_region(tt_gamemap_t* gm, tt_point_t points[], int n_points);

void tt_scan_zones(tt_gamemap_t* gm, bitmap_t *finds, int rect[4], 
		   int zones[], unsigned int n_zones, int orientation, int verbose);

bitmap_t tt_touching_figure(tt_gamemap_t* gm, int shape, int x, int y);


#define BITMAP_OF_BITS  0
#define BITMAP_OF_BYTES 1 
#define BITMAP_OF_INTS  2 


#define BITMAP_EDGE_BIT 0
#define BITMAP_EDGE_MASK (1 << BITMAP_EDGE_BIT)


#define MAX_MAP_SIZE (8192 * 8192)

#define POINTS_MAX 12
#define POINTS 8
#define ZONES_MAX 4
#define ZONES_SEGMENTS 8

#define FAST_BLIT_SIZE 11
#define FAST_SWEEP_SIZE 11
#define SUPERFAST_SWEEP 128

#define MAP_ROW_PADDING 16
#define MAP_TOP_PADDING 8


#ifndef LOWRES_BITS
#define LOWRES_BITS 3
#endif
#define LOWRES_SIZE 1 << LOWRES_BITS

#define IMAGE_FORMAT_RAW 0
#define IMAGE_FORMAT_PGM 1

#define IMAGE_OFFSET 4

#ifndef min
#define min(a, b) ((a < b) ? (a): (b))
#endif
#ifndef max
#define max(a, b) ((a > b) ? (a): (b))
#endif


//round up if at all necessary (unsigned so compiler can shift to divide)
#define CEIL_DIV(x, div) (((unsigned)(x) + (div) - 1) / (div))
#define FLOOR_DIV(x, div) (((unsigned)(x)) / (div))














#endif
