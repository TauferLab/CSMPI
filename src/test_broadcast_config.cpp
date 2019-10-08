#include "configuration.hpp"
#include "mpi.h"
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) 
{
  int mpi_rc, rank, n_procs;
  mpi_rc = MPI_Init( &argc, &argv );
  mpi_rc = MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  mpi_rc = MPI_Comm_size( MPI_COMM_WORLD, &n_procs );
  if ( n_procs < 2 ) {
    if ( rank == 0 ) {
      std::cout << "This test must run on at least 2 MPI processes" 
                << std::endl;   
    }
    std::exit(1);
  }

  Configuration config;
  // Root ingests configuration and broadcasts
  if ( rank == 0 ) {
    Configuration ingested_config(argv[1]);
    config = ingested_config;
    broadcast_config( config );
  } 
  // Other processes receive
  else {
    broadcast_config( config );
  }

  for ( int curr_rank = 0; curr_rank < n_procs; ++curr_rank ) {
    if ( rank == curr_rank ) {
      std::cout << "Rank: " << curr_rank << " config:" << std::endl;
      config.print();
    }
    mpi_rc = MPI_Barrier(MPI_COMM_WORLD);
  }

  mpi_rc = MPI_Finalize();
  return 0;
}

