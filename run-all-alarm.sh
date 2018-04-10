#!/bin/bash

eval "sbatch --time=100:00:00 --mem-per-cpu=1024 ./run-all.sh data/alarm_1000_1.aBIC data/constraints/alarm data/alarm_1000_1_results 10"
eval "sbatch --time=100:00:00 --mem-per-cpu=1024 ./run-all.sh data/alarm_1000_2.aBIC data/constraints/alarm data/alarm_1000_2_results 10"
eval "sbatch --time=100:00:00 --mem-per-cpu=1024 ./run-all.sh data/alarm_1000_3.aBIC data/constraints/alarm data/alarm_1000_3_results 10"
eval "sbatch --time=100:00:00 --mem-per-cpu=1024 ./run-all.sh data/alarm_1000_4.aBIC data/constraints/alarm data/alarm_1000_4_results 10"
eval "sbatch --time=100:00:00 --mem-per-cpu=1024 ./run-all.sh data/alarm_1000_5.aBIC data/constraints/alarm data/alarm_1000_5_results 10"
eval "sbatch --time=100:00:00 --mem-per-cpu=1024 ./run-all.sh data/alarm_1000_6.aBIC data/constraints/alarm data/alarm_1000_6_results 10"

eval "sbatch --time=100:00:00 --mem-per-cpu=1024 ./run-all.sh data/alarm_4000_1.aBIC data/constraints/alarm data/alarm_4000_1_results 10"
eval "sbatch --time=100:00:00 --mem-per-cpu=1024 ./run-all.sh data/alarm_4000_2.aBIC data/constraints/alarm data/alarm_4000_2_results 10"
eval "sbatch --time=100:00:00 --mem-per-cpu=1024 ./run-all.sh data/alarm_4000_3.aBIC data/constraints/alarm data/alarm_4000_3_results 10"
eval "sbatch --time=100:00:00 --mem-per-cpu=1024 ./run-all.sh data/alarm_4000_4.aBIC data/constraints/alarm data/alarm_4000_4_results 10"
eval "sbatch --time=100:00:00 --mem-per-cpu=1024 ./run-all.sh data/alarm_4000_5.aBIC data/constraints/alarm data/alarm_4000_5_results 10"
eval "sbatch --time=100:00:00 --mem-per-cpu=1024 ./run-all.sh data/alarm_4000_6.aBIC data/constraints/alarm data/alarm_4000_6_results 10"
