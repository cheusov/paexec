CC=		cc
CFLAGS=		-I/usr/pkg/include
LDFLAGS=	-L/usr/pkg/lib -Wl,-rpath -Wl,/usr/pkg/lib

.PHONY : all
all : paexec

wrappers.o : wrappers.c
	$(CC) -o $@ -c $(CFLAGS) $<
nonblock_helpers.o : nonblock_helpers.c
	$(CC) -o $@ -c $(CFLAGS) $<

paexec.o : paexec.c
	$(CC) -o $@ -c $(CFLAGS) paexec.c
paexec : paexec.o nonblock_helpers.o wrappers.o
	$(CC) -o $@ $^ $(LDFLAGS) -lmaa

.PHONY : clean
clean:
	rm -f *~ *.o core.* *.core core paexec
