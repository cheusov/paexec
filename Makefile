############################################################

PREFIX?=/usr/local
BINDIR?=${PREFIX}/bin
MANDIR?=${PREFIX}/man

POD2MAN?=		pod2man
POD2HTML?=		pod2html

INST_DIR?=		${INSTALL} -d

MAALIB?=		-lmaa

# maximum length of task (line read from paexec's stdin)
# maximum length of result line (line read from command's stdout)
BUFSIZE?=		4096

# directory with paexec sources
SRCROOT?=		${.PARSEDIR}

############################################################
.include "Makefile.version"

BIRTHDATE=	2008-01-25

WARNS=		4

PROG=		paexec
SRCS=		paexec.c wrappers.c nonblock_helpers.c

LDADD+=		$(MAALIB)

CPPFLAGS+=	-DPAEXEC_VERSION='"${VERSION}"'
CPPFLAGS+=	-DBUFSIZE=${BUFSIZE}

############################################################
.PHONY: install-dirs
install-dirs:
	$(INST_DIR) ${DESTDIR}${BINDIR}
.if !defined(MKMAN) || empty(MKMAN:M[Nn][Oo])
	$(INST_DIR) ${DESTDIR}${MANDIR}/man1
.if !defined(MKCATPAGES) || empty(MKCATPAGES:M[Nn][Oo])
	$(INST_DIR) ${DESTDIR}${MANDIR}/cat1
.endif
.endif

paexec.1 : paexec.pod
	$(POD2MAN) -s 1 -r 'parallel executor' -n paexec \
	   -c 'PAEXEC manual page' ${.ALLSRC} > ${.TARGET}
paexec.html : paexec.pod
	$(POD2HTML) --infile=${.ALLSRC} --outfile=${.TARGET}

##################################################

.PHONY : test
test : paexec
	@echo 'running tests...'; \
	if cd tests && ./test.sh; \
	then echo '   succeeded'; \
	else echo '   failed'; false; \
	fi

############################################################
.PATH: ${SRCROOT}

.include <bsd.prog.mk>
