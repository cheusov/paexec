CC=		mpicc
LD=		$(CC)
CFLAGS=		-I/usr/pkg/include -g -O0 -Wall

.PHONY : all
all : cluster_exec
cluster_exec.o : cluster_exec.c
	$(CC) -o $@ -c $(CFLAGS) $<
cluster_exec : cluster_exec.o
	$(CC) -o $@ -L/usr/pkg/lib -lmaa $<

.PHONY : clean
clean:
	rm -f *~ *.o core.* *.core core cluster_exec
