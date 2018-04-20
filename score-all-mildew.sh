#!/bin/bash

palim="3"


for file in $(eval "ls data/mildew/mildew_*.csv"); do
	fileBaseName=$(eval "basename ${file} .csv")

	# Create the gobnilp.set settings file
	echo "gobnilp/outputfile/scores = \"data/${fileBaseName}.BDeu\"" > gobnilp.set
	echo "gobnilp/delimiter = \",\""  >> gobnilp.set
	echo "gobnilp/scoring/prune = FALSE"  >> gobnilp.set
	echo "gobnilp/mergedelimiters = FALSE"  >> gobnilp.set
	echo "gobnilp/scoring/palim = ${palim}"  >> gobnilp.set

	eval "bin/gobnilp -f=dat -v=5 -x ${file}"

	exit
done