#!/bin/bash

eval "sbatch --time=12:00:00 ./run-all.sh data/alarm_1000_1.BIC data/constraints/alarm 10"
eval "sbatch --time=12:00:00 ./run-all.sh data/alarm_1000_2.BIC data/constraints/alarm 10"
eval "sbatch --time=12:00:00 ./run-all.sh data/alarm_1000_3.BIC data/constraints/alarm 10"
eval "sbatch --time=12:00:00 ./run-all.sh data/alarm_1000_4.BIC data/constraints/alarm 10"
eval "sbatch --time=12:00:00 ./run-all.sh data/alarm_1000_5.BIC data/constraints/alarm 10"

eval "sbatch --time=12:00:00 ./run-all.sh data/alarm_4000_1.BIC data/constraints/alarm 10"
eval "sbatch --time=12:00:00 ./run-all.sh data/alarm_4000_2.BIC data/constraints/alarm 10"
eval "sbatch --time=12:00:00 ./run-all.sh data/alarm_4000_3.BIC data/constraints/alarm 10"
eval "sbatch --time=12:00:00 ./run-all.sh data/alarm_4000_4.BIC data/constraints/alarm 10"
eval "sbatch --time=12:00:00 ./run-all.sh data/alarm_4000_5.BIC data/constraints/alarm 10"