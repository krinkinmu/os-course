all: clean lec7.pdf

lec7.dvi: lec7.tex
	latex lec7
#	bibtex lec7
#	latex lec7
#	latex lec7

lec7.ps: lec7.dvi
	dvips lec7.dvi

lec7.pdf: lec7.ps
	ps2pdf lec7.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav *.vrb
