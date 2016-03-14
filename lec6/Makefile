all: clean lec6.pdf

lec6.dvi: lec6.tex
	latex lec6
#	bibtex lec6
#	latex lec6
#	latex lec6

lec6.ps: lec6.dvi
	dvips lec6.dvi

lec6.pdf: lec6.ps
	ps2pdf lec6.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav *.vrb
