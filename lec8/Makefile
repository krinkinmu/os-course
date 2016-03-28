all: clean lec8.pdf

lec8.dvi: lec8.tex
	latex lec8
#	bibtex lec8
#	latex lec8
#	latex lec8

lec8.ps: lec8.dvi
	dvips lec8.dvi

lec8.pdf: lec8.ps
	ps2pdf lec8.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav *.vrb
