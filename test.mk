CLEANFILES+= transport_closed_stdin _test.in

${.OBJDIR}/transport_closed_stdin: examples/broken_echo/transport_closed_stdin.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(.TARGET) $(.ALLSRC) $(LDFLAGS)

.PHONY : test
test : paexec ${.OBJDIR}/transport_closed_stdin; \
	@echo 'running tests...'; \
	export OBJDIR=${.OBJDIR}; \
	if cd ${.CURDIR}/tests && ./test.sh; \
	then echo 'SUCCEEDED'; \
	else echo 'FAILED'; false; \
	fi
