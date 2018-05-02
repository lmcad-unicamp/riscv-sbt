#!/bin/bash

cat mibench.csv | sed 1,2d | cut -d, -f9,10,13,14
