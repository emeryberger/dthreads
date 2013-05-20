INC_DIR = include
SRC_DIR = source

SRCS = $(SRC_DIR)/libdthread.cpp $(SRC_DIR)/xrun.cpp $(SRC_DIR)/xthread.cpp $(SRC_DIR)/xmemory.cpp $(SRC_DIR)/prof.cpp $(SRC_DIR)/real.cpp
DEPS = $(SRCS) $(INC_DIR)/xpersist.h $(INC_DIR)/xdefines.h $(INC_DIR)/xglobals.h $(INC_DIR)/xpersist.h $(INC_DIR)/xplock.h $(INC_DIR)/xrun.h $(INC_DIR)/warpheap.h $(INC_DIR)/xadaptheap.h $(INC_DIR)/xoneheap.h

INCLUDE_DIRS = -I$(INC_DIR) -I$(INC_DIR)/heaplayers -I$(INC_DIR)/heaplayers/util

all: libdthreads32.so libdthreads64.so

# 64bit version
#CXX = g++ -march=core2 -mtune=core2 -msse3

#32 bit version.
CXX = g++ 

# Check deterministic schedule
# -DCHECK_SCHEDULE

# Get some characteristics about running.
# -DGET_CHARACTERISTICS
CFLAGS32 = -march=core2 -mtune=core2 -msse3 -m32 -DX86_32BIT -DSSE_SUPPORT -O3 -DNDEBUG -shared -fPIC -DLAZY_COMMIT -DLOCK_OWNERSHIP -DDETERM_MEMORY_ALLOC -D'CUSTOM_PREFIX(x)=grace\#\#x'

CFLAGS64 = -march=core2 -mtune=core2 -msse3 -DSSE_SUPPORT -O3 -DNDEBUG -shared -fPIC -DLAZY_COMMIT -DLOCK_OWNERSHIP -DDETERM_MEMORY_ALLOC -D'CUSTOM_PREFIX(x)=grace\#\#x'

LIBS = -ldl -lpthread

libdthreads32.so: $(SRCS) $(DEPS) Makefile
	$(CXX) $(CFLAGS32) $(INCLUDE_DIRS) $(SRCS) -o libdthreads32.so $(LIBS)

libdthreads64.so: $(SRCS) $(DEPS) Makefile
	$(CXX) $(CFLAGS64) $(INCLUDE_DIRS) $(SRCS) -o libdthreads64.so $(LIBS)


clean:
	rm -f libdthreads32.so libdthreads64.so 

