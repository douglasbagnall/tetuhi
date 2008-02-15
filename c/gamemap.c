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
//#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
//#include <math.h>
//#include <time.h>


tt_gamemap_t *
tt_new_map(unsigned int width, unsigned int height)
{
    tt_gamemap_t *gm = calloc(1, sizeof(tt_gamemap_t));
    if (!gm)
	goto error1;

    if (width * height > MAX_MAP_SIZE){
	debug("width %d, height %d are insanely big", width, height);
	goto error2;
    }
    gm->width = width;
    gm->height = height;
    gm->padded_width = width + MAP_ROW_PADDING;
    /*XXX maybe should use memalign */
    gm->memory = calloc(gm->padded_width * (height + 2 * MAP_TOP_PADDING), sizeof(bitmap_t));
    if (! gm->memory)
	goto error2;
    gm->map = gm->memory + MAP_TOP_PADDING * gm->padded_width;
#if LOWRES
    gm->lowres_w = (width >> LOWRES_BITS);
    gm->lowres_h = (height >> LOWRES_BITS);
    gm->lowres = calloc(8 + gm->lowres_w * gm->lowres_h, 1);
    if (! gm->lowres)
	goto error3;
#endif
#if LINESCAN
    gm->linescan = calloc(8 + height, sizeof(bitmap_t));
    if (! gm->linescan)
	goto error4;
#endif

#if DEBUG_IMG
    gm->dbimg = calloc(gm->padded_width * height, sizeof(bitmap_t));
    gm->dbimg_shade = 60;
    if (! gm->dbimg)
	goto error7;
#endif
    return gm;

    /* error paths*/
#if DEBUG_IMG
  error7:
#endif
#if LINESCAN
    free(gm->linescan);
  error4:
#endif
#if LOWRES
    free(gm->lowres);
  error3:
#endif
    free(gm->memory);
  error2:
    free(gm);
  error1:
    debug("trouble creating map. width is %d, height is %d.\n", width, height);
    debug("gm: %p, gm->memory: %p gm->map: %p, gm->lowres: %p\n", gm, gm->memory, gm->map, gm->lowres);
    return NULL;
}

void
tt_delete_map(tt_gamemap_t *gm)
{
    int i;
    for (i = 0; i < gm->element_n; i++){
	free(gm->elements[i].image_mem);
    }
    free(gm->memory);
#if LOWRES
    free(gm->lowres);
#endif
#if LINESCAN
    free(gm->linescan);
#endif
#if DEBUG_IMG
    FILE *fp = fopen("/tmp/db_img.pgm", "w");
    fprintf(fp, "P5\n%d %d\n255\n", gm->padded_width, gm->height);
    fwrite(gm->dbimg, sizeof(bitmap_t), gm->padded_width * gm->height, fp);
    fclose(fp);
    free(gm->dbimg);
#endif
    free(gm);
}

static inline void
save_bitmap(bitmap_t* map, char* filename, int padded_width,
	    int width, int height, int format)
{
    int i;
    FILE *f = fopen(filename, "w");
    if (format == IMAGE_FORMAT_PGM){
	fprintf(f, "P5\n%d %d\n255\n", width, height);
    }
    for (i = 0; i < height; i++){
	fwrite(map + i * padded_width, sizeof(bitmap_t), width, f);
    }
    fclose(f);
}


/*save the map in a raw format. options:
  format: (0)RAW_IMAGE,  (1)PGM_IMAGE
  padding: true to get padding
*/

void
tt_save_map(tt_gamemap_t* gm, char *filename, int format, int padding){
    int width = padding ? gm->padded_width : gm->width;
    save_bitmap(gm->map, filename, gm->padded_width, width, gm->height, format);
#if 1
#if LOWRES
    char *filename2;
    asprintf(&filename2, "%s-lowres.pgm", filename);
    save_bitmap(gm->lowres, filename2, gm->lowres_w, gm->lowres_w, gm->lowres_h, format);
#endif
#if LINESCAN
    char *filename3;
    asprintf(&filename3, "%s-line.pgm", filename);
    save_bitmap(gm->linescan, filename3, 1, 1, gm->height, format);
#endif
#endif
}

