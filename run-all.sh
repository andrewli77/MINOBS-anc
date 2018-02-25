#!/bin/bash

scores=$1
dir=$2
timelimit=$3

for file in $dir/*; do
	cmd="./search $scores $file $timelimit -1 out.txt"
	eval "$cmd"
	echo "$cmd"
done