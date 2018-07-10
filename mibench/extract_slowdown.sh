#!/bin/bash

if [ "$1" ]; then
    file=$1
else
    file=mibench.csv
fi

cat $file | sed 1,2d | cut -d, -f9,10,13,14
