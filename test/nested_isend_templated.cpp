#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

void funcB() {
  int err;
  int buffer = 0;
  MPI_Request req;
  err = MPI_Isend((void*)&buffer, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &req);
}

void funcA() {
  funcB();
}

namespace ns { 

template <typename T, typename U>
void foo(T t, U u) {
  funcA();
}

}  // namespace ns

template <typename T>
struct Klass {
  T t;
  void bar() {
    ns::foo(t, true);
  }
};

int main(int argc, char** argv) {
  int mpi_ret;
  mpi_ret = MPI_Init(&argc, &argv);
  int rank;
  mpi_ret = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  Klass<double> k;
  k.bar();

  mpi_ret = MPI_Finalize();
  return 0;
}

