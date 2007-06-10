CC=	/usr/pkg/bin/mpicc
LD=	$(CC)
CFLAGS=	-g -O0

.PHONY : all
all : mpi_whoami
mpi_whoami : mpi_whoami.o
#	$(CC) 

.PHONY : clean
clean:
	rm -f *~ *.o core.* *.core core mpi_whoami
