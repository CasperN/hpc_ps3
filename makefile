CC = mpicc -fopenmp -std=c99 -cc=/opt/local/bin/gcc-mp-7 -O3
Cr = -0.7
Ci = 0.26
width = 5000
ranks = 5
mpi_modes = dynamic static

info :
	@ echo parameters:
	@ echo
	@ head -n 7 makefile
	@ echo to run network_test locally "'make network_test'"
	@ echo to check versions are consistent "'make test'"



clean :
	rm *.o
	rm out/*

# Tests

test : serial $(mpi_modes)
	python check.py $(width)

network_test : network_test.o
	mpiexec -n 2 ./network_test.o

serial : julia.o
	./julia.o $(width) $(Cr) $(Ci) serial out/serial

$(mpi_modes) : % : julia.o
	mpiexec -n $(ranks) ./julia.o $(width) $(Cr) $(Ci) $@ out/$@

# Binary definitions

network_test.o : network_test.c
	$(CC) network_test.c -o network_test.o

julia.o : main.c
	$(CC) main.c -o julia.o
