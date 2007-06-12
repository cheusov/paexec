CC=		cc
MPICC=		mpicc
CFLAGS=		-I/usr/pkg/include -g -O0 -Wall

.PHONY : all
all : cluster_exec subprocess

cluster_exec.o : cluster_exec.c
	$(MPICC) -o $@ -c $(CFLAGS) cluster_exec.c
cluster_exec : cluster_exec.o
	$(MPICC) -o $@ -L/usr/pkg/lib -lmaa cluster_exec.o

subprocess.o : subprocess.c
	$(CC) -o $@ -c $(CFLAGS) $<
subprocess : subprocess.o
	$(CC) -o $@ -L/usr/pkg/lib -Wl,-rpath -Wl,/usr/pkg/lib -lmaa subprocess.o

.PHONY : clean
clean:
	rm -f *~ *.o core.* *.core core cluster_exec subprocess
