CC=		mpicc
LD=		$(CC)
CFLAGS=		-g -O0 -Wall

.PHONY : all
all : cluster_exec
cluster_exec : cluster_exec.o

.PHONY : clean
clean:
	rm -f *~ *.o core.* *.core core cluster_exec
