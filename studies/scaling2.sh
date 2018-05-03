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
Cr=-0.7
Ci=0.26
ranks='32'
modes='static dynamic'

module load mvapich2

for r in $ranks
do
    for mode in $modes
    do
        echo rank $r width $width $Cr+$Ci\i mode $mode save out/$mode\_$r
        mpirun -n $r ./julia.o $width $Cr $Ci $mode out/$mode\_$r
    done
done
