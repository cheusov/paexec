# Copyright (c) 2007-2024 Aleksey Cheusov <vle@gmx.net>
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

MKC_CHECK_CUSTOM +=		sleep_fract
MKC_CUSTOM_FN.sleep_fract =	../checks/sleep_fract

CLEANFILES +=	_test.in _tasks.tmp _test.tmp

USE_TEST_BUFSIZES ?=	1 2 3 4 5 6 7 8 9 10 11 12 13 14 1000 10000

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
	export PATH=${OBJDIR_paargs}:$$PATH; \
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
	export PATH=${OBJDIR_delayed_output}:$$PATH; \
	export SLEEP_FRACT=${CUSTOM.sleep_fract:S/0//}; \
	export USE_TEST_BUFSIZES=${USE_TEST_BUFSIZES:Q}; \
	if ./test.sh; \
	then echo 'SUCCEEDED'; \
	else echo 'FAILED'; false; \
	fi

.include <mkc.files.mk>
