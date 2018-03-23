#!/bin/bash

scores=$1
dir=$2
timelimit=$3

for file in $dir/*; do
	cmd="sbatch --time=12:00:00 ./run-one-case.sh $scores $file"
	eval "$cmd"
	sleep 1
done