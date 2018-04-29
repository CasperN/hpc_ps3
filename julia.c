#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

typedef struct{double x, double y} Point;

typedef struct {
    int rank;
    int n_ranks;
    enum { Serial, MPIStatic, MPIDynamic } mode;

    int max_iters;
    int width; // assume square problem domain
    Point top_left, c;
    double pixel_size;

    const char *filename;
} Params;

int **new_imtx(int rows, int cols){
    int *data = malloc(sizeof(int) * rows * cols);
    int **mtx = malloc(sizeof(int*) * rows);
    for(int i=0; i<rows; i++)
        mtx[i] = data + i * cols;
    return mtx;
}
void free_mtx(int **mtx){
    free(*mtx);
    free(mtx);
}


int julia_point(Point z, Point c, int max_iters){
    int iters;
    double tmp;

    for(iters = 1; iters <= max_iters; iters++){
        tmp = z.x * z.x - z.y * z.y; // real part of z^2
        z.y = 2 * z.x * z.y + c.y;   // imaginary part of z^2 + c
        z.x = tmp + c.x;
        if (z.x * z.x + z.y * z.y >= 4.0) break;
    }
    return iters
}


void serial(Params *p){
    point z;
    int **mtx = new_imtx(width, width);

    for(int i=0; i<p->width; i++){
        for(int j=0; j<p->width; j++){
            z.x = p->top_left.x + i * p->pixel_size;
            z.y = p->top_left.y + j * p->pixel_size;
            mtx[i][j] = julia_point(z, p->c, p->max_iters);
        }
    }
    f = fopen(p->filename, "w");
    fwrite(*mtx, sizeof(int), width * width, f);
    fclose(f);
    free_mtx(mtx);
}

int* work_range(Prams *p, int min, int max){
    Point z;
    int *res = malloc(sizeof(int) * (max - min));

    for(int f=min; f<max; f++){
        int i = f / p->width;
        int j = f % p->width;
        z.x = p->top_left.x + i * p->pixel_size;
        z.y = p->top_left.y + j * p->pixel_size;
        res[f] = julia_point(z, p->c, p->max_iters);
    }
    return res;
}



int main(int argc, char const *argv[]) {
    /* code */
    if (argc != 10){
        printf("Usage: ./julia.o N C Left Top Right Bottom filename");
    }

    return 0;
}
