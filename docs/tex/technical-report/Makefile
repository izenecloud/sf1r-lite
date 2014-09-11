all : sf1r-tr.tex
	pdflatex sf1r-tr
	makeindex sf1r-tr.tex
	bibtex sf1r-tr
	pdflatex sf1r-tr
	pdflatex sf1r-tr
	pdflatex sf1r-tr
	rm -rf *.aux
	rm -rf *.bbl
	rm -rf *.blg
	rm -rf *.idx
	rm -rf *.ilg
	rm -rf *.ind
	rm -rf *.log
	rm -rf *.out
	rm -rf *.toc

clean :
	rm -rf *.aux
	rm -rf *.bbl
	rm -rf *.blg
	rm -rf *.idx
	rm -rf *.ilg
	rm -rf *.ind
	rm -rf *.log
	rm -rf *.out
	rm -rf *.pdf
	rm -rf *.dvi
	rm -rf *.toc
