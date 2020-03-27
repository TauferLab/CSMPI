#include "runtime.hpp"
#include "callstack.hpp"

#include <mpi.h>

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

#include <sys/stat.h>

// Callstack tracing via libunwind
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <cxxabi.h>

// Callstack tracing via glibc backtrace
#include <execinfo.h>
#define GCLIB_BACKTRACE_MAX_FRAMES 64

Runtime* csmpi_init( Runtime* runtime_ptr ) 
{
  int mpi_rc, rank;
  mpi_rc = MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  if ( rank == 0 ) {
    std::cout << "CSMPI Runtime starting up..." << std::endl;
  }
  mpi_rc = MPI_Barrier( MPI_COMM_WORLD );

  // Get CSMPI environment variable values
  char* csmpi_config_var = std::getenv( "CSMPI_CONFIG" );
  if ( csmpi_config_var == nullptr ) {
    throw std::runtime_error("CSMPI_CONFIG not set");
  }
  
  // Read in CSMPI config
  Configuration config( csmpi_config_var );
 
  // Set up CSMPI trace directory
  mkdir( config.get_trace_dir().c_str(), S_IRWXU ); 

  // Create CSMPI runtime object
  return new Runtime( config );
}

void csmpi_finalize( Runtime* runtime_ptr )
{
  runtime_ptr->write_trace();
  int mpi_rc, rank;
  mpi_rc = MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  double send_buffer[2] = { runtime_ptr->get_backtrace_elapsed_time(), runtime_ptr->get_write_log_elapsed_time() };
  double recv_buffer[2] = { 0.0, 0.0 };
  mpi_rc = MPI_Reduce( &send_buffer, &recv_buffer, 2, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD );
  if ( rank == 0 ) {
    std::cout << "Backtrace total elapsed time: " << recv_buffer[0] << std::endl;
    std::cout << "Write log total elapsed time: " << recv_buffer[1] << std::endl;
    std::cout << "CSMPI Runtime shutting down..." << std::endl;
  }
  mpi_rc = MPI_Barrier( MPI_COMM_WORLD );
  delete runtime_ptr;
}

Runtime::Runtime( Configuration config )
{
  this->config = config;
  for ( auto fn_freq_pair : config.get_fn_to_freq() ) {
    auto fn_name = fn_freq_pair.first;
    auto tracing_frequency = fn_freq_pair.second;
    fn_to_count.insert( { fn_name, 0 } );
    fn_to_freq.insert( { fn_name, tracing_frequency } );
    std::vector< std::pair<size_t, size_t> > callstack_id_seq;
    fn_to_callstack_id_seq.insert( { fn_name, callstack_id_seq } );
  } 
}

double Runtime::get_backtrace_elapsed_time() const
{
  return this->m_backtrace_elapsed_time;
}

double Runtime::get_write_log_elapsed_time() const 
{
  return this->m_write_log_elapsed_time;
}

bool Runtime::trace_unmatched() const
{
  return this->config.get_trace_unmatched();
}



void Runtime::trace_callstack( std::string fn_name )
{
  // First, determine if we're actually going to trace this call
  // Always trace the first invocation of an MPI function in the map
  bool trace_call = false;
  // Disable tracing for this function if frequency is negative
  if ( fn_to_freq[ fn_name ] >= 0 ) {
    // If we are tracing this function at all, always trace the first call
    if ( fn_to_count[ fn_name ] == 0 ) {
      trace_call = true; 
    }
    else if ( fn_to_count[ fn_name ] == fn_to_last[ fn_name ] + fn_to_freq[ fn_name ] ) {
      trace_call = true;
    }
    else if ( fn_to_freq[ fn_name ] == 0 ) {
      trace_call = true;
    }
  }
  
  auto backtrace_start_time = std::chrono::steady_clock::now();
  if ( trace_call ) {
    // Get callstack using config-specified backtrace implementation
    Callstack cs;
    auto impl = config.get_backtrace_impl();
    if ( impl == "libunwind" ) {
      cs = backtrace_libunwind();
    } 
    else if ( impl == "glibc" ) {
      cs = backtrace_glibc();
    }

    // Update table of known callstacks
    size_t curr_callstack_id;
    auto search = callstack_to_id.find(cs);
    if ( search == callstack_to_id.end() ) {
      curr_callstack_id = m_callstack_id;
      m_callstack_id++;
      callstack_to_id.insert( { cs, curr_callstack_id } );
      id_to_callstack.insert( { curr_callstack_id, cs } );
    } else {
      curr_callstack_id = search->second;
    }

    // Store callstack
    auto callstack_id = std::make_pair( fn_to_count[ fn_name ], curr_callstack_id );
    fn_to_callstack_id_seq[ fn_name ].push_back( callstack_id );
    
    // Update the index at which we most recently traced this function
    fn_to_last[ fn_name ] = fn_to_count[ fn_name ];
  }
  auto backtrace_end_time = std::chrono::steady_clock::now();
  std::chrono::duration<double> backtrace_elapsed_time = backtrace_end_time - backtrace_start_time;
  m_backtrace_elapsed_time += backtrace_elapsed_time.count();

  // Update number of times we've seen this function called
  fn_to_count[ fn_name ] = fn_to_count[ fn_name ] + 1;
}



