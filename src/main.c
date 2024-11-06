#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include "sensors.h"
#include "constants.h"
#include "base.h"
#include "structures.h"

// alert raised_alert[NNEIGHBORS+1];

int main(int argc, char* argv[]) {
    int rank, size, provided, reorder=0, caas=0;
    int dims[NDIMS] = {0}, periods[NDIMS]={0};
    int max_iterations;
    MPI_Comm comm, cart_comm;

    // MPI_Init(&argc, &argv);
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == BASE){
        if (argc == 4 || argc == 5) {
            dims[0] = atoi(argv[1]);
            dims[1] = atoi(argv[2]);
            max_iterations = atoi(argv[3]);
            printf("Using max iterations: %d\n", max_iterations);
            if (dims[0] * dims[1] != size-1) { // excludes the root which is the base station
                printf("ERROR: given dimensions %dx%d!=%d\n", dims[0], dims[1], size-1);
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);

                return 0;
            }
            if (argc == 5) {
                if (strcmp(argv[4], "-caas") == 0) {
                    printf("Detected caas\n");
                    caas=1;
                } else {
                    printf("Invalid 4th argument given!\n");
                    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
                    return 0;
                }
            }
            
        } else {
            printf("Letting MPI automatically assign the dims\n");
        }

    }
    MPI_Dims_create(size-1, NDIMS, dims);
    int sentinel;
    if(rank == BASE && !caas){
        printf("Enter sentinel value: "); 
        fflush(stdout);
        scanf("%d", &sentinel);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank==BASE){

        printf("dims: [%d, %d]\n", dims[0], dims[1]);
    }
    MPI_Comm_split(MPI_COMM_WORLD, rank==BASE, 0, &comm);
    if (rank==BASE) {
        base_station_main(MPI_COMM_WORLD, comm, dims, size, max_iterations, sentinel, caas);
    
    } else {
        MPI_Cart_create(comm, NDIMS, dims, periods, reorder, &cart_comm);
        sensor_main(cart_comm, dims);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Comm_free(&comm);
    if (rank != BASE) {
        MPI_Comm_free(&cart_comm);
    }

    MPI_Finalize();
    return 0;
}

