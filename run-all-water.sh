#!/bin/bash

eval "./run-all.sh data/insurance_1000.BIC data/constraints/insurance 10"
eval "./run-all.sh data/insurance_4000.BIC data/constraints/insurance 10"
eval "./run-all.sh data/insurance_16000.BIC data/constraints/insurance 10"

eval "./run-all.sh data/insurance_1000_no_prune.BIC data/constraints/insurance 10"
eval "./run-all.sh data/insurance_4000_no_prune.BIC data/constraints/insurance 10"