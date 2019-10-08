#ifndef CSMPI_CONFIGURATION_H
#define CSMPI_CONFIGURATION_H

#include <unordered_map>
#include <string>

// Boost
#include "boost/serialization/access.hpp"
#include "boost/serialization/string.hpp" 
#include "boost/serialization/unordered_map.hpp" 

class Configuration
{
public:
  Configuration( const std::string config_file_path ); 
  Configuration(); 
  // Accessors and assignment operator needed for ingest-broadcast
  Configuration& operator=(const Configuration& rhs );
  std::unordered_map<std::string,int> get_fn_to_freq() const;
  std::string get_backtrace_impl() const;
  bool get_demangle_in_place() const;
  bool get_translate_in_place() const;
  bool get_skip_unmatched() const;
  void print() const;
private:
  // Mapping from MPI function names to call-stack sampling frequency
  std::unordered_map<std::string,int> fn_to_freq;
  // Remaining member variables determine the "tracing policy" of CSMPI
  // Defaults are selected to minimize the amount of work CSMPI has to do at 
  // runtime, and correspondingly how much overhead it imposes.
  // Determines which backtrace implementation to use 
  std::string backtrace_impl;
  // Flag for whether to skip unmatched matching functions 
  bool skip_unmatched = true;
  // Flag for whether to demangle C++ function names at runtime
  bool demangle_in_place = false;
  // Flag for wether to translate return addresses to human-readable function 
  // names, line numbers etc. at runtime
  bool translate_in_place = false;

  // Serialization helper for broadcast 
  friend class boost::serialization::access;
  template<typename Archive>
  void serialize( Archive& archive, const unsigned int version )
  {
    archive & fn_to_freq;
    archive & backtrace_impl;
    archive & skip_unmatched;
    archive & demangle_in_place;
    archive & translate_in_place;
  }
};

void broadcast_config( Configuration& config );

#endif // CSMPI_CONFIGURATION_H

