#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>

#define REPEAT 25
#define MAG 3


void *ping_pong(int rank, int repeat, int magnitudes){

    int count, *send, *recv, tag;
    double start, stop, diff;

    tag = 1;

    for(int m=1; m <= magnitudes; m++){
        count = 1 << (10 * m);
        send = malloc(count);
        recv = malloc(count);

        if(rank) start = MPI_Wtime();

        for(int i=0; i<repeat; i++){
            MPI_Sendrecv(send, count, MPI_CHAR, 1 - rank, tag,
                         recv, count, MPI_CHAR, 1 - rank, tag,
                         MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
        }
        if(rank){
            stop = MPI_Wtime();
            diff = (stop - start) / repeat * 1000;
            printf("%10d bytes sendrecv %10.5f msec / packet over %d packets\n",
                count, diff, repeat);
        }
        free(send);
        free(recv);
    }
}


int main(int argc, char const *argv[]) {
    int rank, n_ranks, count;

    MPI_Init(&argc, (char***) &argv);

    assert(MPI_SUCCESS == MPI_Comm_size(MPI_COMM_WORLD, &n_ranks));
    assert(MPI_SUCCESS == MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    assert(n_ranks == 2);

    ping_pong(rank, REPEAT, MAG);
    MPI_Finalize();
    return 0;
}
