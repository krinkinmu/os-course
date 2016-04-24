all: clean hw5.pdf

hw5.dvi: hw5.tex
	latex hw5
#	bibtex hw5
#	latex hw5
	latex hw5

hw5.ps: hw5.dvi
	dvips hw5.dvi

hw5.pdf: hw5.ps
	ps2pdf hw5.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out
