#ifndef CSMPI_CALLSTACK_H
#define CSMPI_CALLSTACK_H

#define UNW_LOCAL_ONLY
#include <libunwind.h>

class Callstack
{
public:
  Callstack() {}
  Callstack& operator=( const Callstack& rhs );
  void add_frame( unw_word_t frame );
  void print() const;
  std::vector<unw_word_t> get_frames() const;
private:
  std::vector<unw_word_t> frames; 
};

#endif // CSMPI_CALLSTACK_H
