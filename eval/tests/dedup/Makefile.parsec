# Makefile for dedup kernel

PREFIX=${PARSECDIR}/pkgs/kernels/dedup/inst/${PARSECPLAT}

TARGET=dedup

BIGFILE_CFLAGS=$(shell getconf LFS_CFLAGS)

CFLAGS += -Wall -std=c99 -fno-strict-aliasing -D_XOPEN_SOURCE=600
LIBS += -lcrypto -lz

DEDUP_OBJ =  hashtable.o queue.o util.o binheap.o tree.o decoder.o dedup.o rabin.o encoder.o

ifdef version
  ifeq "$(version)" "pthreads"
    CFLAGS += -DPARALLEL -pthread
  endif
endif


all: $(TARGET)

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(DEDUP_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(DEDUP_OBJ) $(LIBS)

clean:
	rm -f *~ *.o $(TARGET)

install:
	mkdir -p $(PREFIX)/bin
	cp -f $(TARGET) $(PREFIX)/bin/$(TARGET)

