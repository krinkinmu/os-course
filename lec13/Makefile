all: clean lec13.pdf

lec13.dvi: lec13.tex
	latex lec13
#	bibtex lec13
#	latex lec13
#	latex lec13

lec13.ps: lec13.dvi
	dvips lec13.dvi

lec13.pdf: lec13.ps
	ps2pdf lec13.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav *.vrb
