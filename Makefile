############################################################

PREFIX?=/usr/local
BINDIR?=${PREFIX}/bin
MANDIR?=${PREFIX}/man

INST_DIR?=	${INSTALL} -d

############################################################

PROG=		paexec
SRCS=		paexec.c wrappers.c nonblock_helpers.c

VERSION=	0.6.0

MAALIB?=	-lmaa

LDADD+=		$(MAALIB)

MKMAN=		no

############################################################
.PHONY: install-dirs
install-dirs:
	$(INST_DIR) ${DESTDIR}${BINDIR}
.if "$(MKMAN)" != "no"
	$(INST_DIR) ${DESTDIR}${MANDIR}/man1
.if "$(MKCATPAGES)" != "no"
	$(INST_DIR) ${DESTDIR}${MANDIR}/cat1
.endif
.endif

############################################################
.include "Makefile.cvsdist"

.include <bsd.prog.mk>
