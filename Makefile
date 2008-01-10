############################################################

PREFIX?=/usr/local
BINDIR?=${PREFIX}/bin
MANDIR?=${PREFIX}/man

POD2MAN?=		pod2man
POD2HTML?=		pod2html

INST_DIR?=	${INSTALL} -d

MAALIB?=	-lmaa

# maximum length of task (line read from paexec's stdin)
# maximum length of result line (line read from command's stdout)
BUFSIZE?=	4096

############################################################

PROG=		paexec
SRCS=		paexec.c wrappers.c nonblock_helpers.c

VERSION=	0.6.0

LDADD+=		$(MAALIB)

CPPFLAGS+=	-DPAEXEC_VERSION='"${VERSION}"'
CPPFLAGS+=	-DBUFSIZE=${BUFSIZE}

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

paexec.1 : paexec.pod
	$(POD2MAN) -s 1 -r 'AWK Wrapper' -n paexec \
	   -c 'PAEXEC manual page' paexec.pod > $@
paexec.html : paexec.pod
	$(POD2HTML) --infile=paexec.pod --outfile=$@

##################################################

.PHONY : test
test : paexec
	echo 'running tests...'; \
	if cd tests && ./test.sh > _test.res && diff -u test.out _test.res; \
	then echo '   succeeded'; \
	else echo '   failed'; \
	fi

############################################################
.include "Makefile.cvsdist"

.include <bsd.prog.mk>