// Get the callstack using libunwind
// Many thanks to Eli Bendersky's invaluable web presence 
// https://eli.thegreenplace.net/2015/programmatic-access-to-the-call-stack-in-c/
Callstack Runtime::backtrace_libunwind()
{
  Callstack cs;

  unw_cursor_t cursor;
  unw_context_t context;

  // Set cursor to current frame
  unw_getcontext( &context );
  unw_init_local( &cursor, &context );

  // Unwind frames
  while ( unw_step( &cursor ) > 0 ) {
    unw_word_t offset;
    unw_word_t program_counter;
    unw_get_reg( &cursor, UNW_REG_IP, &program_counter );

    if ( program_counter == 0 ) {
      break;
    }

    cs.add_frame( program_counter );
   
    // If we need to translate the program counter address to a function name,
    // line number, etc. at runtime, do this here. 
    // Warning: This is SLOW. 
    if ( config.get_translate_in_place() ) {
      char frame_symbol[256];
      if ( unw_get_proc_name( &cursor, frame_symbol, sizeof( frame_symbol ), &offset ) == 0 ) {
        
        // If we also need to demangle the function name at runtime, do this here
        // Warning: This slows things down even more
        if ( config.get_demangle_in_place() ) {
          char* nameptr = frame_symbol;
          int status;
          char* demangled = abi::__cxa_demangle( frame_symbol, nullptr, nullptr, &status );
          if ( status == 0 ) {
            nameptr = demangled;
          }
          std::free( demangled );
        }
      }
      else {
        printf(" -- error: unable to obtain symbol name for this frame\n");
      }
    }
  }

  return cs; 
}

Callstack Runtime::backtrace_glibc()
{
  Callstack cs;
  size_t max_frames = GCLIB_BACKTRACE_MAX_FRAMES;
  void* frames[ max_frames ];
  size_t size;
  size = backtrace( frames, max_frames );
  //std::printf("# frames traced: %lu\n", size);
  //char** strings;
  size_t i;
  //strings = backtrace_symbols(frames, size);
  //std::printf ("Obtained %zd stack frames.\n", size);

  for (i = 0; i < size; i++) {
     //std::printf ("%s\n", strings[i]);
     //std::printf ("%p\n", frames[i]);
     unw_word_t f = (unw_word_t)frames[i];
     //std::printf ("0x%lx\n", f);
     cs.add_frame( f );
  }

  //free (strings); 
  return cs;
}


void Runtime::write_trace() 
{
  auto start_time = std::chrono::steady_clock::now();
  // Open a trace file for this rank
  int mpi_rc, rank;
  mpi_rc = MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  std::string trace_file_path = config.get_trace_dir() + "/rank_" + std::to_string(rank) + ".csmpi";
  FILE* trace_file = std::fopen( trace_file_path.c_str(), "w" );
  // Iterate over function call types
  for ( auto kvp : fn_to_callstack_id_seq ) {
    std::fprintf(trace_file, "Callstacks for MPI Function: %s\n", kvp.first.c_str() );
    // Iterate over sequence of traced calls of this type
    for ( auto idx_callstack_id_pair : kvp.second ) {
      auto idx = idx_callstack_id_pair.first;
      auto callstack = id_to_callstack.at( idx_callstack_id_pair.second );
      // Print the index of the callstack (i.e., "this is the nth callstack"
      std::fprintf(trace_file, "%lu, ", idx); 
      // Print the callstack itself 
      for ( auto frame : callstack.get_frames() ) {
        std::fprintf(trace_file, "0x%lx ", frame);
      }
      std::fprintf(trace_file, "\n");
    }
  }
  int fclose_rc; 
  fclose_rc = std::fclose( trace_file );
  auto stop_time = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed_time = stop_time - start_time;
  this->m_write_log_elapsed_time = elapsed_time.count();
}

// Convenience function to print the current state of the CSMPI runtime
void Runtime::print() const
{
  int mpi_rc, flag;
  mpi_rc = MPI_Initialized( &flag );
  if ( flag ) {
    int rank;
    mpi_rc = MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    std::cout << "Rank: " << rank << " CSMPI Runtime State:" << std::endl;
  }
  std::cout << "Counts of function calls observed so far:" << std::endl;
  for ( auto kvp : fn_to_count ) {
    std::cout << "Function: " << kvp.first 
              << ", # Seen So Far: " << kvp.second
              << std::endl;
  }
  std::cout << "Numerical Index of last-seen instance of function calls:" << std::endl;
  for ( auto kvp : fn_to_last ) {
    std::cout << "Function: " << kvp.first 
              << ", Index: " << kvp.second
              << std::endl;
  }
  std::cout << "Tracing Frequencies for each Function" << std::endl;
  for ( auto kvp : fn_to_freq ) {
    std::cout << "Function: " << kvp.first 
              << ", Frequency: " << kvp.second
              << std::endl;
  }
  std::cout << "Callstacks for each function so far:" << std::endl;
  for ( auto kvp : fn_to_callstack_id_seq ) {
    std::cout << "Function: " << kvp.first << std::endl;
    for ( auto pair : kvp.second ) {
      auto idx = pair.first;
      auto callstack_id_seq = pair.second;
      // Print the index of the callstack (i.e., "this is the nth callstack"
      std::printf("%lu, ", idx); 
      // Print the callstack itself 
      for ( auto frame : id_to_callstack.at(callstack_id_seq).get_frames() ) {
        std::printf("0x%lx ", frame);
      }
      std::printf("\n");
    }
  }
}
