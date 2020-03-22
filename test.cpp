#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>


int main( int argc, char *argv[] )
{
    MPI_Comm parentcomm, intercomm;

    MPI_Init( &argc, &argv );
    MPI_Comm_get_parent( &parentcomm );

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);


    if (parentcomm == MPI_COMM_NULL)
    {
        printf("Parent process from processor %s, rank %d out of %d processors\n", processor_name, world_rank, world_size);
    }
    else
    {
        printf("Spawned process from processor %s, rank %d out of %d processors\n", processor_name, world_rank, world_size);
    }

    MPI_Finalize();
    return 0;
}
