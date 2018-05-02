#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>

#define REPEAT 50000


void ping_pong(int rank, int repeat){

    int count, send, recv, tag;

    send = recv = rank; // arbitrary
    count = tag = 1;

    for(int i=0; i<repeat; i++){
        MPI_Sendrecv(&send, count, MPI_INT, 1 - rank, tag,
                     &recv, count, MPI_INT, 1 - rank, tag,
                     MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
    }
}



int main(int argc, char const *argv[]) {
    int rank, n_ranks;
    double start, stop, total;
    MPI_Init(&argc, (char***) &argv);

    assert(MPI_SUCCESS == MPI_Comm_size(MPI_COMM_WORLD, &n_ranks));
    assert(MPI_SUCCESS == MPI_Comm_rank(MPI_COMM_WORLD, &rank));
    assert(n_ranks == 2);

    start = MPI_Wtime();
    ping_pong(rank, REPEAT);
    stop = MPI_Wtime();

    total = stop - start;

    if(rank == 0){
        printf("Runtime: %lf seconds\n", total);
        printf("Average: %lf usec / packet\n",total / REPEAT * 1000000);
    }

    MPI_Finalize();
    return 0;
}
