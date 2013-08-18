MKC_CHECK_CUSTOM +=		sleep_fract
MKC_CUSTOM_FN.sleep_fract =	../checks/sleep_fract

CLEANFILES +=	_test.in _tasks.tmp _test.tmp

all:
	@echo 'running tests...'; \
	set -e; \
	cd ${.CURDIR}; \
	export OBJDIR=${.OBJDIR}; \
	export PATH=${.CURDIR}/broken_echo:$$PATH; \
	export PATH=${.CURDIR}/scripts:$$PATH; \
	export PATH=${.CURDIR}/../paexec:$$PATH; \
	export PATH=${.CURDIR}/../examples/all_substr:$$PATH; \
	export PATH=${.CURDIR}/../examples/cc_wrapper:$$PATH; \
	export PATH=${.CURDIR}/../examples/cc_wrapper2:$$PATH; \
	export PATH=${.CURDIR}/../examples/dirtest:$$PATH; \
	export PATH=${.CURDIR}/../examples/divide:$$PATH; \
	export PATH=${.CURDIR}/../examples/make_package:$$PATH; \
	export PATH=${.CURDIR}/../examples/toupper:$$PATH; \
	export PATH=${.CURDIR}/../examples/wav2flac:$$PATH; \
	export PATH=${OBJDIR_paexec}:$$PATH; \
	export PATH=${OBJDIR_scripts}:$$PATH; \
	export PATH=${OBJDIR_transp_closed_stdin}:$$PATH; \
	export PATH=${OBJDIR_all_substr}:$$PATH; \
	export PATH=${OBJDIR_cc_wrapper}:$$PATH; \
	export PATH=${OBJDIR_cc_wrapper2}:$$PATH; \
	export PATH=${OBJDIR_dirtest}:$$PATH; \
	export PATH=${OBJDIR_divide}:$$PATH; \
	export PATH=${OBJDIR_make_package}:$$PATH; \
	export PATH=${OBJDIR_toupper}:$$PATH; \
	export PATH=${OBJDIR_wav2flac}:$$PATH; \
	export SLEEP_FRACT=${CUSTOM.sleep_fract:S/0//}; \
	if ./test.sh; \
	then echo 'SUCCEEDED'; \
	else echo 'FAILED'; false; \
	fi

.include <mkc.files.mk>
