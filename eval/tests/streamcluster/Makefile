include ../defines.mk

TEST_NAME = streamcluster
TEST_FILES = streamcluster
TEST_ARGS = 10 20 128 16384 16384 1000 none output.txt $(NCORES)

CFLAGS = -DENABLE_THREADS
LIBS = -lstdc++

DMP_O_GRAN = 6
DMP_O_QUANTUM = 100000

DMP_B_GRAN = 6
DMP_B_QUANTUM = 100000

include $(HOME)/Default.mk

