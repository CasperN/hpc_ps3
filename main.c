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

    MPI_File_close(&f);
    free(res);
}

// ----------------------------- Dynamic mode ----------------------------------

void next_chunk(int *index, int *range, int chunk_size, int maximum){
    int ix = *index;
    range[0] = ix;
    ix += chunk_size;
    ix = ix > maximum ? maximum : ix;
    range[1] = ix;
    *index = ix;
}

void boss(Params *p){
    int range[2], index, maximum;
    MPI_Request *requests;
    MPI_Status status;
    MPI_File f;
    // Boss will open file but do nothing with it (MPI_File_open is blocking)
    MPI_File_open(MPI_COMM_WORLD, p->filename,
        MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &f);

    requests = malloc(sizeof(MPI_Request) * p->n_ranks);
    maximum = p->width * p->width;
    index = 0;
    // Initialize workers' tasks
    for(int i=1; i < p->n_ranks; i++){
        next_chunk(&index, range, p->chunk_size, maximum);
        MPI_Isend(range, 2, MPI_INT, i, 0, MPI_COMM_WORLD, requests + i);
    }
    // Feed workers more tasks when they're done
    while (index < maximum) {
        MPI_Recv(range, 2, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        next_chunk(&index, range, p->chunk_size, maximum);
        MPI_Send(range, 2, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
    }
    // Out of range workload to signals quiting
    for(int i=1; i < p->n_ranks; i++){
        range[0] = range[1] = maximum;
        MPI_Isend(range, 2, MPI_INT, i, 0, MPI_COMM_WORLD, requests + i);
    }
    MPI_Waitall(p->n_ranks - 1, requests + 1, MPI_STATUSES_IGNORE);
    free(requests);
    MPI_File_close(&f);
}

void worker(Params *p){
    int range[2], *res;
    MPI_File f;
    MPI_File_open(MPI_COMM_WORLD, p->filename,
        MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &f);

    while(1){
        MPI_Recv(range, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);

        if (range[0] == p->width * p->width){
            MPI_File_close(&f);
            return; // No more work to do
        }

        res = work_range(p, range[0], range[1]);

        MPI_File_write_at(f, range[0] * sizeof(int), res, range[1] - range[0],
            MPI_INT, MPI_STATUSES_IGNORE);

        free(res);
        MPI_Send(range, 2, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
}

// -----------------------------------------------------------------------------

int main(int argc, char const *argv[]) {
    int N;
    Params p;
    double start;

    if (argc != 6 && argc != 7){
        printf("Usage: ./julia.o width Cx Cy MPI_Mode filename [chunk]\n\n");
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

    p.filename = argv[5];
    p.chunk_size = argc == 7 ? atoi(argv[6]) : CHUNK_SIZE

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
    fflush(stdout);

    return 0;
}
