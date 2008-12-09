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

BASENAME = gamemap

LIBNAME = lib$(BASENAME).a
HEADERNAME = lib$(BASENAME).h


#DEBUG_FLAGS = -g -O0
DEBUG_FLAGS = -O3


COMMON_OPTS =  $(DEBUG_FLAGS) -pipe -ffast-math -std=gnu99 -Wall -fomit-frame-pointer  -D_GNU_SOURCE

RANLIB	= true
PY_VERSION = $(shell python -c 'import sys; print sys.version[:3]')
PYTHON = python$(PY_VERSION)

include ../config.mak

PY_CFLAGS =    -Wall -pthread -fno-strict-aliasing -DNDEBUG -Wstrict-prototypes -fPIC -I/usr/include/$(PYTHON)

SRCS	= gamemap.c scan.c
OBJS	= $(SRCS:.c=.o)

CFLAGS	= $(COMMON_OPTS) $(LOCAL_OPTS)




########## making asm versions for debug ########
ASM_DIR = ./asm
ASM_INFILE = scan.c
ASM_OUTFILE = scan
#ASM_OPTS = -S -fkeep-inline-functions
ASM_OPTS = -S -fverbose-asm  -fkeep-inline-functions


.SUFFIXES: .c .o

.PHONY: all clean asm test_prox

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(CPPFLAGS2) -o $@ $<

$(LIBNAME):	$(OBJS)
	$(AR) r $(LIBNAME) $(OBJS)
	$(RANLIB) $(LIBNAME)


all:	$(LIBNAME)  $(INCLUDES)

clean:
	rm -f *.o *.a *~ *.so

distclean:
	rm -f test *.o *.a *~ .depend

dep:    depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend

install:
	cp $(LIBNAME) /usr/local/lib/
	cp $(HEADERNAME) /usr/local/include/
	rm $(LIBNAME)

asm:
	$(CC)  $(CFLAGS) $(ASM_OPTS) $(ASM_INFILE) -o $(ASM_DIR)/$(ASM_OUTFILE).s
#	gcc-3.4  $(CFLAGS) $(ASM_OPTS) $(ASM_INFILE) -o $(ASM_DIR)/$(ASM_OUTFILE)-3_4.s
#	gcc-4.0  $(CFLAGS) $(ASM_OPTS) $(ASM_INFILE) -o $(ASM_DIR)/$(ASM_OUTFILE)-4_0.s
#	gcc-4.1  $(CFLAGS) $(ASM_OPTS) $(ASM_INFILE) -o $(ASM_DIR)/$(ASM_OUTFILE)-4_1.s

cpp:
	$(CC)  $(CFLAGS) -E $(ASM_INFILE) -o $(ASM_DIR)/$(ASM_OUTFILE).i

python: all install
	$(CC) $(PY_CFLAGS)  $(CFLAGS)  $(CPPFLAGS2) -c semantic.c -o semantic.o
	$(CC) -pthread -shared -Wl,-O1 semantic.o -lgamemap -o semanticCore.so

python-install: python
	mv semanticCore.so /usr/local/lib/$(PYTHON)/site-packages/
#	$(PYTHON) setup.py install_lib --install-dir /usr/local/lib/$(PYTHON)/site-packages  -f
#	$(PYTHON) -c 'import nnpy;print dir(nnpy)'

python-test: python-install
	$(PYTHON) test.py -s

olpc: all
	$(CC) $(PY_CFLAGS)  $(CFLAGS)  $(CPPFLAGS2) -c semantic.c -o semantic.o
	$(CC) -pthread -shared -Wl,-O1 semantic.o -L. -lgamemap  -o semanticCore.so

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
