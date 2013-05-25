include ../defines.mk

TEST_NAME = word_count
TEST_FILES = word_count sort
#TEST_ARGS = ../../datasets/word_count/word_100MB.txt
TEST_ARGS = $(DATASET_HOME)/word_count/word_100MB.txt

DMP_O_GRAN = 6
DMP_O_QUANTUM = 100000

DMP_B_GRAN = 6
DMP_B_QUANTUM = 100000

include $(HOME)/Default.mk

