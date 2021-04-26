if (!exists("outfile")) outfile='out/plot.pdf'

# Square since each axis is a rate
set terminal pdf enhanced size 4.8in, 4.8in
set encoding utf8
set output outfile

set xlabel 'False Positive Rate'
set ylabel 'True Positive Rate'
set key inside center right title 'Resolution' 
set title "Comparison of intruder ROC curves"

set offsets 0.1,0,0.1,0
set autoscale fix

stats infile1 u 2 prefix 'A1' nooutput
stats infile1 u 3 prefix 'B1' nooutput
stats infile2 u 2 prefix 'A2' nooutput
stats infile2 u 3 prefix 'B2' nooutput

plot infile1 u ($2/A1_max):($3/B1_max) w lines title title1,\
     infile2 u ($2/A2_max):($3/B2_max) w lines title title2,\