#!/bin/bash

eval "sbatch --time=100:00:00 ./run-all.sh data/child_500_1.BIC data/constraints/child data/child_500_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/child_500_2.BIC data/constraints/child data/child_500_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/child_500_3.BIC data/constraints/child data/child_500_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/child_500_4.BIC data/constraints/child data/child_500_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/child_500_5.BIC data/constraints/child data/child_500_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/child_500_6.BIC data/constraints/child data/child_500_results"

eval "sbatch --time=100:00:00 ./run-all.sh data/child_2000_1.BIC data/constraints/child data/child_2000_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/child_2000_2.BIC data/constraints/child data/child_2000_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/child_2000_3.BIC data/constraints/child data/child_2000_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/child_2000_4.BIC data/constraints/child data/child_2000_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/child_2000_5.BIC data/constraints/child data/child_2000_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/child_2000_6.BIC data/constraints/child data/child_2000_results"
