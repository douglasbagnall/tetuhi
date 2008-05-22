#tuning for the local machine

#MACHINE_OPTS =  -march=geode
MACHINE_OPTS =  -march=i686
LOCAL_OPTS = $(MACHINE_OPTS)

PYTHON = python2.5

DEBUG_FLAGS = -O3
#DEBUG_FLAGS = -Os
#DEBUG_FLAGS = -g -O0