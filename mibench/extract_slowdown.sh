#!/bin/bash

if [ "$1" ]; then
    file=$1
else
    file=mibench.csv
fi

cat $file | sed 1,2d | cut -d, -f15,16,23,24
