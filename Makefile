SRCS = libdthread.cpp xrun.cpp xthread.cpp xmemory.cpp prof.cpp real.cpp
DEPS = $(SRCS) xpersist.h xdefines.h xglobals.h xpersist.h xplock.h xrun.h warpheap.h xadaptheap.h xoneheap.h

INCLUDE_DIRS = -I. -I./heaplayers -I./heaplayers/util

all: libgrace.so

# 64bit version
#CXX = g++ -march=core2 -mtune=core2 -msse3

#32 bit version.
CXX = g++ -march=core2 -mtune=core2 -msse3 -m32 -DSSE_SUPPORT 

# Check deterministic schedule
# -DCHECK_SCHEDULE

# Get some characteristics about running.
# -DGET_CHARACTERISTICS
CFLAGS = -O3 -DNDEBUG -shared -fPIC -DLAZY_COMMIT -DLOCK_OWNERSHIP -DDETERM_MEMORY_ALLOC -D'CUSTOM_PREFIX(x)=grace\#\#x'

LIBS = -ldl -lpthread

libgrace.so: $(SRCS) $(DEPS) Makefile
	$(CXX) $(CFLAGS) $(INCLUDE_DIRS) $(SRCS) -o libdthread.so $(LIBS)

clean:
	rm -f libdthread.so 

