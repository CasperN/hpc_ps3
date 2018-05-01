#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <assert.h>
#include <omp.h>

#define LEFT  -1.5
#define RIGHT 1.5
#define TOP 1.0
#define BOTTOM -1.0
#define MAX_ITERS 1000;
#define CHUNK_SIZE 10000;

typedef struct{double x; double y;} Point;

typedef struct {
    enum { Serial, MPIStatic, MPIDynamic } mode;
    int rank, n_ranks, chunk_size;

    int max_iters;
    int width; // assume square problem domain
    Point top_left, c;
    Point pixel_size;

    const char *filename;
} Params;

// -------------------------------- Julia Set ----------------------------------

int julia_point(Point z, Point c, int max_iters){
    int iters;
    double tmp;

    for(iters=0; iters <= max_iters; iters++){
        tmp = z.x * z.x - z.y * z.y; // real part of z^2
        z.y = 2 * z.x * z.y + c.y;   // imaginary part of z^2 + c
        z.x = tmp + c.x;
        if (z.x * z.x + z.y * z.y >= 4.0) break;
    }
    return iters;
}

int* work_range(Params *p, int min, int max){
    Point z;
    int i,j, *res = malloc(sizeof(int) * (max - min));

    for(int f=min; f<max; f++){
        i = f / p->width;
        j = f % p->width;
        z.x = p->top_left.x + i * p->pixel_size.x;
        z.y = p->top_left.y + j * p->pixel_size.y;
        // print_point(&z);
        res[f-min] = julia_point(z, p->c, p->max_iters);
    }
    return res;
}

// ----------------------- Serial and static modes -----------------------------

void serial(Params *p){
    Point z;
    FILE *f;
    int *mtx = work_range(p, 0, p->width * p->width);

    f = fopen(p->filename, "w");
    fwrite(mtx, sizeof(int), p->width * p->width, f);
    fclose(f);
    free(mtx);
}


void static_(Params *p){
    int low, high, size, *res;
    MPI_File f;
    MPI_File_open(MPI_COMM_WORLD, p->filename,
        MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &f);

    size = p->width * p->width / p->n_ranks;
    low = size * p->rank;
    high = size * (p->rank + 1);
    // Remainder
    if (p->rank + 1 == p->n_ranks)
        high += (p->width * p->width) % size;

    res = work_range(p, low, high);

    MPI_File_write_at_all(f, low * sizeof(MPI_INT), res, high - low, MPI_INT,
        MPI_STATUSES_IGNORE);

        // MPI_File_write_at_all(f, msg[0] * sizeof(int), res, MPI_INT,
        //     msg[1] - msg[0], MPI_STATUSES_IGNORE);

    MPI_File_close(&f);
    free(res);
}

// ----------------------------- Dynamic mode ----------------------------------

void boss(Params *p){
    int msg[2], loc, len;
    MPI_Request *requests;
    MPI_Status status;
    MPI_File f;
    // Boss will open file but do nothing because all process must open
    MPI_File_open(MPI_COMM_WORLD, p->filename,
        MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &f);

    requests = malloc(sizeof(MPI_Request) * p->n_ranks);

    loc = 0;
    len = p->width * p->width;

    for(int i=1; i < p->n_ranks; i++){
        msg[0] = loc;
        loc += p->chunk_size;
        loc = loc > len ? len : loc;
        msg[1] = loc;
        MPI_Isend(msg, 2, MPI_INT, i, 0, MPI_COMM_WORLD, requests + i);
    }

    while (loc < len) {
        // Workers send a signal they're done. Reply with more work
        MPI_Recv(msg, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
            &status);

        msg[0] = loc;
        loc += p->chunk_size;
        loc = loc > len ? len : loc;
        msg[1] = loc;
        MPI_Send(msg, 2, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
    }

    // Send everyone out of range workload to signal quiting
    for(int i=1; i < p->n_ranks; i++){
        msg[0] = len;
        msg[1] = len;
        MPI_Isend(msg, 2, MPI_INT, i, 0, MPI_COMM_WORLD, requests + i);
    }
    //  Make sure they're all sent
    MPI_Waitall(p->n_ranks - 1, requests + 1, MPI_STATUSES_IGNORE);
    free(requests);
    MPI_File_close(&f);
}

void worker(Params *p){
    int msg[2], *res;
    MPI_File f;
    MPI_File_open(MPI_COMM_WORLD, p->filename,
        MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &f);

    while(1){
        MPI_Recv(msg, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
        // printf("Worker %d recv work[%d,%d)\n", p->rank, msg[0], msg[1]);


        if (msg[0] == p->width * p->width){
            MPI_File_close(&f);
            return; // No more work to do
        }

        res = work_range(p, msg[0], msg[1]);

        MPI_File_write_at(f, msg[0] * sizeof(int), res, msg[1] - msg[0],
            MPI_INT, MPI_STATUSES_IGNORE);

        free(res);
        // Ask for more work
        // printf("Worker %d Asking for more work\n", p->rank);
        MPI_Send(msg, 2, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
}

// -----------------------------------------------------------------------------

int main(int argc, char const *argv[]) {
    int N;
    Params p;
    double start;

    if (argc != 6 && argc != 5){
        printf("Usage: ./julia.o width Cx Cy MPI_Mode filename\n\n");
        exit(0);
    }
    p.width = atoi(argv[1]);
    p.c.x   = atof(argv[2]);
    p.c.y   = atof(argv[3]);

    if (strcmp(argv[4], "static") == 0) {
        p.mode = MPIStatic;
    } else if (strcmp(argv[4], "dynamic") == 0) {
        p.mode = MPIDynamic;
    } else if (strcmp(argv[4], "serial") == 0) {
        p.mode = Serial;
    } else {
        printf("Invalid mode");
        exit(1);
    }

    if(argc == 6) p.filename = argv[5];

    assert(p.width > 0);

    p.top_left.x = LEFT;
    p.top_left.y = BOTTOM;
    p.pixel_size.x = (RIGHT - LEFT) / p.width;
    p.pixel_size.y = (TOP - BOTTOM) / p.width;
    p.max_iters = MAX_ITERS;

    start = omp_get_wtime();

    if (p.mode == Serial) {
        p.rank = 0;
        p.n_ranks = 1;
        serial(&p);

    } else {
        MPI_Init(&argc, (char ***) &argv);
        assert(MPI_SUCCESS == MPI_Comm_size(MPI_COMM_WORLD, &p.n_ranks));
        assert(p.n_ranks > 1);
        assert(MPI_SUCCESS == MPI_Comm_rank(MPI_COMM_WORLD, &p.rank));

        if (p.mode == MPIDynamic){
            p.chunk_size = CHUNK_SIZE;
            if (p.rank == 0){
                boss(&p);
            } else {
                worker(&p);
            }
        } else {
            static_(&p);
        }

        MPI_Finalize();
    }

    if(p.rank == 0)
        printf("Runtime: %lf seconds\n", omp_get_wtime() - start);

    return 0;
}
