# initial buffer size for the tasks and results
BUFSIZE   ?=	${USE_BUFSIZE:U4096}

# optimization flags
COPTS     ?=	${USE_COPTS:U-O2 -g}

#
PROG       =	paexec
SRCS       =	paexec.c wrappers.c tasks.c nodes.c signals.c

SCRIPTS    =	paexec_reorder

MAN        =	paexec.1 paexec_reorder.1

LINKS      =	${BINDIR}/paexec_reorder        ${BINDIR}/pareorder
MLINKS     =	paexec_reorder.1 pareorder.1

WARNS     ?=	4
WARNERR   ?=	yes

CFLAGS    +=	-DPAEXEC_VERSION='"${VERSION}"'
CFLAGS    +=	-DBUFSIZE=${BUFSIZE}

MKC_FEATURES         =	strlcat strlcpy SLIST RB getdelim
MKC_SOURCE_FUNCLIBS  =  shquote

MKC_REQUIRE_HEADERS    =	maa.h
MKC_REQUIRE_FUNCLIBS   =	pr_open2:maa
MKC_REQUIRE_FUNCS6     =	pr_open2:maa.h

MKC_COMMON_DEFINES   =	-D_GNU_SOURCE
MKC_COMMON_HEADERS   =	unistd.h stdlib.h
MKC_CHECK_TYPES      =	intptr_t:stdint.h intptr_t:inttypes.h
MKC_CHECK_HEADERS    =	sys/select.h
MKC_CHECK_FUNCS3     =  shquote

CLEANFILES           =  *~ core* *.1 *.html ktrace* ChangeLog *.tmp

.PHONY: _prepdist
_prepdist: ${MAN}

############################################################
.include <mkc.prog.mk>
