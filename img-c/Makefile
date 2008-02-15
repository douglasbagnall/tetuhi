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

BASENAME = imagescan

PYNAME = $(BASENAME).so

#DEBUG_FLAGS = -g -O0
DEBUG_FLAGS = -O3

COMMON_OPTS =   -pipe -ffast-math -std=gnu99 -Wall -ffast-math -fomit-frame-pointer -fno-PIC  -D_GNU_SOURCE


#CC	= cgcc
RANLIB	= true
PYTHON	= python

include ./local.mak

PY_CFLAGS =   $(DEBUG_FLAGS)  -Wall -pthread -fno-strict-aliasing -DNDEBUG -Wstrict-prototypes -fPIC -I/usr/include/$(PYTHON) 

SRCS	= img.c
OBJS	= $(SRCS:.c=.o)

CFLAGS	= $(DEBUG_FLAGS) $(COMMON_OPTS) $(LOCAL_OPTS)


.SUFFIXES: .c .o

.PHONY: all clean asm test_prox

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(CPPFLAGS2) -o $@ $<

all:	python

clean:
	rm -f *.o *.a *~ *.so

distclean:
	rm -f test *.o *.a *~ .depend

dep:    depend

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend


asm:
	$(CC)  $(CFLAGS) $(ASM_OPTS) $(ASM_INFILE) -o $(ASM_DIR)/$(ASM_OUTFILE).s

cpp:
	$(CC)  $(CFLAGS) -E $(ASM_INFILE) -o $(ASM_DIR)/$(ASM_OUTFILE).i

python:
	$(CC) $(PY_CFLAGS)  $(CFLAGS)  $(CPPFLAGS2) -c img.c -o $(BASENAME).o
	$(CC) -pthread -shared -Wl,-O1 $(BASENAME).o  -o $(PYNAME)

python-install: python
	mv $(PYNAME) /usr/local/lib/$(PYTHON)/site-packages/

python-test: python-install
	$(PYTHON) test.py 



#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
