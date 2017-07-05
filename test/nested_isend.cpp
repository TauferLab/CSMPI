#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

//#include <caliper/cali.h>

void funcB() {
  int err;
  int buffer = 0;
  MPI_Request req;
  err = MPI_Isend((void*)&buffer, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &req);
}

void funcA() {
  funcB();
}

int main(int argc, char** argv) {
  //CALI_CXX_MARK_FUNCTION;
  int mpi_ret;
  mpi_ret = MPI_Init(&argc, &argv);
  int rank;
  mpi_ret = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  funcA();
  mpi_ret = MPI_Finalize();
} 