/* clear everything off -- and clear the dirty flag */
void
tt_prescan_clear(tt_gamemap_t* gm){
#if LOWRES
    memset(gm->lowres, 0, gm->lowres_w * gm->lowres_h * sizeof(bitmap_t));
#endif
#if LINESCAN
    memset(gm->linescan, 0, gm->height * sizeof(bitmap_t));
#endif
    gm->prescan_dirty = 0;
}

/* clear everything off -- and clear the dirty flag */
void
tt_clear_map(tt_gamemap_t* gm){
    memset(gm->memory, 0, sizeof(bitmap_t) * gm->padded_width * (gm->height + 2 * MAP_TOP_PADDING));
#if LOWRES || LINESCAN
    tt_prescan_clear(gm);
#endif
}


/*add an element to the store.  This should only happen once per element, at
  the beginning of the game, so as much processing as possible should be done
  here.

  bitmap:   the raw image data
  bit:      the bit that the image uses
  bitsize:  indicates the image format
  decimate: how much to reduce the scale (shift this many bits right)
*/

int
tt_add_element(tt_gamemap_t* gm, void* bitmap, int bit, int width,
	       int height, int bitsize, int decimate)
{
    int offset = gm->element_n;
    tt_element_t *e;
    if (bit < 0 || bit >= sizeof(bitmap_t) * 8){
	debug("can't set bit %d (try 0-7)\n", bit);
	return -1;
    }
    if (offset >= MAX_ELEMENTS) {
	debug("no more room for bitmaps!\n");
	return -1; //XXX should just realloc
    }
    if (bitsize != BITMAP_OF_BYTES){
	/*probably useless check */
	debug("unknown bitmap format (try 1 byte per pixel)\n");
	return -1;
    }
    /* width is rounded up for quicker blitting. A few zero bytes are reserved
       before the image, so it can be copied in 4 bytes at a time, aligned at
       the other end.

       could keep 4 copies, offset so fully aligned copy is always
       possible
    */
    e = gm->elements + offset;
    e->true_width = CEIL_DIV(width, 1 << decimate);
    e->width = (e->true_width + 2 * IMAGE_OFFSET - 1) & ~(IMAGE_OFFSET - 1);
    e->height = CEIL_DIV(height, 1 << decimate);
    e->bit = bit;
    e->mask = 1 << bit;
    size_t imsize = e->width * e->height;
    size_t memsize = imsize + 2 * IMAGE_OFFSET;
    size_t mem_wanted = 8 * memsize + 2 * IMAGE_OFFSET;

    e->image_mem = calloc(mem_wanted, sizeof(bitmap_t));
    if (! e->image_mem){
	debug("no memory for bitmap\n");
	return -1;
    }
#if 0
    debug("width: orig %d, true %d, used: %d, height %d,"
	  " imsize %d, memsize %d, total alloc'd %d\n", width, 
	  e->true_width, e->width, e->height, imsize, memsize, mem_wanted);
#endif
    e->image[0] = e->image_mem + IMAGE_OFFSET;
    e->image[1] = e->image[0] + memsize;
    e->image[2] = e->image[1] + memsize;
    e->image[3] = e->image[2] + memsize;
    e->imagemask[0] = e->image[3] + memsize;
    e->imagemask[1] = e->imagemask[0] + memsize;
    e->imagemask[2] = e->imagemask[1] + memsize;
    e->imagemask[3] = e->imagemask[2] + memsize;

    /*more complicated than a linear copy
      - width is padded to 4
      - need to decimate
    */
    int x, y;
    for(y = 0; y < height; y++){
	for (x = 0; x < width; x++){
	    if (((char *)bitmap)[y * width + x]){
		int p = (y >> decimate) * e->width + (x >> decimate);
		e->image[0][p] = e->mask;
		e->imagemask[0][p] = 0xff ;
	    }
	}
    }
    memcpy(e->image[1] + 1, e->image[0], imsize);
    memcpy(e->image[2] + 2, e->image[0], imsize);
    memcpy(e->image[3] + 3, e->image[0], imsize);
    memcpy(e->imagemask[1] + 1, e->imagemask[0], imsize);
    memcpy(e->imagemask[2] + 2, e->imagemask[0], imsize);
    memcpy(e->imagemask[3] + 3, e->imagemask[0], imsize);

    //debug("element %p internal size is (%d, %d)\n", &e, e->width, e->height);
#if SAVE_MANY_PGMS 

    int i, j;
    char *filename;
    asprintf(&filename, "/tmp/element_%03d.pgm", offset);
    FILE *f = fopen(filename, "w");
    fprintf(f, "P5\n%d %d\n255\n", e->width, e->height * 4);

    for (j = 0; j < 4; j++){
	for (i = 0; i < height; i++){
	    fwrite(e->imagemask[j] + i * e->width, sizeof(bitmap_t), e->width, f);
	}
    }
    fclose(f);

#endif

    gm->element_n++;
    /*keep track of which bits are used, to short circuit searches for impossible bits */
    gm->used_bits |= e->mask;
    return offset;
}


