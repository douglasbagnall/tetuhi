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

/*helper functions */

//nn_ prefix for symmetry
#define nn_tanh TANH

static inline weight_t nn_detanh(weight_t x){
    return (1.001 - x * x);
}

static inline weight_t nn_sigmoid(weight_t x){
    if (x < -300.0)
        return 0.0;
    return 1.0 / (1.0 + EXP(-x));
}

static inline weight_t nn_desigmoid(weight_t x){
    return 0.001 + x * (1.0 - x);
}


/*************** gettin an opinion: **********************
   nn_opinion successively calls calculate interlayer
 **/

inline void
nn_calculate_interlayer(nn_Interlayer_t *il){
    /*ask for answer from input nodes, multiply by weights, and hand to
     * output nodes. returns nothing.
     *
     *   ______in,x,cols____
     *  |
     * out,y,rows
     *  |
     *  |                                 */
    asm("/*64bit interlayer here */");
    int rows = il->output->insize;
    int cols = il->input->outsize;  //includes bias, if any.
    int x, y, y2;
    weight_t a;

    for (y = 0, y2 = 0; y < rows; y2 += cols, y ++){
	asm(
	    "subps %[sum], %[sum]   \n"
	    ".p2align 4,,15   \n"
	    ".Lback%=:  \n"
	    "movaps (%[inputs]), %[a]   \n"
	    "mulps (%[weights]), %[a]   \n"
	    "addps %[a], %[sum]   \n"
	    "addq  $16, %[inputs]   \n"
	    "addq  $16, %[weights]   \n"
	    "cmpq  %[inputs], %[cols] \n"
	    "jle  .Lback%= \n"
	    "pshufd      $0b01001110, %[sum], %[a]  \n"
	    "paddd       %[a], %[sum]  \n"
	    "pshufd      $0b10110001, %[sum], %[a]  \n"
	    "paddd       %[a], %[sum]  \n"
	    "movss        %[sum],  %[total] \n"

	    : [total] "=f" (a)
	    : [cols] "r"(cols),
	      [inputs] "r"(il->input->values),
	      [weights] "r"(il->weights + y2),
	    : [sum] "xmm0", [a]"xmm1"
	    );
        il->output->values[y] = DEFORM(a);
    }
}
