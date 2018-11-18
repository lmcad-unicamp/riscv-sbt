#!/usr/bin/gnuplot
reset
set ylabel "Slowdown"
set xtics rotate by 33 offset 0,0 right
set ytics 1
set grid ytics
set xrange [0:21]
set yrange [0:17]
set size noratio 2,1
set key on outside top horizontal

inc='0.125'

set style data histograms
set style histogram errorbars
set boxwidth inc
set style fill solid 1.0 border -1
set style line 1 lt 1 linecolor rgb "#ff0000"
set style line 2 lt 1 linecolor rgb "#00c000"
set style line 3 lt 1 linecolor rgb "#0000ff"
set style line 4 lt 1 linecolor rgb "#ff00ff"
set style line 5 lt 1 linecolor rgb "#00c0c0"
set style line 7 lt 1 linecolor rgb "#ffffff"

set terminal postscript eps enhanced color "Arial" 14
set output f_out

l1='RV-SBT Globals'
l2='RV-SBT Locals'
l3='RV-SBT ABI'
l4='QEMU'
l5='RV8'

plot \
    f1 using ($1-4*inc):3:4:xticlabels(2) with boxerrorbars title l1 ls 1, \
    '' using ($1-3*inc):5:6   with boxerrorbars title l2 ls 2, \
    '' using ($1-2*inc):7:8   with boxerrorbars title l3 ls 3, \
    f2 using ($1-1*inc):3:4   with boxerrorbars title l4 ls 4, \
    f3 using ($1-0*inc):3:4   with boxerrorbars title l5 ls 5

# f1
#    '' using ($1-4*inc):3:3   with labels left rotate font "Arial-Bold,10" offset 0,-2 tc ls 7 notitle, \
#    '' using ($1-3*inc):5:5   with labels left rotate font "Arial-Bold,10" offset 0,-2 tc ls 7 notitle, \
#    '' using ($1-2*inc):7:7   with labels left rotate font "Arial-Bold,10" offset 0,-2 tc ls 7 notitle, \
# f2
#    '' using ($1-1*inc):3:3   with labels left rotate font "Arial-Bold,10" offset 0,-2 tc ls 7 notitle, \
# f3
#    '' using ($1-0*inc):3:3   with labels left rotate font "Arial-Bold,10" offset 0,-2 tc ls 7 notitle
