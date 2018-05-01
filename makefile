CC = mpicc -fopenmp -std=c99 -cc=/opt/local/bin/gcc-mp-7 -O3

Cr = -0.7
Ci = 0.26

width = 5000
ranks = 5

mpi_modes = dynamic static

info :
	@ echo "TODO info"

test : serial $(mpi_modes)
	python check.py $(width)


serial : julia.o
	./julia.o $(width) $(Cr) $(Ci) serial out/serial

$(mpi_modes) : % : julia.o
	mpiexec -n $(ranks) ./julia.o $(width) $(Cr) $(Ci) $@ out/$@


julia.o : main.c
	$(CC) main.c -o julia.o
