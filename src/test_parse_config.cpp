#include "configuration.hpp"

int main(int argc, char** argv) 
{
  Configuration config(argv[1]);
  config.print();
  return 0;
}


