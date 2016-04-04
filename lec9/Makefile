all: clean lec9.pdf

lec9.dvi: lec9.tex
	latex lec9
#	bibtex lec9
#	latex lec9
#	latex lec9

lec9.ps: lec9.dvi
	dvips lec9.dvi

lec9.pdf: lec9.ps
	ps2pdf lec9.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav *.vrb
