# initial buffer size for the tasks and results
BUFSIZE ?=	4096

#
EGDIR ?=	${DATADIR}/doc/paexec/examples
DOCDIR ?=	${DATADIR}/doc/paexec

# Those poor souls who run Solaris can set AWK to /path/to/gawk
AWK ?=			/usr/bin/awk
INTEXTS_REPLS +=	awk ${AWK}

#
VERSION=	0.16.1