CLEANFILES +=		_test.in

.PHONY : test
test : all-paexec all-examples all-tests
	@echo 'running tests...'; \
	export OBJDIR=${.OBJDIR}; \
	export PATH=${OBJDIR_broken_echo}:${OBJDIR_paexec}:$$PATH; \
	if cd ${.CURDIR}/tests && ./test.sh; \
	then echo 'SUCCEEDED'; \
	else echo 'FAILED'; false; \
	fi
