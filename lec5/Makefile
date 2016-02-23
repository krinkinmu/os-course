all: clean lec5.pdf

lec5.dvi: lec5.tex
	latex lec5
#	bibtex lec5
#	latex lec5
#	latex lec5

lec5.ps: lec5.dvi
	dvips lec5.dvi

lec5.pdf: lec5.ps
	ps2pdf lec5.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav
