LIBS = -lm -lrt -ldl
CFLAGS = -m32 -Wall -g -O3 # -T../src/dthreads.ld
DTHREAD_LIBS = $(LIBS) -rdynamic ../src/libdthreads32.so
PTHREAD_LIBS = $(LIBS) -lpthread

all:
	gcc $(CFLAGS) bug-atom.c      -o bug-atom $(PTHREAD_LIBS)
	gcc $(CFLAGS) bug-atom.c      -o bug-atom-det $(DTHREAD_LIBS)
	gcc $(CFLAGS) bug-deadlock1.c -o bug-deadlock1 $(PTHREAD_LIBS)
	gcc $(CFLAGS) bug-deadlock1.c -o bug-deadlock1-det $(DTHREAD_LIBS)
	gcc $(CFLAGS) bug-deadlock.c  -o bug-deadlock $(PTHREAD_LIBS)
	gcc $(CFLAGS) bug-deadlock.c  -o bug-deadlock-det $(DTHREAD_LIBS)
	gcc $(CFLAGS) bug-lockuse.c   -o bug-lockuse $(PTHREAD_LIBS)
	gcc $(CFLAGS) bug-lockuse.c   -o bug-lockuse-det $(DTHREAD_LIBS)
	gcc $(CFLAGS) bug-order.c     -o bug-order $(PTHREAD_LIBS)
	gcc $(CFLAGS) bug-order.c     -o bug-order-det $(DTHREAD_LIBS)
	gcc $(CFLAGS) bug-race.c      -o bug-race $(PTHREAD_LIBS)
	gcc $(CFLAGS) bug-race.c      -o bug-race-det $(DTHREAD_LIBS)
	g++ $(CFLAGS) -o lock-dthread lockowner.c $(DTHREAD_LIBS) 
	g++ $(CFLAGS) -o lock-pthread lockowner.c $(PTHREAD_LIBS) 
	g++ $(CFLAGS) -o bug-init bug-init-32bit.c $(DTHREAD_LIBS) 

