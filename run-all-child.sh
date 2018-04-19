#!/bin/bash

eval "sbatch --time=4:00:00 ./run-all.sh data/bic/child_500_1.BIC data/all-constraints/child data/bic/child_500_1_results 10"
eval "sbatch --time=4:00:00 ./run-all.sh data/bic/child_500_2.BIC data/all-constraints/child data/bic/child_500_2_results 10"
eval "sbatch --time=4:00:00 ./run-all.sh data/bic/child_500_3.BIC data/all-constraints/child data/bic/child_500_3_results 10"
eval "sbatch --time=4:00:00 ./run-all.sh data/bic/child_500_4.BIC data/all-constraints/child data/bic/child_500_4_results 10"
eval "sbatch --time=4:00:00 ./run-all.sh data/bic/child_500_5.BIC data/all-constraints/child data/bic/child_500_5_results 10"
eval "sbatch --time=4:00:00 ./run-all.sh data/bic/child_500_6.BIC data/all-constraints/child data/bic/child_500_6_results 10"

eval "sbatch --time=4:00:00 ./run-all.sh data/bic/child_2000_1.BIC data/all-constraints/child data/bic/child_2000_1_results 10"
eval "sbatch --time=4:00:00 ./run-all.sh data/bic/child_2000_2.BIC data/all-constraints/child data/bic/child_2000_2_results 10"
eval "sbatch --time=4:00:00 ./run-all.sh data/bic/child_2000_3.BIC data/all-constraints/child data/bic/child_2000_3_results 10"
eval "sbatch --time=4:00:00 ./run-all.sh data/bic/child_2000_4.BIC data/all-constraints/child data/bic/child_2000_4_results 10"
eval "sbatch --time=4:00:00 ./run-all.sh data/bic/child_2000_5.BIC data/all-constraints/child data/bic/child_2000_5_results 10"
eval "sbatch --time=4:00:00 ./run-all.sh data/bic/child_2000_6.BIC data/all-constraints/child data/bic/child_2000_6_results 10"
