all: clean hw1.pdf

hw1.dvi: hw1.tex
	latex hw1
	bibtex hw1
	latex hw1
	latex hw1

hw1.ps: hw1.dvi
	dvips hw1.dvi

hw1.pdf: hw1.ps
	ps2pdf hw1.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out
