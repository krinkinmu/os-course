all: clean lec1.pdf

lec1.dvi: lec1.tex
	latex lec1
#	bibtex lec1
#	latex lec1
#	latex lec1

lec1.ps: lec1.dvi
	dvips lec1.dvi

lec1.pdf: lec1.ps
	ps2pdf lec1.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav
