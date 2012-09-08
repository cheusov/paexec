BIRTHDATE   =	2008-01-25
PROJECTNAME =	paexec

#####
SUBPRJ      =	paexec:tests doc
SUBPRJ_DFLT?=	paexec

examples    =	divide all_substr cc_wrapper cc_wrapper2 \
		make_package toupper dirtest
.for d in ${examples}
SUBPRJ     +=	examples/${d}:tests examples/${d}:examples
.endfor

tests       =	transp_closed_stdin scripts
.for d in ${tests}
SUBPRJ     +=	tests/${d}:tests
.endfor

#####
MKC_REQD    =	0.23.0

#####
test: all-tests
	@:

clean: clean-tests
cleandir: cleandir-tests

# new recursive target for making a distribution tarball
TARGETS    +=	_manpages

DIST_TARGETS=	manpages

.PHONY: manpages
manpages: _manpages
	rm ${MKC_CACHEDIR}/_mkc*

#####
.include <mkc.subprj.mk>
