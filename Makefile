BIRTHDATE   =	2008-01-25

PROJECTNAME =	paexec

SUBPRJ      =	paexec:tests examples doc
SUBPRJ_DFLT =	paexec

.PHONY: manpages
manpages:
	set -e; \
	MKC_CACHEDIR=`pwd`; export MKC_CACHEDIR; \
	cd paexec; ${MAKE} paexec.1 paexec_reorder.1; cd ..; \
	rm -f _mkc_*

.include "test.mk"
.include <mkc.subprj.mk>
