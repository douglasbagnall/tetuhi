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

#include <time.h>
#include <stddef.h>
#include <stdlib.h>

#include "libperceptron.h"

//void nn_rng_init(unsigned int seed)
//void nn_rng_maybe_init(unsigned int seed)
//inline int nn_rng_uniform_int(int limit)
//inline double nn_rng_uniform_double(double limit)

#define ITERATIONS 100000000

int test_doubles(void){
    nn_rng_init(3);
    int i, j;
    double total, t;
    double ranges[3] = { 1.0, 2.0, 2.71828};


    for (j = 0; j < 3; j++){
	double top = ranges[j];
	printf("trying %d iterations, (0 to %f)", ITERATIONS, top);
	total = 0;
	t = clock();
	for (i = 0; i < ITERATIONS; i++){
	    total += nn_rng_uniform_double(top);
	}
	printf("... took %f seconds\n", (clock() - t)/CLOCKS_PER_SEC);
	printf("average is: %f\n", total / ITERATIONS);
    }
    return 0;
}

int test_ints(void){
    nn_rng_init(3);
    unsigned int i, j, total;
    double t;
    double ranges[3] = { 1, 2, 3};

    for (j = 0; j < 3; j++){
	int top = ranges[j];
	printf("trying %d iterations, (0 to %d)", ITERATIONS, top);
	total = 0;
	t = clock();
	for (i = 0; i < ITERATIONS; i++){
	    total += nn_rng_uniform_int(top);
	}
	printf("... took %f seconds\n", (clock() - t)/CLOCKS_PER_SEC);
	printf("average is: %f\n", ((double)total) / ITERATIONS);
    }
    return 0;
}




int main(){
    debug("hello\n");
    test_doubles();
    test_ints();
    return 0;
}
