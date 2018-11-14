#!/usr/bin/gnuplot
reset
#set title a_title
set ylabel "Slowdown"
set xtics rotate by 33 offset 0,0 right
set grid ytics
set xrange [0:21]
set yrange [0:3]
set size noratio 2,1
set key on outside top horizontal

inc='0.25'

set style data histograms
set style histogram errorbars
set boxwidth inc
set style fill solid 1.0 border -1
set style line 1 lt 1 linecolor rgb "#ff0000"
set style line 2 lt 1 linecolor rgb "#008000"
set style line 3 lt 1 linecolor rgb "#0000ff"
set style line 4 lt 1 linecolor rgb "#ffffff"

set terminal postscript eps enhanced color "Arial" 14
set output f_out

plot \
  f_in using ($1-2*inc):3:4:xticlabels(2) with boxerrorbars title l1 ls 1, \
    '' using ($1-1*inc):5:6   with boxerrorbars title l2 ls 2, \
    '' using ($1-0*inc):7:8   with boxerrorbars title l3 ls 3, \
    '' using ($1-2*inc):3:3   with labels left rotate font "Arial-Bold,10" offset 0,-2 tc ls 4 notitle, \
    '' using ($1-1*inc):5:5   with labels left rotate font "Arial-Bold,10" offset 0,-2 tc ls 4 notitle, \
    '' using ($1-0*inc):7:7   with labels left rotate font "Arial-Bold,10" offset 0,-2 tc ls 4 notitle

