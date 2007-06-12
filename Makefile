CC=		cc
MPICC=		mpicc
CFLAGS=		-I/usr/pkg/include -g -O0 -Wall

.PHONY : all
all : cluster_exec subprocess

wrappers.o : wrappers.c
	$(CC) -o $@ -c $(CFLAGS) wrappers.c

cluster_exec.o : cluster_exec.c
	$(MPICC) -o $@ -c $(CFLAGS) cluster_exec.c
cluster_exec : cluster_exec.o wrappers.o
	$(MPICC) -o $@ cluster_exec.o wrappers.o -L/usr/pkg/lib -lmaa

subprocess.o : subprocess.c
	$(CC) -o $@ -c $(CFLAGS) subprocess.c
subprocess : subprocess.o wrappers.o
	$(CC) -o $@ subprocess.o wrappers.o -L/usr/pkg/lib -Wl,-rpath -Wl,/usr/pkg/lib -lmaa

.PHONY : clean
clean:
	rm -f *~ *.o core.* *.core core cluster_exec subprocess
