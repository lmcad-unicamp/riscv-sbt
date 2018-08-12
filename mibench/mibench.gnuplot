#!/usr/bin/gnuplot
reset
set title "MiBench RISC-V to x86 Translation"
set ylabel "Slowdown"
set xtics rotate by 33 offset 0,0 right
set grid ytics
set xrange [0:20]
set yrange [0:3]
set key on

set style data histograms
set style histogram errorbars
set boxwidth 0.2
set style fill solid 1.0 border -1
set style line 1 lt 1 linecolor rgb "#4575b4"
set style line 2 lt 1 linecolor rgb "#91bfdb"
set style line 3 lt 1 linecolor rgb "#e0f3f8"

set terminal postscript eps enhanced color
# output a pdf file via ps2pdf
set output '| ps2pdf -dEPSCrop -dAutoRotatePages=/None - mibench.pdf'

plot 'mibench_slowdown.dat' using ($1-0.2):3:4:xticlabels(2) with boxerrorbars title "Globals" ls 1, \
                         '' using ($1):5:6 with boxerrorbars title "Locals" ls 2, \
                         '' using ($1+0.2):7:8 with boxerrorbars title "ABI" ls 3
