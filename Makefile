############################################################

PREFIX?=		/usr/local
BINDIR?=		${PREFIX}/bin
MANDIR?=		${PREFIX}/man

MKHTML?=		no

POD2MAN?=		pod2man
POD2HTML?=		pod2html

INST_DIR?=		${INSTALL} -d

MAALIB?=		-lmaa

# initial buffer size for the tasks
#    (line read from paexec's stdin)
# initial buffer size for the result line
#    (line read from command's stdout)
BUFSIZE?=		4096

############################################################
.include "Makefile.version"

BIRTHDATE=	2008-01-25

WARNS?=		4

PROG=		paexec
SRCS=		paexec.c wrappers.c

LDADD+=		$(MAALIB)

CFLAGS+=	-DPAEXEC_VERSION='"${VERSION}"'
CFLAGS+=	-DBUFSIZE=${BUFSIZE}

############################################################
.PHONY: installdirs
installdirs:
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

CLEANFILES=  *~ core* paexec.1 *.cat1 ktrace* ChangeLog *.tmp
CLEANFILES+= paexec.html transport_closed_stdin

${.OBJDIR}/transport_closed_stdin: examples/broken_echo/transport_closed_stdin.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $(.TARGET) $(.ALLSRC) $(LDFLAGS)

.PHONY : test
test : paexec ${.OBJDIR}/transport_closed_stdin; \
	@echo 'running tests...'; \
	export OBJDIR=${.OBJDIR}; \
	if cd ${.CURDIR}/tests && ./test.sh; \
	then echo '   succeeded'; \
	else echo '   failed'; false; \
	fi

############################################################

.include <bsd.prog.mk>
