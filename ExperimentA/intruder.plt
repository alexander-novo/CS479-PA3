if (!exists("outfile")) outfile='out/plot.pdf'

# set terminal pdf enhanced size 8in, 4.8in
set terminal pdf enhanced
set encoding utf8
set output outfile

set xlabel 'False Positives'
set ylabel 'True Positives'
set key inside center right title 'Resolution' 
set title "Comparison of intruder ROC curves"

set offsets 0.1,0,0.1,0
set autoscale fix

stats infile1 u 2 prefix 'A1'
stats infile1 u 3 prefix 'B1'
stats infile2 u 2 prefix 'A2'
stats infile2 u 3 prefix 'B2'

plot infile1 u ($2/A1_max):($3/B1_max) w lines title title1,\
     infile2 u ($2/A2_max):($3/B2_max) w lines title title2,\