include ../defines.mk

TEST_NAME = swaptions
TEST_FILES = CumNormalInv MaxFunction RanUnif icdf HJM_SimPath_Forward_Blocking HJM HJM_Swaption_Blocking HJM_Securities nr_routines
TEST_ARGS = -ns 128 -sm 50000 -nt $(NCORES)

CFLAGS = -DENABLE_THREADS -fstrict-aliasing -fkeep-inline-functions
LIBS = -lstdc++

DMP_O_GRAN = 6
DMP_O_QUANTUM = 100000

DMP_B_GRAN = 6
DMP_B_QUANTUM = 100000

include $(HOME)/Default.mk

