##################################################

FILES    = paexec.pdf
FILESDIR = ${DOCDIR}

.PHONE : all
all : paexec.pdf

.PHONY : pdf dvi
pdf : paexec.pdf
ps  : paexec.ps
dvi : paexec.dvi

.SUFFIXES: .ps .eps .pdf .dvi .tex .dot

paexec.ps paexec.dvi: dep-graph.eps

.ps.pdf:
	ps2pdf "$<" "$@"

.dot.eps:
	dot -Tps ${.IMPSRC} > ${.TARGET}

.dvi.ps:
	dvips $<

.tex.dvi:
	latex "${.IMPSRC}" && latex "${.IMPSRC}"

GARBAGE     =	*.dvi *.aux *.vrb *.toc *.snm *.log *.nav *.out *.eps *.ps
CLEANFILES +=	${GARBAGE} *.pdf

.PHONY: _clean_garbage
_clean_garbage:
	rm -f ${GARBAGE}

_prepdist: all _clean_garbage

.include <mkc.files.mk>
