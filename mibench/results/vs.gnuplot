#!/usr/bin/gnuplot
reset
#set title a_title
set ylabel "Slowdown"
set xtics rotate by 33 offset 0,0 right
set grid ytics
set xrange [0:21]
set yrange [0:3]
set size noratio 2,1
set key on

set style data histograms
set style histogram errorbars
set boxwidth 0.1
set style fill solid 1.0 border -1
set style line 1 lt 1 linecolor rgb "#ff0000"
set style line 2 lt 1 linecolor rgb "#ff8080"
set style line 3 lt 1 linecolor rgb "#00ff00"
set style line 4 lt 1 linecolor rgb "#80ff80"
set style line 5 lt 1 linecolor rgb "#0000ff"
set style line 6 lt 1 linecolor rgb "#8080ff"

set terminal postscript eps enhanced color "Arial" 14
set output f_out

plot f_in using ($1-0.2):3:4:xticlabels(2) with boxerrorbars title l1 ls 1, \
    '' using ($1-0.1):5:6   with boxerrorbars title l2 ls 2, \
    '' using ($1):7:8       with boxerrorbars title l3 ls 3, \
    '' using ($1+0.1):9:10  with boxerrorbars title l4 ls 4, \
    '' using ($1+0.2):11:12 with boxerrorbars title l5 ls 5, \
    '' using ($1+0.3):13:14 with boxerrorbars title l6 ls 6
