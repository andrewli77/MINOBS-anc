#!/bin/bash

base=$1
cur=$(pwd)

base1="$cur/${base}_1_results"
base2="$cur/${base}_2_results"
base3="$cur/${base}_3_results"
base4="$cur/${base}_4_results"
base5="$cur/${base}_5_results"
base6="$cur/${base}_6_results"


cat $base1 >> $cur/"${base}_results"
cat $base2 >> $cur/"${base}_results"
cat $base3 >> $cur/"${base}_results"
cat $base4 >> $cur/"${base}_results"
cat $base5 >> $cur/"${base}_results"
cat $base6 >> $cur/"${base}_results"

rm $base1
rm $base2
rm $base3
rm $base4
rm $base5
rm $base6