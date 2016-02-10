set terminal postscript eps enhanced color font 'Verdana,10'
set output outputfile
set xlabel "size in bytes"
set ylabel "allocation number"
set autoscale xfix
set grid
set boxwidth 0.95 relative
set style fill transparent solid 0.8 noborder

plot inputfile using 1:2 with boxes lc rgb"blue" notitle
