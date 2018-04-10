#!/bin/bash

scores=$1
dir=$2
outFile=$3
gens=$4

for file in $dir/*; do
	cmd="sbatch --time=12:00:00 --mem-per-cpu=2048 ./run-one-case.sh $scores $file $outFile $gens"
	eval "$cmd"
done