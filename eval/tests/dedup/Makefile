include ../defines.mk

TEST_NAME = dedup
TEST_FILES = binheap decoder dedup encoder hashtable queue rabin tree util

ifeq ($(NCORES), 8)
	THREADS = 2
else
	THREADS = 1
endif

TEST_ARGS = -c -p -f -t $(THREADS) -i $(DATASET_HOME)/dedup/media.dat -o output.dat.ddp

#CFLAGS = -D_XOPEN_SOURCE=600 -Iinclude -DENABLE_THREADS -DPARALLEL -std=gnu99 -D_XOPEN_SOURCE=600 -DMD32_REG_T=int -funroll-loops -fprefetch-loop-arrays -fstrict-aliasing -Wstrict-aliasing -O5 -fkeep-inline-functions -DOPENSSL_THREADS -D_REENTRANT -DL_ENDIAN -DTERMIO -DUSE_MMAP
CFLAGS = -Iinclude -DENABLE_THREADS -DPARALLEL -std=gnu99 -DMD32_REG_T=int -D_REENTRANT -DL_ENDIAN -DUSE_MMAP -D_XOPEN_SOURCE=600
LIBS = -Llib -lssl -lz -lcrypto

DMP_O_GRAN = 6
DMP_O_QUANTUM = 100000

DMP_B_GRAN = 6
DMP_B_QUANTUM = 100000

include $(HOME)/Default.mk

