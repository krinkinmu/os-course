all: clean lec4.pdf

lec4.dvi: lec4.tex
	latex lec4
#	bibtex lec4
#	latex lec4
#	latex lec4

lec4.ps: lec4.dvi
	dvips lec4.dvi

lec4.pdf: lec4.ps
	ps2pdf lec4.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav
