#!/bin/bash

eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/water_1000_1.BIC data/constraints/water data/bic/water_1000_1_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/water_1000_2.BIC data/constraints/water data/bic/water_1000_2_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/water_1000_3.BIC data/constraints/water data/bic/water_1000_3_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/water_1000_4.BIC data/constraints/water data/bic/water_1000_4_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/water_1000_5.BIC data/constraints/water data/bic/water_1000_5_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/water_1000_6.BIC data/constraints/water data/bic/water_1000_6_results 10"

eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/water_4000_1.BIC data/constraints/water data/bic/water_4000_1_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/water_4000_2.BIC data/constraints/water data/bic/water_4000_2_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/water_4000_3.BIC data/constraints/water data/bic/water_4000_3_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/water_4000_4.BIC data/constraints/water data/bic/water_4000_4_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/water_4000_5.BIC data/constraints/water data/bic/water_4000_5_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/water_4000_6.BIC data/constraints/water data/bic/water_4000_6_results 10"