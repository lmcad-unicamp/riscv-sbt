#!/bin/bash

AUTO=$TOPDIR/scripts/auto
MEASURE_PY=$AUTO/measure.py

clean()
{
    rm -f x86-avx-*_slowdown.csv arm_slowdown.csv *.dat *.pdf
}


#f=x86-avx-gcchf.csv
#plot_single $f ${f%.csv}.pdf x86

plot_single()
{
    local in=$1
    local out=$2
    local arch=$3

    local base=${in%.csv}
    local slowdown_csv=${base}_slowdown.csv
    local slowdown_dat=${base}_slowdown.dat
    local output="| ps2pdf -dEPSCrop -dAutoRotatePages=/None - $out"

    $MEASURE_PY -x $in -m globals locals abi -c 6 7 > $slowdown_csv
    sed 1,2d $slowdown_csv | grep -v geomean | tr , ' ' | \
        awk 'BEGIN{i=1} {print i++ " " $$0}' > $slowdown_dat
    gnuplot \
        -e "f_in='$slowdown_dat'" \
        -e "f_out='$output'" \
        -e "a_title='MiBench RISC-V to $arch Translation'" \
        mibench.gnuplot
}

extract_slowdown()
{
    local in=$1
    local base=${in%.csv}
    local slowdown_csv=${base}_slowdown.csv
    local slowdown_dat=${base}_slowdown.dat

    if [ ! -f "$slowdown_csv" ]; then
        $MEASURE_PY -x $in -m globals locals abi -c 6 7 > $slowdown_csv
    fi
    if [ ! -f "$slowdown_dat" ]; then
        sed 1,2d $slowdown_csv | tr , ' ' | \
            awk 'BEGIN{i=1} {print i++ " " $$0}' > $slowdown_dat
    fi
    echo $slowdown_dat
}

merge()
{
    local first=$1
    local second=$2

    local i=1

    cat $first | while read ln1; do
        ln2=`head -n$i $second | tail -n1`

        echo "$i " $ln1 $ln2 | awk \
'{print $1 " " $3 " " \
$4 " " $5 " " $12 " " $13 " " \
$6 " " $7 " " $14 " " $15 " " \
$8 " " $9 " " $16 " " $17}'

        i=$((i+1))
    done
}

plot_vs()
{
    local label1=$1
    local label2=$2
    local arch=$3
    local first=$4
    local second=$5
    local out=$6
    local slabel1=$7
    local slabel2=$8

    local title="$label1 vs $label2 ($arch)"
    local first_dat
    local second_dat
    local dat=${out%.pdf}.dat
    # output a pdf file via ps2pdf
    local output="| ps2pdf -dEPSCrop -dAutoRotatePages=/None - $out"

    local l1="$slabel1 Globals"
    local l2="$slabel2 Globals"
    local l3="$slabel1 Locals"
    local l4="$slabel2 Locals"
    local l5="$slabel1 ABI"
    local l6="$slabel2 ABI"

    if [ -f "$out" ]; then
        return
    fi

    first_dat=`extract_slowdown $first`
    second_dat=`extract_slowdown $second`
    merge $first_dat $second_dat > $dat

    gnuplot \
        -e "f_in='$dat'" \
        -e "f_out='$output'" \
        -e "a_title='$title'" \
        -e "l1='$l1'" \
        -e "l2='$l2'" \
        -e "l3='$l3'" \
        -e "l4='$l4'" \
        -e "l5='$l5'" \
        -e "l6='$l6'" \
        vs.gnuplot
}


### main ###

if [ "$1" = clean ]; then
    clean
    exit 0
fi

set -e
set -u
set -x

plot_vs "Clang soft-float" "GCC soft-float" "x86" \
    x86-avx-clang.csv x86-avx-gccsf.csv x86-clang-vs-gcc.pdf \
    "Clang" "GCC"

plot_vs "RISC-V SBT" "OpenISA SBT" "x86" \
    x86-avx-gcchf.csv x86-oi.csv x86-rv-vs-oi.pdf \
    "RISC-V" "OpenISA"

plot_vs "RISC-V SBT" "OpenISA SBT" "ARM" \
    arm.csv arm-oi.csv arm-rv-vs-oi.pdf \
    "RISC-V" "OpenISA"
