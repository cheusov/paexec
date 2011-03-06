.for f in big_result_cmd transport_broken_rnd
all: ${f}
${f} : ${f}.in
	sed 's|@awk@|${AWK}|g' ${f}.in > $@; chmod +x $@
CLEANFILES += ${f}
.endfor

SUBPRJ    =	broken_echo

.include <mkc.subprj.mk>
