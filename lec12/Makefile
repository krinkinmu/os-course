all: clean lec12.pdf

lec12.dvi: lec12.tex
	latex lec12
#	bibtex lec12
#	latex lec12
#	latex lec12

lec12.ps: lec12.dvi
	dvips lec12.dvi

lec12.pdf: lec12.ps
	ps2pdf lec12.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav *.vrb
