#include "runtime.hpp"

#include <mpi.h>

#include <iostream>
#include <cstdlib>

Runtime* runtime_ptr;

int main(int argc, char** argv) 
{
  int mpi_rc;
  mpi_rc = MPI_Init( &argc, &argv );
  
  runtime_ptr = csmpi_init( runtime_ptr );
  csmpi_finalize( runtime_ptr );
  
  mpi_rc = MPI_Finalize(); 

  return 0;
}
