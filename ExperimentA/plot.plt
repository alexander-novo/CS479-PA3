if (!exists("outfile")) outfile='out/plot.pdf'

# set terminal pdf enhanced size 8in, 4.8in
set terminal pdf enhanced
set encoding utf8
set output outfile

set xlabel 'N'
set key inside top left

plot infile u 1:2 w linespoints