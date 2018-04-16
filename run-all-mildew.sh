#!/bin/bash

eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/mildew_8000_1.BIC data/constraints/mildew data/bic/mildew_8000_1_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/mildew_8000_2.BIC data/constraints/mildew data/bic/mildew_8000_2_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/mildew_8000_3.BIC data/constraints/mildew data/bic/mildew_8000_3_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/mildew_8000_4.BIC data/constraints/mildew data/bic/mildew_8000_4_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/mildew_8000_5.BIC data/constraints/mildew data/bic/mildew_8000_5_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/mildew_8000_6.BIC data/constraints/mildew data/bic/mildew_8000_6_results 10"

eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/mildew_32000_1.BIC data/constraints/mildew data/bic/mildew_32000_1_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/mildew_32000_2.BIC data/constraints/mildew data/bic/mildew_32000_2_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/mildew_32000_3.BIC data/constraints/mildew data/bic/mildew_32000_3_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/mildew_32000_4.BIC data/constraints/mildew data/bic/mildew_32000_4_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/mildew_32000_5.BIC data/constraints/mildew data/bic/mildew_32000_5_results 10"
eval "sbatch --time=40:00:00 --mem-per-cpu=1024 ./run-all.sh data/bic/mildew_32000_6.BIC data/constraints/mildew data/bic/mildew_32000_6_results 10"
