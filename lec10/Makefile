all: clean lec10.pdf

lec10.dvi: lec10.tex
	latex lec10
#	bibtex lec10
#	latex lec10
#	latex lec10

lec10.ps: lec10.dvi
	dvips lec10.dvi

lec10.pdf: lec10.ps
	ps2pdf lec10.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav *.vrb
