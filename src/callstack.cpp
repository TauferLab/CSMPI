#include <iostream>
#include <vector>
#include <cstdio>

#define UNW_LOCAL_ONLY
#ifdef DET_LIBUNWIND
  #include <libunwind.h>
#endif

#include "callstack.hpp"

namespace detail {
#ifdef DET_LIBUNWIND
  using csmpi_word_t = unw_word_t;
#else
  using csmpi_word_t = uint64_t;
#endif
}

bool Callstack::operator==(const Callstack& rhs) const
{
  auto lhs_n_frames = this->frames.size();
  auto rhs_frames = rhs.get_frames();
  auto rhs_n_frames = rhs_frames.size();
  if ( lhs_n_frames != rhs_n_frames ) {
    return false;
  } else {
    for (int i=0; i<lhs_n_frames; ++i) {
      if (this->frames[i] != rhs_frames[i]) {
        return false;
      }
    }
  }
  return true;
}

void Callstack::add_frame( detail::csmpi_word_t frame )
{
  frames.push_back(frame);
}
  
std::vector<detail::csmpi_word_t> Callstack::get_frames() const
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
