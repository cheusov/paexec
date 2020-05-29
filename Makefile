BIRTHDATE   =	2008-01-25
PROJECTNAME =	paexec

#####
SUBPRJ      =	paexec:tests paargs presentation doc
SUBPRJ_DFLT?=	paexec paargs

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
MKC_REQD    =	0.33.0

#####
test: all-tests
	@:

clean: clean-tests clean-presentation
cleandir: cleandir-tests  cleandir-presentation

# new recursive target for making a distribution tarball
TARGETS    +=	_prepdist

DIST_TARGETS=	prepdist

.PHONY: prepdist
prepdist: all-presentation _prepdist
	rm ${MKC_CACHEDIR}/_mkc*
	${MAKE} -Cpresentation ${MAKEFILES} _clean_garbage

#####
.include "help.mk"
.include "use.mk"
.include "Makefile.inc"
.include <mkc.subprj.mk>
