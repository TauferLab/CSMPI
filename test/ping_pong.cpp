#include <mpi.h>

#include <cassert>
#include <cstdlib>
#include <iostream>


void recv_top_level();
void recv_helper();
void recv_impl();
void send_top_level();
void send_helper();
void send_impl();

void recv_top_level() {
  recv_helper();
}

void recv_helper() {
  recv_impl();
}

void recv_impl() {
  int mpi_rc;
  int recv_buffer;
  MPI_Status status;
  mpi_rc = MPI_Recv( &recv_buffer, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status );
  //std::cout << "Received: " << recv_buffer << " from: " << status.MPI_SOURCE << std::endl;
}

void send_top_level() {
  send_helper();
}

void send_helper() {
  send_impl();
}

void send_impl() {
  int mpi_rc;
  int send_buffer = 17;
  mpi_rc = MPI_Send( &send_buffer, 1, MPI_INT, 0, 0, MPI_COMM_WORLD );
}

int main(int argc, char** argv) 
{
  assert(argc == 2);
  int n_iters = std::atoi(argv[1]);
  int mpi_rc;
  mpi_rc = MPI_Init(&argc, &argv);
  int rank, comm_size;
  mpi_rc = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  mpi_rc = MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  assert(comm_size == 2);  
  for ( int i=0; i<n_iters; ++i ) {
  if ( rank == 0 ) {
      recv_top_level();
    } else {
      send_top_level();
    }
  }
  mpi_rc = MPI_Finalize();
  return EXIT_SUCCESS;
}
