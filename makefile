CC = mpicc -fopenmp -std=c99 -cc=/opt/local/bin/gcc-mp-7 -O3

Cr = -0.7
Ci = 0.26

width = 5000
ranks = 5

mpi_modes = dynamic static

info :
	@ echo "TODO info"

clean :
	rm *.o
	rm out/*

# Tests

test : serial $(mpi_modes)
	python check.py $(width)

network_test : latency_test.o bandwidth_test.o
	mpiexec -n 2 ./latency_test.o
	mpiexec -n 2 ./bandwidth_test.o

serial : julia.o
	./julia.o $(width) $(Cr) $(Ci) serial out/serial

$(mpi_modes) : % : julia.o
	mpiexec -n $(ranks) ./julia.o $(width) $(Cr) $(Ci) $@ out/$@

# Binary definitions

latency_test.o : latency_test.c
	$(CC) latency_test.c -o latency_test.o

bandwidth_test.o : bandwidth_test.c
	$(CC) bandwidth_test.c -o bandwidth_test.o

julia.o : main.c
	$(CC) main.c -o julia.o
