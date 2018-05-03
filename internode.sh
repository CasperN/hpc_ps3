#!/bin/bash
#SBATCH   time=00:05:00
#SBATCH   partition=sandyb
#SBATCH   nodes=2
#SBATCH   ntasks per node=1
#SBATCH   cpus per task=1
#SBATCH   exclusive

module load mvapich2
mpirun -n 2 ./network_test.o
