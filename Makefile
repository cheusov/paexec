MPIROOT=	/usr/pkg
#MPIROOT=	/home/cheusov/local/chen.chizhovka.net/openmpi

CC=		cc
MPICC=		${MPIROOT}/bin/mpicc
CFLAGS=		-I${MPIROOT}/include -I/usr/pkg/include -g -O0 -Wall
LIBDIR=		${MPIROOT}/lib

.PHONY : all
all : cluster_exec cluster_exec2

wrappers.o : wrappers.c
	$(CC) -o $@ -c $(CFLAGS) $<
nonblock_helpers.o : nonblock_helpers.c
	$(CC) -o $@ -c $(CFLAGS) $<

cluster_exec.o : cluster_exec.c
	$(MPICC) -o $@ -c $(CFLAGS) cluster_exec.c
cluster_exec : cluster_exec.o nonblock_helpers.o wrappers.o
	$(MPICC) -o $@ -L${LIBDIR} -Wl,-rpath -Wl,${LIBDIR} $^ -L/usr/pkg/lib -lmaa

cluster_exec2.o : cluster_exec2.c
	$(CC) -o $@ -c $(CFLAGS) cluster_exec2.c
cluster_exec2 : cluster_exec2.o nonblock_helpers.o wrappers.o
	$(CC) -o $@ $^ -L/usr/pkg/lib -lmaa

.PHONY : clean
clean:
	rm -f *~ *.o core.* *.core core cluster_exec cluster_exec2
