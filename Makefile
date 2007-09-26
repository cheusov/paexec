CC=		cc
CFLAGS=		-I/usr/pkg/include
LDFLAGS=	-L/usr/pkg/lib

.PHONY : all
all : cluster_exec

wrappers.o : wrappers.c
	$(CC) -o $@ -c $(CFLAGS) $<
nonblock_helpers.o : nonblock_helpers.c
	$(CC) -o $@ -c $(CFLAGS) $<

cluster_exec.o : cluster_exec.c
	$(CC) -o $@ -c $(CFLAGS) cluster_exec.c
cluster_exec : cluster_exec.o nonblock_helpers.o wrappers.o
	$(CC) -o $@ $^ $(LDFLAGS) -lmaa

.PHONY : clean
clean:
	rm -f *~ *.o core.* *.core core cluster_exec
