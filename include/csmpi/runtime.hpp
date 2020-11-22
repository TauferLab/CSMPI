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
  void write_trace();
  void print() const;
  double get_backtrace_elapsed_time() const;
  double get_write_log_elapsed_time() const;
  bool should_trace(std::string fn_name) const;
private:
  Configuration config;
  std::unordered_map<std::string, size_t> fn_to_count;
  std::unordered_map<std::string, size_t> fn_to_last;
  std::unordered_map<std::string, int> fn_to_freq;
  size_t m_callstack_id{0};
  std::unordered_map<Callstack, size_t, CallstackHash> callstack_to_id;
  std::unordered_map<size_t, Callstack> id_to_callstack;
  std::unordered_map<std::string, std::vector< std::pair<size_t, size_t> > > fn_to_callstack_id_seq;
  double m_backtrace_elapsed_time{0};
  double m_write_log_elapsed_time{0};
};

Runtime* csmpi_init( Runtime* runtime_ptr );
void csmpi_finalize( Runtime* runtime_ptr );

#endif // CSMPI_RUNTIME_H
