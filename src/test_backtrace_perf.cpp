#include "runtime.hpp"
#include "callstack.hpp"

#include <iostream>
#include <cstdlib>
#include <chrono>
#include <vector>
#include <cassert>

Runtime* runtime_ptr;
static int n_invocations;

int dummy_func_C(int x)
{
  std::vector<Callstack> callstacks;
  auto start = std::chrono::steady_clock::now();
  for ( size_t i = 0; i < n_invocations; ++i ) {
    runtime_ptr->trace_callstack( "dummy_func_C" );
  }
  auto end = std::chrono::steady_clock::now();
  auto elapsed = end - start;
  const unsigned int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  std::cout << "Tracing " << n_invocations << " call-stacks took: " << milliseconds << " ms" << std::endl;
  return x + 1; 
}

int dummy_func_B(int x)
{
  return x * dummy_func_C(x);
}

int dummy_func_A(int x)
{
  return x + dummy_func_B(x);
}


int main(int argc, char** argv) 
{
  Configuration config( argv[1] ); 
  n_invocations = std::atoi( argv[2] );
  Runtime csmpi_runtime( config );
  runtime_ptr = &csmpi_runtime;
  int input = 1;
  int res = dummy_func_A( input );
  assert( res == 3 );
  return 0;
}
