BIRTHDATE   =	2008-01-25

PROJECTNAME =	paexec

SUBPRJ      =	paexec:tests examples doc
SUBPRJ_DFLT?=	paexec

MKC_REQD    =	0.23.0

TARGETS    +=	_manpages

.PHONY: manpages
manpages: _manpages
	rm ${MKC_CACHEDIR}/_mkc*

clean: clean-examples clean-tests
cleandir: cleandir-examples cleandir-tests

.include "test.mk"
.include <mkc.subprj.mk>
