#!/bin/bash

for i in `ls -1 arm/*.litmus`; do
    n=`echo $i | sed -e s/arm/armv8-gen/`
    echo "$i => $n"
    ../py/parse_litmus.py $i > $n
done