#if LOWRES
/*blit the thing into the lowres map.  This is a little inexact, due to the
  predetermined downsampled shape.
*/
static inline void
lowres_blit(tt_gamemap_t *gm, tt_element_t *e, uint32_t x, uint32_t y){
    int ex = min(x + e->width, gm->width - 1);
    int ey = min(y + e->height, gm->height - 1);
    x >>= LOWRES_BITS;
    y >>= LOWRES_BITS;
    ex >>= LOWRES_BITS;
    ey >>= LOWRES_BITS;
    int ix, iy;
    for (iy = y; iy <= ey; iy++){
	for (ix = x; ix <= ex; ix++){
	    gm->lowres[iy * gm->lowres_w + ix] |= e->mask;
	}
    }
}
#endif



/* these next two function duplicate a terrible amount of code, but it is hard
   to avoid without rather icky macros, given that the difference is an
   operation in the innermost loop.
*/

void
tt_set_figure(tt_gamemap_t* gm, int shape, int x, int y){
    if (shape >= gm->element_n){
	debug("tried to blit an invalid element (%d, ceiling is %d)\n", shape, gm->element_n);
	return;
    }
    int ix, iy;
    tt_element_t *e = gm->elements + shape;
    int offset = x & 3;
    int h = min(e->height, gm->height - y);
    int w = min(e->width, gm->width - x);

    int32_t *map32 = (int32_t *)(gm->map + y * gm->padded_width + x - offset);
    int32_t *img32 = (int32_t *)(e->image[offset]);
    /* blit can run over right edge, so round up */
    int w32 = CEIL_DIV(w, 4);
    int mapw32 = gm->padded_width >> 2;
    int imgw32 = e->width >> 2;

    for (iy = 0; iy < h; iy++){
	for (ix = 0; ix < w32; ix++){
	    map32[ix] |= img32[ix];
	}
	img32 += imgw32;
	map32 += mapw32;
#if LINESCAN
	/*indicates this bit is *probably* set in this line */
	gm->linescan[y + iy] |= e->mask;

#if 0
	int j;
	bitmap_t f = 0;
	for (j = 0; j < gm->width; j++){
	    f |= gm->map[gm->padded_width * (y+iy) + j];
	}
	debug("%s x%3d, y%3d, mask:%2x, line %d linescan is %2x, "
	      "in line: %2x, previously: linescan %2x, in line %2x\n",
	      (gm->linescan[y + iy] != f) ? "*": " ",
	      x,y, e->mask, y + iy, gm->linescan[y + iy], f, ls, f2);
#endif
#endif
    }

#if LOWRES
    lowres_blit(gm, e, x, y);
#if 1
    int x2 = x >> LOWRES_BITS;
    int y2 = x >> LOWRES_BITS;
    int h2 = min(h >> LOWRES_BITS, gm->lowres_h);
    for (iy = y2; iy < y2 + h2; iy++){
	if (! gm->lowres[y2 + gm->lowres_w + x2] & e->mask){
	    debug("missing one at %d,%d\n", x2,y2);
	}
    }
#endif
#endif
}

