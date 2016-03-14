all: clean hw3.pdf

hw3.dvi: hw3.tex
	latex hw3
#	bibtex hw3
#	latex hw3
	latex hw3

hw3.ps: hw3.dvi
	dvips hw3.dvi

hw3.pdf: hw3.ps
	ps2pdf hw3.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out
