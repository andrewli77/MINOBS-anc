#!/bin/bash

base=$1
cur=$(pwd)

base1="$cur/bic/${base}_1_results"
base2="$cur/bic/${base}_2_results"
base3="$cur/bic/${base}_3_results"
base4="$cur/bic/${base}_4_results"
base5="$cur/bic/${base}_5_results"
base6="$cur/bic/${base}_6_results"


cat $base1 >> $cur/bic/"${base}_all_results"
cat $base2 >> $cur/bic/"${base}_all_results"
cat $base3 >> $cur/bic/"${base}_all_results"
cat $base4 >> $cur/bic/"${base}_all_results"
cat $base5 >> $cur/bic/"${base}_all_results"
cat $base6 >> $cur/bic/"${base}_all_results"

rm $base1
rm $base2
rm $base3
rm $base4
rm $base5
rm $base6