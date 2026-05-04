#ifndef CSMPI_RUNTIME_H
#define CSMPI_RUNTIME_H

#include "configuration.hpp"
#include "callstack.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

// ======== BEGIN: glibc sym table construction additions ========
#include <map>
#include <link.h>
// ======== END: glibc sym table construction additions ========

class Runtime
{
public:
  Runtime( Configuration config );
  void trace_callstack( std::string fn_name );
  bool trace_unmatched() const;
#ifdef DET_LIBUNWIND
  Callstack backtrace_libunwind();
#endif
  Callstack backtrace_glibc();
  void write_trace();
  void print() const;
  // ======== BEGIN: glibc sym table construction additions ========
  void build_symtab();
  void write_symtab();
  std::string lookup_symbol( uint64_t addr ) const;
  uint64_t lookup_entry_addr( uint64_t addr ) const;
  bool get_write_symtab() const;
  bool get_resolve_to_entry() const;
  // ======== END: glibc sym table construction additions ========
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
  // ======== BEGIN: glibc sym table construction additions ========
  // Ordered map required for nearest-symbol lookup via upper_bound:
  // captured frame addresses are return addresses (mid-function), not entry points
  std::map<uint64_t, std::string> addr_to_name;
  // ======== END: glibc sym table construction additions ========
};

Runtime* csmpi_init( Runtime* runtime_ptr );
void csmpi_finalize( Runtime* runtime_ptr );

#endif // CSMPI_RUNTIME_H
