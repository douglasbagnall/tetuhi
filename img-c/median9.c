/*
 * This file is part of Te Tuhi Video Game System.
 *
 * Some code was adopted from N. Devillard
 * http://ndevilla.free.fr/
 * http://ndevilla.free.fr/median/median/node20.html
 * who said:
 *
 * "The following snippets are placed in the public domain"
 *
 * ...and also...
 *
 * "The following routines have been built from knowledge gathered
 * around the Web. I am not aware of any copyright problem with
 * them, so use it as you want.
 * N. Devillard - 1998"
 *
 * ---------------
 *
 * other parts Copyright (C) 2008 Douglas Bagnall
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


/*----------------------------------------------------------------------------
   Function :   opt_med9()
   In       :   pointer to an array of 9 pixelvalues
   Out      :   a pixelvalue
   Job      :   optimized search of the median of 9 pixelvalues
   Notice   :   in theory, cannot go faster without assumptions on the
                signal.
                Formula from:
                XILINX XCELL magazine, vol. 23 by John L. Smith

                The input array is modified in the process
                The result array is guaranteed to contain the median
                value
                in middle position, but other elements are NOT sorted.
 ---------------------------------------------------------------------------*/

/* NB: it might be possible to do better using mmx/sse, comparing 8/16 at a time... or not */

#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
#define PIX_SWAP(a,b) { uint8_t temp=(a);(a)=(b);(b)=temp; }

static inline uint8_t opt_med9(uint8_t * p)
{
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[1]) ; PIX_SORT(p[3], p[4]) ; PIX_SORT(p[6], p[7]) ;
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[3]) ; PIX_SORT(p[5], p[8]) ; PIX_SORT(p[4], p[7]) ;
    PIX_SORT(p[3], p[6]) ; PIX_SORT(p[1], p[4]) ; PIX_SORT(p[2], p[5]) ;
    PIX_SORT(p[4], p[7]) ; PIX_SORT(p[4], p[2]) ; PIX_SORT(p[6], p[4]) ;
    PIX_SORT(p[4], p[2]) ; return(p[4]) ;
}

static inline
uint8_t median9(uint8_t working[], uint8_t**in,
		int x, int y, int top, int bottom, int left, int right){
    working[0] =  in[y + top][x + left];
    working[1] =  in[y + top][x];
    working[2] =  in[y + top][x + right];
    working[3] =  in[y][x + left];
    working[4] =  in[y][x];
    working[5] =  in[y][x + right];
    working[6] =  in[y + bottom][x + left];
    working[7] =  in[y + bottom][x];
    working[8] =  in[y + bottom][x + right];
    return opt_med9(working);
}


#define MEDIAN(x, y, top, bottom, left, right)(out[(y)][(x)] = median9(working, in, (x), (y),\
								       -1 * (top), 1 * (bottom),\
								       -4 * (left), 4 *(right)))


void
blob_fast_median9(int32_t** in32, int32_t** out32, int width32, int height){
    int x, y;
    uint8_t **in = (uint8_t **)in32;
    uint8_t **out = (uint8_t **)out32;
    int width = width32 * 4;
    uint8_t working[12];
    for (y = 1; y < height - 1; y++){
	for (x = 0; x < 3; x++){ //XXX < 4 to include alpha
	    /*first column*/
	    MEDIAN(x, y,  1,1,0,1);
	    /*last*/
	    MEDIAN(x + width - 4, y,  1,1,1,0);
	}
	//central columns
	for (x = 4; x < width - 4; x++){
	    if ((x & 3) != 3){	   /*avoid processing the alpha channel */
		MEDIAN(x, y,  1,1,1,1);
	    }
	}
    }

    //top and bottom rows
    for (x = 4; x < width - 4; x++){
	if ((x & 3) != 3){
	    MEDIAN(x, 0,  0,1,1,1);
	    MEDIAN(x, height - 1,  1,0,1,1);
	}
    }

    //corners
    for (x = 0; x < 3; x++){ //XXX < 4 to include alpha
	MEDIAN(x, 0,  0,1,0,1);
	MEDIAN(x, height - 1, 1,0,0,1);
	MEDIAN(width - 4 + x, 0, 0,1,1,0);
	MEDIAN(width - 4 + x, height - 1, 1,0,1,0);
    }

}
