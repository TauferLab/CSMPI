#include <iostream>
#include <vector>
#include <cstdio>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include "callstack.hpp"

void Callstack::add_frame( unw_word_t frame )
{
  frames.push_back(frame);
}
  
std::vector<unw_word_t> Callstack::get_frames() const
{
  return this->frames;
}

Callstack& Callstack::operator=( const Callstack& rhs )
{
  if ( &rhs == this ) {
    return *this;
  }
  this->frames = rhs.get_frames();
  return *this; 
}

void Callstack::print() const
{
  std::cout << "Callstack: " << std::endl;
  for ( auto frame : frames ) {
    printf( "0x%lx:\n", frame );
  }
}
