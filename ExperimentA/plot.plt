if (!exists("outfile")) outfile='out/plot.pdf'

# set terminal pdf enhanced size 8in, 4.8in
set terminal pdf enhanced size 10in, 4.8in font 'Helvetica, 15pt'
set encoding utf8
set output outfile

if (!exists("MP_LEFT"))   MP_LEFT = .0375
if (!exists("MP_RIGHT"))  MP_RIGHT = .975
if (!exists("MP_BOTTOM")) MP_BOTTOM = .125
if (!exists("MP_TOP"))    MP_TOP = .85
if (!exists("MP_xGAP"))   MP_xGAP = 0.015
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

set yrange [Y_min-3:Y_max+3]
set key at graph .95,.66 center right title 'Information' box width 2

Y_max = max(max(A_max, B_max), C_max)
Y_min = max(max(A_min, B_min), C_min)
set arrow from graph 0, first Y_max to graph 1, first Y_max nohead lc rgb "red" dashtype 4
set arrow from graph 0, first Y_min to graph 1, first Y_min nohead lc rgb "red" dashtype 4
set title title1

plot infile1 u 1:(100*$2) w linespoints title key1,\
     infile2 u 1:(100*$2) w linespoints title key2,\
	 infile3 u 1:(100*$2) w linespoints title key3

unset arrow
unset ylabel
unset key
set format y ""

Y_max = max(max(D_max, E_max), F_max)
Y_min = max(max(D_min, E_min), F_min)
set arrow from graph 0, first Y_max to graph 1, first Y_max nohead lc rgb "red" dashtype 4
set arrow from graph 0, first Y_min to graph 1, first Y_min nohead lc rgb "red" dashtype 4
set title title2

plot infile4 u 1:(100*$2) w linespoints,\
     infile5 u 1:(100*$2) w linespoints,\
	 infile6 u 1:(100*$2) w linespoints

unset multiplot