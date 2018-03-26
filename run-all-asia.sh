#!/bin/bash

eval "sbatch --time=100:00:00 ./run-all.sh data/asia_250_1.BIC data/constraints/asia data/asia_250_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/asia_250_2.BIC data/constraints/asia data/asia_250_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/asia_250_3.BIC data/constraints/asia data/asia_250_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/asia_250_4.BIC data/constraints/asia data/asia_250_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/asia_250_5.BIC data/constraints/asia data/asia_250_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/asia_250_6.BIC data/constraints/asia data/asia_250_results"

eval "sbatch --time=100:00:00 ./run-all.sh data/asia_1000_1.BIC data/constraints/asia data/asia_1000_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/asia_1000_2.BIC data/constraints/asia data/asia_1000_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/asia_1000_3.BIC data/constraints/asia data/asia_1000_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/asia_1000_4.BIC data/constraints/asia data/asia_1000_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/asia_1000_5.BIC data/constraints/asia data/asia_1000_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/asia_1000_6.BIC data/constraints/asia data/asia_1000_results"
