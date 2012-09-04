CLEANFILES +=	_test.in _tasks.tmp _test.tmp

all:
	@echo 'running tests...'; \
	export OBJDIR=${.OBJDIR}; \
	export PATH=${.CURDIR}/tests/broken_echo:${OBJDIR_paexec}:$$PATH; \
	if cd ${.CURDIR} && ./test.sh; \
	then echo 'SUCCEEDED'; \
	else echo 'FAILED'; false; \
	fi

.include <mkc.files.mk>
