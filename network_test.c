#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>

#define LATENCY_REPEAT 20000
#define BANDWIDTH_REPEAT 16
#define MAG 3

void latency(int rank, int repeat){

    int count, send, recv, tag;
    double start, stop, usec;

    send = recv = rank; // arbitrary
    count = tag = 1;

    start = MPI_Wtime();
    for(int i=0; i<repeat; i++){
        MPI_Sendrecv(&send, count, MPI_INT, 1 - rank, tag,
                     &recv, count, MPI_INT, 1 - rank, tag,
                     MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
    }
    stop = MPI_Wtime();
    if(rank){
        printf("Latency:\n");
        usec = (stop-start) / repeat * 1000000;
        printf("%lf usec/exchange avg over %5d packets\n", usec, repeat);
    }
}


void bandwidth(int rank, int repeat_max, int magnitudes){

    int count, *buf, tag, repeat;
    double start, stop, msec;

    if(rank) printf("Bandwidth:\n");

    for(int m=1; m <= magnitudes; m++){
        count = 1 << 10 * m;
        repeat = repeat_max << (magnitudes - m) * 5;
        buf = malloc(count);

        if(rank) start = MPI_Wtime();

        for(int i=0; i<repeat; i++){

            if(rank)
                MPI_Send(buf, count, MPI_CHAR, 1 - rank, m, MPI_COMM_WORLD);
            else
                MPI_Recv(buf, count, MPI_CHAR, 1 - rank, m, MPI_COMM_WORLD,
                    MPI_STATUSES_IGNORE);

            MPI_Barrier(MPI_COMM_WORLD);
        }
        if(rank){
            stop = MPI_Wtime();
            msec = (stop - start) / repeat * 1000;
            printf("%10d bytes, %10.5f msec/buffer, avg over %5d packets\n",
                count, msec, repeat);
        }
        free(buf);
    }
}


int main(int argc, char const *argv[]) {
    int rank, n_ranks, count;

    MPI_Init(&argc, (char***) &argv);

    assert(MPI_SUCCESS == MPI_Comm_size(MPI_COMM_WORLD, &n_ranks));
    assert(MPI_SUCCESS == MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    assert(n_ranks == 2);

    latency(rank, LATENCY_REPEAT);
    printf("\n");
    bandwidth(rank, BANDWIDTH_REPEAT, MAG);

    MPI_Finalize();
    return 0;
}
