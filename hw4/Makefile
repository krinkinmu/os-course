all: clean hw4.pdf

hw4.dvi: hw4.tex
	latex hw4
#	bibtex hw4
#	latex hw4
	latex hw4

hw4.ps: hw4.dvi
	dvips hw4.dvi

hw4.pdf: hw4.ps
	ps2pdf hw4.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out
