all: clean hw2.pdf

hw2.dvi: hw2.tex
	latex hw2
	bibtex hw2
	latex hw2
	latex hw2

hw2.ps: hw2.dvi
	dvips hw2.dvi

hw2.pdf: hw2.ps
	ps2pdf hw2.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out
