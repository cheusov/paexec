BIRTHDATE   =	2008-01-25

PROJECTNAME =	paexec

SUBPRJ      =	paexec:tests doc
SUBPRJ_DFLT?=	paexec

MKC_REQD    =	0.23.0

# new recursive target for making a distribution tarball
TARGETS    +=	_manpages

examples    =	divide all_substr cc_wrapper cc_wrapper2 make_package toupper
.for d in ${examples}
SUBPRJ     +=	examples/${d}:tests examples/${d}:examples
.endfor

tests       =	transp_closed_stdin scripts
.for d in ${tests}
SUBPRJ     +=	tests/${d}:tests
.endfor

.PHONY: manpages
manpages: _manpages
	rm ${MKC_CACHEDIR}/_mkc*

test: all-tests
	@:

clean: clean-tests
cleandir: cleandir-tests

.include <mkc.subprj.mk>
