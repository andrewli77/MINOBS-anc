#!/bin/bash

eval "sbatch --time=100:00:00 ./run-all.sh data/alarm_1000_1.BIC data/constraints/alarm data/alarm_1000_1_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/alarm_1000_2.BIC data/constraints/alarm data/alarm_1000_2_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/alarm_1000_3.BIC data/constraints/alarm data/alarm_1000_3_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/alarm_1000_4.BIC data/constraints/alarm data/alarm_1000_4_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/alarm_1000_5.BIC data/constraints/alarm data/alarm_1000_5_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/alarm_1000_6.BIC data/constraints/alarm data/alarm_1000_6_results"

eval "sbatch --time=100:00:00 ./run-all.sh data/alarm_4000_1.BIC data/constraints/alarm data/alarm_4000_1_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/alarm_4000_2.BIC data/constraints/alarm data/alarm_4000_2_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/alarm_4000_3.BIC data/constraints/alarm data/alarm_4000_3_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/alarm_4000_4.BIC data/constraints/alarm data/alarm_4000_4_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/alarm_4000_5.BIC data/constraints/alarm data/alarm_4000_5_results"
eval "sbatch --time=100:00:00 ./run-all.sh data/alarm_4000_6.BIC data/constraints/alarm data/alarm_4000_6_results"
