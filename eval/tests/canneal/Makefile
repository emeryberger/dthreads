include ../defines.mk
include $(HOME)/Default.mk

TEST_NAME = canneal
TEST_FILES = annealer_thread rng netlist main netlist_elem

ifeq ($(NCORES), 8)
	THREADS = 7
endif

ifeq ($(NCORES), 4)
	THREADS = 3
endif

ifeq ($(NCORES), 2)
	THREADS = 1
endif

TEST_ARGS = $(THREADS) 15000 2000 $(DATASET_HOME)/canneal/400000.nets 128

CFLAGS = -DENABLE_THREADS -DPARALLEL
LIBS = -lm -lstdc++

DMP_O_GRAN = 6
DMP_O_QUANTUM = 100000

DMP_B_GRAN = 6
DMP_B_QUANTUM = 100000


