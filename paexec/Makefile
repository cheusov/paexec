# initial buffer size for the tasks and results
BUFSIZE?=		4096

############################################################
BIRTHDATE=	2008-01-25

WARNS?=		4
WARNERR?=	no

PROG=		paexec
SRCS=		paexec.c wrappers.c tasks.c

SCRIPTS=	paexec_reorder

MAN=		paexec.1 paexec_reorder.1

CFLAGS+=	-DPAEXEC_VERSION='"${VERSION}"'
CFLAGS+=	-DBUFSIZE=${BUFSIZE}

MKC_REQUIRE_HEADERS=	maa.h
MKC_REQUIRE_FUNCLIBS=	maa_init:maa

MKC_COMMON_DEFINES=	-D_GNU_SOURCE
MKC_COMMON_HEADERS=	unistd.h
MKC_CHECK_FUNCS5=	getopt_long:getopt.h getopt_long
MKC_CHECK_TYPES=	intptr_t:stdint.h intptr_t:inttypes.h
MKC_CHECK_HEADERS=	sys/select.h

CLEANFILES=  *~ core* *.1 *.html ktrace* ChangeLog *.tmp

############################################################
.include <mkc.prog.mk>
