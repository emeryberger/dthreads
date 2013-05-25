include ../defines.mk

TEST_NAME = blackscholes
TEST_FILES = blackscholes
TEST_ARGS = 8 $(DATASET_HOME)/blackscholes/in_1M.txt prices.txt

CFLAGS = -DENABLE_THREADS -DPARALLEL -DNCO=2 -DN=960
PTHREAD_LIBS = -lm -lstdc++
DTHREAD_LIBS = -lm -lstdc++

SRC_SUFFIX=.cpp

DMP_O_GRAN = 6
DMP_O_QUANTUM = 100000

DMP_B_GRAN = 6
DMP_B_QUANTUM = 100000

include $(HOME)/Default.mk