void
tt_clear_figure(tt_gamemap_t* gm, int shape, int x, int y){
    if (shape >= gm->element_n){
	debug("tried to clear an invalid element (%d, ceiling is %d)\n", shape, gm->element_n);
	return;
    }
    int ix, iy;
    tt_element_t *e = gm->elements + shape;
    int offset = x & 3;
    bitmap_t *bm = e->image[offset];
    bitmap_t *imgrow = bm;
    bitmap_t *maprow = gm->map + x + y * gm->padded_width;
    int h = min(e->height, gm->height - y);
    int w = min(e->width, gm->width - x);

    int32_t *map32 = (int32_t *)(maprow - offset);
    int32_t *img32 = (int32_t *)imgrow;

    int w32 = CEIL_DIV(w, 4);
    int mapw32 = gm->padded_width >> 2;
    int imgw32 = e->width >> 2;
    for (iy = 0; iy < h; iy++){
	for (ix = 0; ix < w32; ix++){
	    map32[ix] &= ~img32[ix];
	}
	img32 += imgw32;
	map32 += mapw32;
    }
#if LINESCAN || LOWRES
    /* the prescan no longer looks like the main picture -- on the other hand,
       it will not give a false idea if it is only used for elimination */
    gm->prescan_dirty = 1;
#endif
}


/*find the things that are touching the named figure.  If the figure has not
  been cleared, it will of course be touching itself.

XXX could use lowres, linescan etc.  Could also use mmx, which obviates the
need for e->imagemask (because packed compare can generate the necessary mask).

 */

bitmap_t
tt_touching_figure(tt_gamemap_t* gm, int shape, int x, int y){
    if (shape >= gm->element_n){
	debug("tried to scan an invalid element (%d, ceiling is %d)\n", shape, gm->element_n);
	return 0;
    }
    int ix, iy;
    tt_element_t *e = gm->elements + shape;
    int offset = x & 3;
    uint32_t f = 0;

    int h = min(e->height, gm->height - y);
    int w = min(e->width, gm->width - x);
    if (h != e->height || x + e->true_width >= gm->width || x < 0 || y < 0){
	f |= BITMAP_EDGE_MASK;
#if 0
	debug("apparently hit edge: h: %d, e->h %d, w: %d, e->w: %d, x:%d, y:%d, gm->w: %d, gm->h: %d\n",
	      h, e->height, w, e->width, x, y, gm->width, gm->height);
#endif
    }

    /*XXX added  at last minute. not quite correct.*/
    if (x < 0)
	x = 0;
    if (y < 0)
	y = 0;

    int32_t *map32 = (int32_t *)(gm->map + y * gm->padded_width + x - offset);
    int32_t *img32 = (int32_t *)(e->imagemask[offset]);

    int w32 = CEIL_DIV(w, 4);
    int mapw32 = gm->padded_width >> 2;
    int imgw32 = e->width >> 2;

    for (iy = 0; iy < h; iy++){
	for (ix = 0; ix < w32; ix++){
	    f |= (map32[ix] & img32[ix]);
	}
	img32 += imgw32;
	map32 += mapw32;
    }
    f |= f >> 16;
    f |= f >> 8;
    return f;
}




void
tt_print_options(void){
    printf("using ");
#if LINESCAN
    printf("LINESCAN  ");
#endif
#if LOWRES
    printf("LOWRES: %d  ", LOWRES_BITS);
#endif
#if FAST_SWEEP
    printf("FAST_SWEEP  ");
#endif
    printf("\n");
}
