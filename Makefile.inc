# initial buffer size for the tasks and results
BUFSIZE ?=	4096

#
EGDIR ?=	${DATADIR}/doc/paexec/examples
DOCDIR ?=	${DATADIR}/doc/paexec

# Those poor souls who run Solaris can set AWK to gawk
AWK ?=			/usr/bin/awk

MKC_REQUIRE_PROGS +=		${AWK} runawk

.export AWK
MKC_CHECK_CUSTOM +=		awk_fflush
MKC_CUSTOM_FN.awk_fflush =	checks/awk_fflush

.include "mkc.configure.mk"

.if !${CUSTOM.awk_fflush:U1}
MKC_ERR_MSG += "ERROR: ${AWK} doesnt not support fflush() function"
.endif

INTEXTS_REPLS +=	awk ${PROG.${AWK}:U${AWK}}
INTEXTS_REPLS +=	version ${VERSION:U}

#
VERSION=	1.0.1
