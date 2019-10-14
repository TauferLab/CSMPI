#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>

void receiver_top_level_function();
void receiver_intermediate_function();
void sender_top_level_function();
void sender_intermediate_function();

void receiver_top_level_function()
{
  receiver_intermediate_function();
}

void receiver_intermediate_function()
{
  int mpi_rc;
  int recv_buffer;
  mpi_rc = MPI_Recv( &recv_buffer,
                     1,
                     MPI_INT,
                     1,
                     0,
                     MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE );
  //std::cout << "Rank 0: " << " received: " << recv_buffer << std::endl;
  //printf( "Rank 0: received: %d\n", recv_buffer );
}

void sender_top_level_function()
{
  sender_intermediate_function();
}

void sender_intermediate_function()
{
  int mpi_rc;
  int send_buffer = 17;
  mpi_rc = MPI_Send( &send_buffer,
                     1,
                     MPI_INT,
                     0,
                     0,
                     MPI_COMM_WORLD );
}

int main( int argc, char** argv ) 
{
  int mpi_rc, rank, n_procs;
  mpi_rc = MPI_Init( &argc, &argv );
  mpi_rc = MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  mpi_rc = MPI_Comm_size( MPI_COMM_WORLD, &n_procs );
  
  //if ( n_procs != 2 ) {
  //  throw std::runtime_error("This test must run on 2 MPI processes");
  //}
  //if ( argc != 2 ) {
  //  throw std::runtime_error("Provide a number of iterations to run");
  //}

  int n_iters = atoi( argv[1] );
  int i;
  for ( i = 0; i < n_iters; ++i ) {
    if ( rank == 0 ) {
      receiver_top_level_function();
    } else {
      sender_top_level_function();
    } 
  }

  mpi_rc = MPI_Finalize(); 
  return 0;
}
