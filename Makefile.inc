# initial buffer size for the tasks and results
BUFSIZE ?=	4096

#
EGDIR ?=	${DATADIR}/doc/paexec/examples
DOCDIR ?=	${DATADIR}/doc/paexec

# Those poor souls who run Solaris can set AWK to gawk
AWK ?=			/usr/bin/awk

.ifndef PAEXEC_SH
.  if exists(/usr/xpg4/bin/sh)
# Solaris-10's /bin/sh is completely broken,
# /usr/xpg4/bin/sh sucks too but sucks less.
PAEXEC_SH     =		/usr/xpg4/bin/sh
.  else
PAEXEC_SH     =		/bin/sh
.  endif
.endif

MKC_REQUIRE_PROGS +=		${AWK} runawk ${PAEXEC_SH}

.export AWK
MKC_CHECK_CUSTOM +=		awk_fflush
MKC_CUSTOM_FN.awk_fflush =	checks/awk_fflush

.include "mkc.configure.mk"

.if !${CUSTOM.awk_fflush:U1}
MKC_ERR_MSG += "ERROR: ${AWK} doesnt not support fflush() function"
.endif

INTEXTS_REPLS +=	awk ${PROG.${AWK}:U${AWK}}
INTEXTS_REPLS +=	sh ${PROG.${PAEXEC_SH}:U${PAEXEC_SH}}
INTEXTS_REPLS +=	version ${VERSION:U}

#
VERSION=	1.1.4
