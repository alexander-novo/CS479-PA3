if (!exists("outfile")) outfile='out/plot.pdf'

# set terminal pdf enhanced size 8in, 4.8in
set terminal pdf enhanced
set encoding utf8
set output outfile

set xlabel 'N'
set ylabel 'Accuracy'
set format y '%.0f%%'
set key inside center right title 'Information'
set title sprintf("Classification accuracy on %s-resolution images\n{/*.75Based on information preserved by eigenfaces}", dataSet)
set pointsize .66

plot infile1 u 1:(100*$2) w linespoints title title1,\
     infile2 u 1:(100*$2) w linespoints title title2,\
	 infile3 u 1:(100*$2) w linespoints title title3