if (!exists("outfile")) outfile='out/plot.pdf'

# set terminal pdf enhanced size 8in, 4.8in
set terminal pdf enhanced size 12in, 4.8in
set encoding utf8
set output outfile

if (!exists("MP_LEFT"))   MP_LEFT = .03
if (!exists("MP_RIGHT"))  MP_RIGHT = .975
if (!exists("MP_BOTTOM")) MP_BOTTOM = .14
if (!exists("MP_TOP"))    MP_TOP = .85
if (!exists("MP_xGAP"))   MP_xGAP = 0.01
if (!exists("MP_yGAP"))   MP_yGAP = 0.02
set multiplot layout 1,2 title "Comparison of classification accuracies\n{/*.75Based on information preserved by eigenfaces}"\
	margins screen MP_LEFT, MP_RIGHT, MP_BOTTOM, MP_TOP spacing screen MP_xGAP, MP_yGAP

set xlabel 'N'
set ylabel 'Accuracy'
set format y '%.0f%%'
set pointsize .66

min(x, y) = (x < y ? x : y)
max(x, y) = (x > y ? x : y)
stats infile1 u (100*$2) prefix 'A' nooutput
stats infile2 u (100*$2) prefix 'B' nooutput
stats infile3 u (100*$2) prefix 'C' nooutput
stats infile4 u (100*$2) prefix 'D' nooutput
stats infile5 u (100*$2) prefix 'E' nooutput
stats infile6 u (100*$2) prefix 'F' nooutput
Y_min = min(min(min(min(min(A_min, B_min), C_min), D_min), E_min), F_min)
Y_max = max(max(max(max(max(A_max, B_max), C_max), D_max), E_max), F_max)

set yrange [Y_min:Y_max]
set offsets 0, 0, 3, 3
unset key

Y_max = max(max(A_max, B_max), C_max)
set arrow from graph 0, first Y_max to graph 1, first Y_max nohead lc rgb "red" dashtype 4
set title title1

plot infile1 u 1:(100*$2) w linespoints,\
     infile2 u 1:(100*$2) w linespoints,\
	 infile3 u 1:(100*$2) w linespoints

unset arrow
unset ylabel

set format y ""
set key inside center right title 'Information'

Y_max = max(max(D_max, E_max), F_max)
set arrow from graph 0, first Y_max to graph 1, first Y_max nohead lc rgb "red" dashtype 4
set title title2

plot infile4 u 1:(100*$2) w linespoints title key1,\
     infile5 u 1:(100*$2) w linespoints title key2,\
	 infile6 u 1:(100*$2) w linespoints title key3

unset multiplot