CC=		cc
MPICC=		mpicc
CFLAGS=		-I/usr/pkg/include -g -O0 -Wall

.PHONY : all
all : cluster_exec

wrappers.o : wrappers.c
	$(CC) -o $@ -c $(CFLAGS) $<
nonblock_helpers.o : nonblock_helpers.c
	$(CC) -o $@ -c $(CFLAGS) $<

cluster_exec.o : cluster_exec.c
	$(MPICC) -o $@ -c $(CFLAGS) cluster_exec.c
cluster_exec : cluster_exec.o nonblock_helpers.o wrappers.o
	$(MPICC) -o $@ $^ -L/usr/pkg/lib -lmaa

.PHONY : clean
clean:
	rm -f *~ *.o core.* *.core core cluster_exec subprocess
