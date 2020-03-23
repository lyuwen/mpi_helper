#include "mpi.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#define NUM_SPAWNS 2

#if SIZE_MAX == UCHAR_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_CHAR
#elif SIZE_MAX == USHRT_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_SHORT
#elif SIZE_MAX == UINT_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED
#elif SIZE_MAX == ULONG_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_LONG
#elif SIZE_MAX == ULLONG_MAX
   #define my_MPI_SIZE_T MPI_UNSIGNED_LONG_LONG
#else
   #error "what is happening here?"
#endif


void trim(char * s) {
    char * p = s;
    int l = strlen(p);

    while(isspace(p[l - 1])) p[--l] = 0;
    while(* p && isspace(* p)) ++p, --l;

    memmove(s, p, l + 1);
}   


char** str_split(char* a_str, const char a_delim, size_t &count)
{
    char** result    = 0;
    // size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    count = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    // count++;

    result = (char **)malloc(sizeof(char*) * count + 1);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count);
        *(result + idx + 1) = 0;
    }

    return result;
}


int main( int argc, char *argv[] )
{
    char cwd[PATH_MAX];
    char fullpath[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
      perror("Error getting current working directory.");
      return 1;
    }

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

    // parse option
    int verbose = 0;
    int opt;
    int nonoptarg = 0;
    char *input_file = NULL;
    //
    while (optind < argc) {
      if ((opt = getopt (argc, argv, "hv")) != -1)
      {
        switch (opt)
          {
          case 'h':
            printf(
                "%s\n"
                "\n"
                "Usage: %s OPTIONS [CONFIG_FILE]\n"
                "\n"
                " -v    Increase verbosity.\n"
                " -h    Print this help message.\n", argv[0], argv[0]);
            return 0;
          case 'v':
            verbose = 1;
            break;
          default:
            abort ();
          } 
      } else {
            if (nonoptarg== 0)
            {
              input_file = argv[optind];
            }
            nonoptarg++;
            optind++;
      }
    }

    // if (parentcomm == MPI_COMM_NULL)
    // {
    //     if (world_rank == 0){
    //       printf("- argc %d\n", argc);
    //       for(int i=0; i<argc; i++){
    //         printf("- arg no. %d: %s\n", i, argv[i]);
    //       }
    //     }
    //     realpath("spawn_example", fullpath);
    //     // [> Create 2 more processes - this example must be called spawn_example.exe for this to work. <]
    //     MPI_Comm_spawn( argv[0], MPI_ARGV_NULL, np, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, errcodes );
    //     // [>* MPI_Comm_spawn( fullpath, MPI_ARGV_NULL, np, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, errcodes ); <]
    //     printf("Parent process from processor %s, rank %d out of %d processors\n", processor_name, world_rank, world_size);
    // }
    // else
    // {
    //     printf("Spawned process from processor %s, rank %d out of %d processors\n", processor_name, world_rank, world_size);
    // }

    // Read file in rank 0 and then broadcast
    int njobs, *ncores; // Number of jobs and Ncores for each job
    char **execs, **wdirs; // Executables and working directories
    char **argvs; // sub-command args
    int *argvs_len; // sub-command args
    const size_t NBUFF = 4096;
    char *temp;
    if ((parentcomm == MPI_COMM_NULL) && (world_rank == 0))
    {
      if (!input_file) {
        perror("Missing input file.\n");
        return 1;
      } else {
        FILE * fp;
        fp = fopen (input_file, "r");
        fscanf(fp, "%d", &njobs);
        ncores = (int *)malloc(njobs * sizeof(int));
        execs = (char **)malloc(njobs * sizeof(char *));
        wdirs = (char **)malloc(njobs * sizeof(char *));
        argvs = (char **)malloc(njobs * sizeof(char *));
        argvs_len = (int *)malloc(njobs * sizeof(int));
        for (int i = 0; i < njobs; i++)
        {
          execs[i] = (char *)malloc(PATH_MAX * sizeof(char));
          wdirs[i] = (char *)malloc(PATH_MAX * sizeof(char));
          fscanf(fp, "%d %s %s", &ncores[i], execs[i], wdirs[i]);
          // Extra arguments
          temp = (char *)malloc(NBUFF * sizeof(char));
          fgets(temp, 4096, fp);
          trim(temp);
          if (strlen(temp)) {
            argvs[i] = temp;
            argvs_len[i] = strlen(argvs[i]);
          } else {
            argvs[i] = NULL;
            argvs_len[i] = 0;
          }
          if (argvs_len[i])
          {
            printf("Job No.%d: Ncores=%d exe=%s wdir=%s args=(%s) args_len=%d\n", 
                i, ncores[i], execs[i], wdirs[i], argvs[i], argvs_len[i]);
          } else {
            printf("Job No.%d: Ncores=%d exe=%s wdir=%s args_len=%d\n", i, ncores[i], execs[i], wdirs[i], argvs_len[i]);
          }
        }
        fclose(fp);
      }
    }

    // Broadcast data accross
    // MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(&njobs, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if ((parentcomm == MPI_COMM_NULL) && (world_rank != 0))
    {
      ncores = (int *)malloc(njobs * sizeof(int));
      execs = (char **)malloc(njobs * sizeof(char *));
      wdirs = (char **)malloc(njobs * sizeof(char *));
      for (int i = 0; i < njobs; i++)
      {
        execs[i] = (char *)malloc(PATH_MAX * sizeof(char));
        wdirs[i] = (char *)malloc(PATH_MAX * sizeof(char));
      }
      argvs_len = (int *)malloc(njobs * sizeof(int));
      argvs = (char **)malloc(njobs * sizeof(char *));
    }
    MPI_Bcast(ncores, njobs, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(argvs_len, njobs, MPI_INT, 0, MPI_COMM_WORLD);
    for (int i = 0; i < njobs; i++)
    {
      MPI_Bcast(execs[i], PATH_MAX, MPI_CHAR, 0, MPI_COMM_WORLD);
      MPI_Bcast(wdirs[i], PATH_MAX, MPI_CHAR, 0, MPI_COMM_WORLD);
      if (argvs_len[i]) {
        if ((parentcomm == MPI_COMM_NULL) && (world_rank != 0))
        {
          argvs[i] = (char *)malloc(NBUFF * sizeof(char));
          // argvs[i] = (char *)malloc(argvs_len[i] * sizeof(char));
        }
        MPI_Bcast(argvs[i], NBUFF, MPI_CHAR, 0, MPI_COMM_WORLD);
      }
    }
    // MPI_Barrier(MPI_COMM_WORLD);
    // Show the result of the broadcast
    // if ((parentcomm == MPI_COMM_NULL) && (world_rank != 0))
    if ((verbose) && (parentcomm == MPI_COMM_NULL))
    {
      printf("[Rank %d] njobs %d\n", world_rank, njobs);
      for (int i = 0; i < njobs; i++)
      {
        if (argvs_len[i]) {
          printf("[Rank %d] Job No.%d: Ncores=%d exe=%s wdir=%s args=(%s)\n",
              world_rank, i, ncores[i], execs[i], wdirs[i], argvs[i]);
        } else {
          printf("[Rank %d] Job No.%d: Ncores=%d exe=%s wdir=%s args=(None)\n", 
              world_rank, i, ncores[i], execs[i], wdirs[i]);
        }
      }
    }
    // Run job
    int jobs_per_rank;
    int errcodes[njobs];
    size_t argcs;
    if (njobs % world_size != 0){
      jobs_per_rank = njobs / world_size + 1;
    } else {
      jobs_per_rank = njobs / world_size;
    }
    if ((parentcomm == MPI_COMM_NULL) && (world_rank == 0))
    {
      printf("We have %d ranks, and %d jobs. Distributing %d jobs per rank.\n", world_size, njobs, jobs_per_rank);
    }
    fflush(stdout);
    MPI_Barrier(MPI_COMM_WORLD);
    for (int i = 0; i < jobs_per_rank; i++){
      int jobid = i * world_size + world_rank;
      if (jobid < njobs)
      {
        printf("[Rank %d] executing job %d.\n", world_rank, jobid);
        // Split argv
        char **mpiargv;
        MPI_Info info;
        MPI_Info_create( &info );
        MPI_Info_set( info, "wdir", wdirs[jobid] );
        // if (argvs_len[jobid])
        // {
        //   MPI_Comm_spawn( execs[jobid], str_split(argvs[jobid], ' ', argcs), ncores[jobid], info, 0, MPI_COMM_SELF, &intercomm, errcodes );
        // } else {
          MPI_Comm_spawn( execs[jobid], MPI_ARGV_NULL, ncores[jobid], info, 0, MPI_COMM_SELF, &intercomm, errcodes );
        // }
      } else {
        printf("[Rank %d] Job %d does not exist, idling.\n", world_rank, jobid);
      }
    }
    // MPI_Barrier(MPI_COMM_WORLD);
    //
    fflush(stdout);
    if (parentcomm == MPI_COMM_NULL)
    {
      free(ncores);
      for (int i = 0; i < njobs; i++)
      {
        free(execs[i]);
        free(wdirs[i]);
        // free(argvs[i]);
      }
      free(execs);
      free(wdirs);
      free(argvs);
    }
    MPI_Finalize();
    return 0;
}
