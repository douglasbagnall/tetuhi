#tuning for Geode

MACHINE_OPTS =  -march=geode
LOCAL_OPTS = $(MACHINE_OPTS)

PYTHON = python2.5

#DEBUG_FLAGS = -Os
DEBUG_FLAGS = -g -O0