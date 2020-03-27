#ifndef CSMPI_CALLSTACK_H
#define CSMPI_CALLSTACK_H

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include "boost/functional/hash.hpp"

class Callstack
{
public:
  Callstack() {}
  Callstack& operator=( const Callstack& rhs );
  void add_frame( unw_word_t frame );
  void print() const;
  std::vector<unw_word_t> get_frames() const;
  bool operator==(const Callstack& c) const;
private:
  std::vector<unw_word_t> frames;
};

struct CallstackHash 
{
  std::size_t operator() (Callstack callstack) const
  {
    std::size_t hash = 0;
    for ( auto frame : callstack.get_frames() ) {
      boost::hash_combine( hash, boost::hash_value( frame ) );
    }
    return hash;
  }
};

#endif // CSMPI_CALLSTACK_H
