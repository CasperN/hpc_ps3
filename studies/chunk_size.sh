#!/bin/bash
#SBATCH --time=00:05:00
#SBATCH --partition=sandyb
#SBATCH --output=out/scaling2.out
#SBATCH --error=out/scaling2.err
#SBATCH --nodes=2
#SBATCH --ntasks per node=16
#SBATCH --cpus per task=1
#SBATCH --exclusive

width=12000
Cr=-.7
Ci=.26
rank=16

module load mvapich2

for chunk in 10 100 1000 10000 100000 1000000
do
    echo rank $rank width $width $Cr+$Ci\i dynamic out/dy_cs_$chunk $chunk
    mpirun -n $rank ./julia.o $width $Cr $Ci dynamic out/dy_cs_$chunk $chunk
done
