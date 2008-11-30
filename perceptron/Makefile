# Copyright (C) 2008 Douglas Bagnall
#
# This file is part of Te Tuhi Video Game System, or Te Tuhi for short.
#
# Te Tuhi is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# Te Tuhi is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Te Tuhi.  If not, see <http://www.gnu.org/licenses/>.
#
# see the README file in the perceptron directory for information
# about possible future licensing.

BASENAME = perceptron

PY_VERSION = $(shell python -c 'import sys; print sys.version[:3]')
PYTHON = python$(PY_VERSION)

LIBNAME = lib$(BASENAME).a
HEADERNAME = lib$(BASENAME).h

#include /config.mak
#OPTFLAGS =  -O4 -march=athlon -pipe -ffast-math -pg
GCC_VERSION_OPTS = $(shell $(CC) --help -v 2>/dev/null |grep -o -m 1 "\-fgnu89-inline")
INCLUDES =  -lm
COMMON_OPTS =  -O3 -pipe -ffast-math -std=gnu99 -Wall -fomit-frame-pointer -D_GNU_SOURCE $(GCC_VERSION_OPTS)
#for profiling:
#COMMON_OPTS =  -O3 -pipe -ffast-math -std=gnu99 -Wall -D_GNU_SOURCE -pg


RANLIB	= true

include ./local.mak

PY_CFLAGS =   $(DEBUG_FLAGS)  -Wall -pthread -fno-strict-aliasing -DNDEBUG -Wstrict-prototypes -fPIC -I/usr/include/$(PYTHON)

SRCS	=  perceptron.c training.c debug.c  dSFMT/dSFMT.c
OBJS	= $(SRCS:.c=.o)

CFLAGS	=  $(COMMON_OPTS) $(MACHINE_OPTS) -DDSFMT_MEXP=19937

########## making asm versions for debug ########
ASM_DIR = ./asm
ASM_INFILE = training.c
ASM_OUTFILE = training
#ASM_INFILE = perceptron.c
#ASM_OUTFILE = perceptron

ASM_OPTS = -S -fverbose-asm


.SUFFIXES: .c .o

.PHONY: all clean asm cpp

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

$(LIBNAME):	$(OBJS)
	$(AR) r $(LIBNAME) $(OBJS)
	$(RANLIB) $(LIBNAME)


all:	$(LIBNAME)

clean:
	rm -f *.o *.a *~ *.so

distclean:
	rm -f test *.o *.a *~ .depend

dep:    depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) nn_test.c 1>.depend

install:	all
	cp $(LIBNAME) /usr/local/lib/
	cp $(HEADERNAME) /usr/local/include/

asm:
	gcc-4.1  $(CFLAGS) $(ASM_OPTS) $(ASM_INFILE) -o $(ASM_DIR)/$(ASM_OUTFILE)-4_1.s

cpp:
	$(CC)  $(CFLAGS) -E nn_test.c -o $(ASM_DIR)/nn_test.i
	$(CC)  $(CFLAGS) -E training.c -o $(ASM_DIR)/training.i

test:	install
	$(CC)  $(CFLAGS)   nn_test.c -o /tmp/nn_test.o -l$(BASENAME) -lmcheck $(INCLUDES)
	/tmp/nn_test.o

random-test:	install
	$(CC)  $(CFLAGS)   random-test.c -o /tmp/random-test.o -l$(BASENAME) -lmcheck $(INCLUDES)
	/tmp/random-test.o

python:	all install
	$(PYTHON) setup.py clean
	$(PYTHON) setup.py build_ext -lperceptron $(INCLUDES)  -f

python-install:	python
	$(PYTHON) setup.py install --install-lib /usr/local/lib/$(PYTHON)/site-packages  -f
	$(PYTHON) -c 'import nnpy;print dir(nnpy)'

python-test: python-install
	$(PYTHON) test.py

olpc: all
	$(CC) $(PY_CFLAGS)  $(CFLAGS)   -c nnpy.c -o nnpy.o
	$(CC) -pthread -shared -Wl,-O1 nnpy.o -L. -lperceptron  $(INCLUDES)  -o nnpy.so



#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
