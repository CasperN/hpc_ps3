#!/bin/bash
#SBATCH --time=00:05:00
#SBATCH --partition=sandyb
#SBATCH --output=intranode.out
#SBATCH --error=intranode.err
#SBATCH --nodes=1
#SBATCH --ntasks per node=2
#SBATCH --cpus per task=1
#SBATCH --exclusive

module load mvapich2
mpirun -n 2 ./network_test.o
