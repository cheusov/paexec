CC=		cc
CFLAGS=		-I/usr/pkg/include
LDFLAGS=	-L/usr/pkg/lib

.PHONY : all
all : cluster_exec2

wrappers.o : wrappers.c
	$(CC) -o $@ -c $(CFLAGS) $<
nonblock_helpers.o : nonblock_helpers.c
	$(CC) -o $@ -c $(CFLAGS) $<

cluster_exec2.o : cluster_exec2.c
	$(CC) -o $@ -c $(CFLAGS) cluster_exec2.c
cluster_exec2 : cluster_exec2.o nonblock_helpers.o wrappers.o
	$(CC) -o $@ $^ $(LDFLAGS) -lmaa

.PHONY : clean
clean:
	rm -f *~ *.o core.* *.core core cluster_exec2
