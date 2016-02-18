all: clean lec3.pdf

lec3.dvi: lec3.tex
	latex lec3
#	bibtex lec3
#	latex lec3
#	latex lec3

lec3.ps: lec3.dvi
	dvips lec3.dvi

lec3.pdf: lec3.ps
	ps2pdf lec3.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav
