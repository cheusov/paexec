# initial buffer size for the tasks and results
BUFSIZE   ?=	4096

############################################################
BIRTHDATE  =	2008-01-25

WARNS     ?=	4
WARNERR   ?=	yes

PROG       =	paexec
SRCS       =	paexec.c wrappers.c tasks.c nodes.c signals.c pr.c

SCRIPTS    =	paexec_reorder

MAN        =	paexec.1 paexec_reorder.1

CFLAGS    +=	-DPAEXEC_VERSION='"${VERSION}"'
CFLAGS    +=	-DBUFSIZE=${BUFSIZE}

MKC_FEATURES         =	getline SLIST RB
MKC_SOURCE_FUNCLIBS  =  shquote

MKC_COMMON_DEFINES   =	-D_GNU_SOURCE
MKC_COMMON_HEADERS   =	unistd.h stdlib.h
MKC_CHECK_TYPES      =	intptr_t:stdint.h intptr_t:inttypes.h
MKC_CHECK_HEADERS    =	sys/select.h
MKC_CHECK_FUNCS3     =  shquote
MKC_CHECK_FUNCS1     =	sysconf

CLEANFILES           =  *~ core* *.1 *.html ktrace* ChangeLog *.tmp

.PHONY: _prepdist
_prepdist: ${MAN}

############################################################
.include <mkc.prog.mk>

CFLAGS.warns.gcc.4 += -Wno-uninitialized
