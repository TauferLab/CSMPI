#include "configuration.hpp"

// Used to parse the configuration file 
#include "external/nlohmann/json.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string>

// Used to broadcast the configuration so that only one MPI process needs to 
// read it in from file
#include "boost/mpi.hpp"
#include "boost/archive/text_oarchive.hpp"
#include "boost/archive/text_iarchive.hpp"

Configuration::Configuration( const std::string config_file_path )
{
  std::ifstream config_stream( config_file_path );
  nlohmann::json config_json = nlohmann::json::parse( config_stream );
  // Set sampling frequencies
  nlohmann::json mpi_functions = config_json["mpi_functions"];
  for ( auto elem : mpi_functions ) {
    fn_to_freq.insert( { elem["name"], (int)elem["freq"] } );
  }
  // Set trace directory
  trace_dir = config_json["trace_dir"];
  // Set backtrace implementation to use 
  backtrace_impl = config_json["backtrace_impl"];
  // Set tracing policy flags
  trace_unmatched = config_json["trace_unmatched"];
  demangle_in_place = config_json["demangle_in_place"];
  translate_in_place = config_json["translate_in_place"];
}

void broadcast_config( Configuration& config ) 
{
  boost::mpi::communicator world;
  int rank = world.rank();
  std::stringstream config_ss;
  boost::archive::text_oarchive config_oarchive { config_ss };
  if ( rank == 0 ) {
    config_oarchive << config;
  }
  std::string config_payload = config_ss.str();
  boost::mpi::broadcast( world, config_payload, 0 );
  if ( rank != 0 ) {
    std::istringstream config_iss( config_payload );
    boost::archive::text_iarchive config_iarchive { config_iss };
    config_iarchive >> config;
  }
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////// Needed for ingest-broadcast idiom //////////////////////
////////////////////////////////////////////////////////////////////////////////

Configuration::Configuration() 
{
}

Configuration& Configuration::operator=( const Configuration& rhs )
{
  if ( &rhs == this ) {
    return *this;
  }
  this->fn_to_freq         = rhs.get_fn_to_freq();
  this->trace_dir          = rhs.get_trace_dir();
  this->backtrace_impl     = rhs.get_backtrace_impl();
  this->demangle_in_place  = rhs.get_demangle_in_place();
  this->translate_in_place = rhs.get_translate_in_place();
  this->trace_unmatched     = rhs.get_trace_unmatched();
  return *this;
}

std::unordered_map<std::string,int> Configuration::get_fn_to_freq() const
{
  return this->fn_to_freq;
}

std::string Configuration::get_trace_dir() const
{
  return this->trace_dir;
}

std::string Configuration::get_backtrace_impl() const
{
  return this->backtrace_impl;
}

bool Configuration::get_trace_unmatched() const
{
  return this->trace_unmatched;
}

bool Configuration::get_demangle_in_place() const
{
  return this->demangle_in_place;
}

bool Configuration::get_translate_in_place() const
{
  return this->translate_in_place;
}

////////////////////////////////////////////////////////////////////////////////

void Configuration::print() const
{
  std::cout << "MPI Function Sampling Frequencies:" << std::endl;
  for ( auto kvp : fn_to_freq ) {
    std::cout << "Function: " << kvp.first 
              << ", Frequency: " << kvp.second 
              << std::endl;
  }
  std::cout << "Trace Directory: " << trace_dir << std::endl;
  std::cout << "Tracing Policy:" << std::endl;
  std::cout << "Backtrace Implementation: " << backtrace_impl << std::endl;
  std::cout << std::boolalpha 
            << "Trace unmatched tests? " << trace_unmatched 
            << std::endl;
  std::cout << "Demangle C++ Function Names in Place? " << demangle_in_place
            << std::endl;
  std::cout << "Translate Return Addresses in Place? " << translate_in_place
            << std::endl;
}
