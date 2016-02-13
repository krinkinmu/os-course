all: clean lec2.pdf

lec2.dvi: lec2.tex
	latex lec2
#	bibtex lec2
#	latex lec2
#	latex lec2

lec2.ps: lec2.dvi
	dvips lec2.dvi

lec2.pdf: lec2.ps
	ps2pdf lec2.ps

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav
