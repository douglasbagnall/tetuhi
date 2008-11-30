# Processor type
# native is a good choice for GCC 4.3+
# i686 should work for most.
# geode for XO

LOCAL_OPTS :=  -march=native
#LOCAL_OPTS :=  -march=i686

#SSE2?

LOCAL_OPTS := $(LOCAL_OPTS) -msse2 -DHAVE_SSE2

#Extra optimisations, good for GCC4, according to dSFMT.h
#
#LOCAL_OPTS := $(LOCAL_OPTS) --param inline-unit-growth=500 --param large-function-growth=900

#Aso used by dSFMT
#--param max-inline-insns-single=1800


#LOCAL_OPTS := $(LOCAL_OPTS) -DUSE_INT_RAND
#RAND_SOURCE :=  SFMT/SFMT.c

RAND_SOURCE :=  dSFMT/dSFMT.c
