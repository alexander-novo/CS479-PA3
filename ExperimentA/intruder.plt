if (!exists("outfile")) outfile='out/plot.pdf'

# set terminal pdf enhanced size 8in, 4.8in
set terminal pdf enhanced
set encoding utf8
set output outfile

set xlabel 'False Positives'
set ylabel 'True Positives'
set title sprintf("Intruder ROC curve on %s-resolution images", dataSet)

stats infile u 2 prefix 'A'
stats infile u 3 prefix 'B'

plot infile u ($2/A_max):($3/B_max) w lines notitle