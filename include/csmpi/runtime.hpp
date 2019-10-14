#ifndef CSMPI_RUNTIME_H
#define CSMPI_RUNTIME_H

#include "configuration.hpp"
#include "callstack.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

class Runtime
{
public:
  Runtime( Configuration config );
  void trace_callstack( std::string fn_name );
  bool trace_unmatched() const;
  Callstack backtrace_libunwind();
  Callstack backtrace_glibc();
  void write_trace() const;
  void print() const;
  void start_timer();
  void stop_timer();
private:
  Configuration config;
  std::unordered_map<std::string, size_t> fn_to_count;
  std::unordered_map<std::string, size_t> fn_to_last;
  std::unordered_map<std::string, int> fn_to_freq;
  std::unordered_map<std::string, std::vector< std::pair<size_t, Callstack> > > fn_to_callstacks;
  std::chrono::time_point<std::chrono::steady_clock> trace_start_time;
};

Runtime* csmpi_init( Runtime* runtime_ptr );
void csmpi_finalize( Runtime* runtime_ptr );

#endif // CSMPI_RUNTIME_H
