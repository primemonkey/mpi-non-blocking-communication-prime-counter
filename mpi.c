#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>
#include <stdbool.h>
#include "numgen.c"

#define DATA 0
#define RESULT 1
#define FINISH 2

int main(int argc,char **argv) 
{

  Args ins__args;
  parseArgs(&ins__args, &argc, argv);

  long inputArgument = ins__args.arg; 

  struct timeval ins__tstart, ins__tstop;

  int myrank,nproc;
  unsigned long int *numbers;
  MPI_Request request;

  MPI_Init(&argc,&argv);

  MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
  MPI_Comm_size(MPI_COMM_WORLD,&nproc);

  if(!myrank)
  {
    gettimeofday(&ins__tstart, NULL);
    numbers = (unsigned long int*)malloc(inputArgument * sizeof(unsigned long int));
    numgen(inputArgument, numbers);
  }

  

  int slaveNumber = nproc - 1;
  int batchSize = 100; 

  if (myrank == 0)
  {
    int primeCounter = 0;

    for (int i = 0; i < inputArgument; i += batchSize)
    {
      int destination_rank = i / batchSize % (nproc - 1) + 1;

      MPI_Isend(&numbers[i], batchSize, MPI_UNSIGNED_LONG, destination_rank, DATA, MPI_COMM_WORLD, &request);
      MPI_Wait(&request, MPI_STATUS_IGNORE);
    }

    for (int i = 1; i < nproc; i++)
    {
      MPI_Isend(NULL, 0, MPI_UNSIGNED_LONG, i, FINISH, MPI_COMM_WORLD, &request);
      MPI_Wait(&request, MPI_STATUS_IGNORE);
    }

    for (int i = 1; i < nproc; i++)
    {
      int slavePrimes;
      MPI_Irecv(&slavePrimes, 1, MPI_INT, i, RESULT, MPI_COMM_WORLD, &request);
      MPI_Wait(&request, MPI_STATUS_IGNORE);
      primeCounter += slavePrimes;
    }
    
    printf("Amount of prime numbers: %d", primeCounter);
  }

  else
  {
    bool flag = true;
    int counter = 0;
    unsigned long int received_numbers[batchSize]; 

    MPI_Status status;

    do
    {
      MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

      if (status.MPI_TAG == DATA)
      {  
        MPI_Irecv(received_numbers, batchSize, MPI_UNSIGNED_LONG, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, MPI_STATUS_IGNORE);

        for (int i = 0; i < batchSize; ++i) 
        {
          flag = true;
          for (unsigned long j = 2; j * j <= received_numbers[i]; ++j)
          {
            if (received_numbers[i] % j == 0)
            {
              flag = false;
              break;
            }
          }

          if (flag)
          {
            counter++;
          }
        }
      }

    } while (status.MPI_TAG != FINISH);

    MPI_Isend(&counter, 1, MPI_INT, 0, RESULT, MPI_COMM_WORLD, &request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);
  }

  // synchronize/finalize computations

  if (!myrank)
  {
    gettimeofday(&ins__tstop, NULL);
    ins__printtime(&ins__tstart, &ins__tstop, ins__args.marker);
  }
  
  MPI_Finalize();

}
