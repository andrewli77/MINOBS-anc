#!/bin/bash

scores=$1
dir=$2
outFile=$3

for file in $dir/*; do
	cmd="./run-one-case.sh $scores $file $outFile"
	eval "$cmd"
done