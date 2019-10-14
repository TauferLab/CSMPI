#include "runtime.hpp"
#include "callstack.hpp"

#include <iostream>
#include <cstdlib>
#include <cassert>

Runtime* runtime_ptr;

int dummy_func_C(int x)
{
  runtime_ptr->trace_callstack("dummy_func_C"); 
  runtime_ptr->print();
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
  Runtime csmpi_runtime( config );
  runtime_ptr = &csmpi_runtime;
  int input = 1;
  int res = dummy_func_A( input );
  assert( res == 3 );
  return 0;
}
