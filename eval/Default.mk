DTHREADS_HOME=../../..

NCORES ?= 8

#CC = gcc -m32 -march=core2 -mtune=core2
#CXX = g++ -m32 -march=core2 -mtune=core2
CC = gcc -march=core2 -mtune=core2
CXX = g++ -march=core2 -mtune=core2
CFLAGS += -O5

CONFIGS = pthread dthread dmp_o dmp_b
PROGS = $(addprefix $(TEST_NAME)-, $(CONFIGS))

.PHONY: default all clean

default: all
all: $(PROGS)
clean:
	rm -f $(PROGS) obj/*

eval: $(addprefix eval-, $(CONFIGS))

############ pthread builders ############

PTHREAD_CFLAGS = $(CFLAGS)
PTHREAD_LIBS += $(LIBS) -lpthread

PTHREAD_OBJS = $(addprefix obj/, $(addsuffix -pthread.o, $(TEST_FILES)))

obj/%-pthread.o: %-pthread.c
	$(CC) $(PTHREAD_CFLAGS) -c $< -o $@ -I$(HOME)/include

obj/%-pthread.o: %.c
	$(CC) $(PTHREAD_CFLAGS) -c $< -o $@ -I$(HOME)/include

obj/%-pthread.o: %-pthread.cpp
	$(CXX) $(PTHREAD_CFLAGS) -c $< -o $@ -I$(HOME)/include

obj/%-pthread.o: %.cpp
	$(CXX) $(PTHREAD_CFLAGS) -c $< -o $@ -I$(HOME)/include

$(TEST_NAME)-pthread: $(PTHREAD_OBJS)
	$(CC) $(PTHREAD_CFLAGS) -o $@ $(PTHREAD_OBJS) $(PTHREAD_LIBS)

eval-pthread: $(TEST_NAME)-pthread
	time ./$(TEST_NAME)-pthread $(TEST_ARGS) &> /dev/null

############ dthread builders ############

DTHREAD_CFLAGS = $(CFLAGS) -DNDEBUG
DTHREAD_LIBS += $(LIBS) -rdynamic $(DTHREADS_HOME)/src/libdthreads64.so -ldl
#DTHREAD_LIBS += $(LIBS) -rdynamic $(DTHREADS_HOME)/src/libdthreads.so -ldl

DTHREAD_OBJS = $(addprefix obj/, $(addsuffix -dthread.o, $(TEST_FILES)))

obj/%-dthread.o: %-pthread.c
	$(CC) $(DTHREAD_CFLAGS) -c $< -o $@ -I$(HOME)/include

obj/%-dthread.o: %.c
	$(CC) $(DTHREAD_CFLAGS) -c $< -o $@ -I$(HOME)/include

obj/%-dthread.o: %-pthread.cpp
	$(CC) $(DTHREAD_CFLAGS) -c $< -o $@ -I$(HOME)/include

obj/%-dthread.o: %.cpp
	$(CXX) $(DTHREAD_CFLAGS) -c $< -o $@ -I$(HOME)/include

### FIXME, put the 
$(TEST_NAME)-dthread: $(DTHREAD_OBJS) $(DTHREADS_HOME)/src/libdthreads.so
	$(CC) $(DTHREAD_CFLAGS) -o $@ $(DTHREAD_OBJS) $(DTHREAD_LIBS)

eval-dthread: $(TEST_NAME)-dthread
	time ./$(TEST_NAME)-dthread $(TEST_ARGS)
#	time ./$(TEST_NAME)-dthread $(TEST_ARGS) &> /dev/null

############ coredet generic defines ############

COREDET_CFLAGS = -DENABLE_DMP
COREDET_LIBS += $(LIBS) -lstdc++ -lpthread -ldl
COREDET_OPT = -load=libLLVMDataStructure.so -load=libLLVMMakeDeterministic.so -basicaa -libcall-aa -anders-aa -ds-aa -std-compile-opts -makedeterministic -dmp=true -no-pthreads

COREDET_RUNTIME = library-barrier library-cv library-mutex library-once library-rwlock library-semaphore library-thread runtime-main runtime-profiling runtime-sched runtime-wb debug

obj/static-ctor.bc: $(HOME)/coredet/static-ctor.ll
	llvm-as -f -o $@ $<

############ dmp-o builders ############

DMP_O_CFLAGS = $(CFLAGS) $(COREDET_CFLAGS) -DDMP_ENABLE_MODEL_O_S
DMP_O_LIBS = $(COREDET_LIBS)
DMP_O_OPT = $(COREDET_OPT) -dmp-quantum-balance=heuristic -dmp-heuristic-balance-thresh=3 -dmp-do-escapeopt=true -dmp-do-memtrackopt=true -dmp-do-memcoalesceopt=true

# Set MOT granularity
DMP_O_GRAN ?= 6
DMP_O_QUANTUM ?= 1000
DMP_O_BLOCKSIZE = $(shell expr $$((2 ** $(DMP_O_GRAN))))
DMP_O_CFLAGS += -DDMP_MOT_GRANULARITY=$(DMP_O_GRAN)
DMP_O_OPT += -dmp-mot-blocksize=$(DMP_O_BLOCKSIZE)

DMP_O_RUNTIME = $(COREDET_RUNTIME) runtime-model-O_S

DMP_O_OBJS = $(addprefix obj/, $(addsuffix -dmp_o.bc, $(TEST_FILES)))
DMP_O_RUNTIME_OBJS = obj/static-ctor.bc $(addprefix obj/, $(addsuffix -dmp_o.runtime.bc, $(DMP_O_RUNTIME)))

obj/%-dmp_o.bc: %-pthread.c
	$(CC) -emit-llvm $(DMP_O_CFLAGS) -c $< -o $@ -I$(HOME)/include -I$(HOME)/coredet

obj/%-dmp_o.bc: %.c
	$(CC) -emit-llvm $(DMP_O_CFLAGS) -c $< -o $@ -I$(HOME)/include -I$(HOME)/coredet

obj/%-dmp_o.bc: %-pthread.cpp
	$(CXX) -emit-llvm $(DMP_O_CFLAGS) -c $< -o $@ -I$(HOME)/include -I$(HOME)/coredet

obj/%-dmp_o.bc: %.cpp
	$(CXX) -emit-llvm $(DMP_O_CFLAGS) -c $< -o $@ -I$(HOME)/include -I$(HOME)/coredet

obj/%-dmp_o.runtime.bc: $(HOME)/coredet/%.cpp
	$(CC) -emit-llvm $(DMP_O_CFLAGS) -c $< -o $@

obj/$(TEST_NAME)-dmp_o.complete.bc: $(DMP_O_OBJS)
	llvm-link -f -o $@ $(DMP_O_OBJS)
	opt $(DMP_O_OPT) -o $@ -f $@

$(TEST_NAME)-dmp_o: obj/$(TEST_NAME)-dmp_o.complete.bc $(DMP_O_RUNTIME_OBJS)
	$(LLVM_LD) -o $@ -native $< $(DMP_O_RUNTIME_OBJS) $(DMP_O_LIBS)
	rm $@.bc

eval-dmp_o: $(TEST_NAME)-dmp_o
	time DMP_SCHEDULING_CHUNK_SIZE=$(DMP_O_QUANTUM) ./$(TEST_NAME)-dmp_o $(TEST_ARGS) &> /dev/null

########### dmp-b builders ############

DMP_B_CFLAGS = $(CFLAGS) $(COREDET_CFLAGS) -DDMP_ENABLE_MODEL_B_S -DDMP_ENABLE_WB_PARALLEL_COMMIT
DMP_B_LIBS = $(COREDET_LIBS)
DMP_B_OPT = $(COREDET_OPT) -dmp-memtrack-for-buffering=true -dmp-quantum-balance=heuristic -dmp-heuristic-balance-thresh=3 -dmp-do-escapeopt=true

# Set MOT granularity
DMP_B_GRAN ?= 6
DMP_B_QUANTUM ?= 1000
DMP_B_BLOCKSIZE = $(shell expr $$((2 ** $(DMP_B_GRAN))))
DMP_B_CFLAGS += -DDMP_MOT_GRANULARITY=$(DMP_B_GRAN)
DMP_B_OPT += -dmp-mot-blocksize=$(DMP_B_BLOCKSIZE)

DMP_B_RUNTIME = $(COREDET_RUNTIME) runtime-model-B_S

DMP_B_OBJS = $(addprefix obj/, $(addsuffix -dmp_b.bc, $(TEST_FILES)))
DMP_B_RUNTIME_OBJS = obj/static-ctor.bc $(addprefix obj/, $(addsuffix -dmp_b.runtime.bc, $(DMP_B_RUNTIME)))

obj/%-dmp_b.bc: %-pthread.c
	$(CC) -emit-llvm $(DMP_B_CFLAGS) -c $< -o $@ -I$(HOME)/include -I$(HOME)/coredet

obj/%-dmp_b.bc: %.c
	$(CC) -emit-llvm $(DMP_B_CFLAGS) -c $< -o $@ -I$(HOME)/include -I$(HOME)/coredet

obj/%-dmp_b.bc: %-pthread.cpp
	$(CXX) -emit-llvm $(DMP_B_CFLAGS) -c $< -o $@ -I$(HOME)/include -I$(HOME)/coredet

obj/%-dmp_b.bc: %.cpp
	$(CXX) -emit-llvm $(DMP_B_CFLAGS) -c $< -o $@ -I$(HOME)/include -I$(HOME)/coredet

obj/%-dmp_b.runtime.bc: $(HOME)/coredet/%.cpp
	$(CC) -emit-llvm $(DMP_B_CFLAGS) -c $< -o $@

obj/$(TEST_NAME)-dmp_b.complete.bc: $(DMP_B_OBJS)
	llvm-link -f -o $@ $(DMP_B_OBJS)
	opt $(DMP_B_OPT) -o $@ -f $@

$(TEST_NAME)-dmp_b: obj/$(TEST_NAME)-dmp_b.complete.bc $(DMP_B_RUNTIME_OBJS)
	$(LLVM_LD) -o $@ -native $< $(DMP_B_RUNTIME_OBJS) $(DMP_B_LIBS)
	rm $@.bc

eval-dmp_b: $(TEST_NAME)-dmp_b
	time DMP_SCHEDULING_CHUNK_SIZE=$(DMP_B_QUANTUM) ./$(TEST_NAME)-dmp_b $(TEST_ARGS) &> /dev/null


