all: clean lec11.pdf

lec11.dvi: lec11.tex
	latex lec11
#	bibtex lec11
#	latex lec11
#	latex lec11

lec11.ps: lec11.dvi
	dvips lec11.dvi

lec11.pdf: lec11.ps
	ps2pdf lec11.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav *.vrb
