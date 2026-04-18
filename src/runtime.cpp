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

// ======== BEGIN: glibc sym table construction additions ========
#include <link.h>      // dl_iterate_phdr, dl_phdr_info
#include <elf.h>       // Elf64_Ehdr, Elf64_Shdr, Elf64_Sym, ELF64_ST_TYPE, STT_FUNC
#include <fcntl.h>     // open, O_RDONLY
#include <sys/mman.h>  // mmap, munmap, PROT_READ, MAP_PRIVATE
#include <unistd.h>    // close
#include <map>
// ======== END: glibc sym table construction additions ========

// Callstack tracing via libunwind
#define UNW_LOCAL_ONLY
#ifdef DET_LIBUNWIND
	#include <libunwind.h>
#endif
#include <cxxabi.h>

// Callstack tracing via glibc backtrace
#include <execinfo.h>
#define GCLIB_BACKTRACE_MAX_FRAMES 64

Runtime* csmpi_init( Runtime* runtime_ptr ) 
{
  int mpi_rc, rank;
  mpi_rc = PMPI_Comm_rank( MPI_COMM_WORLD, &rank );
  if ( rank == 0 ) {
    std::cout << "CSMPI Runtime starting up..." << std::endl;
  }
  mpi_rc = PMPI_Barrier( MPI_COMM_WORLD );

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
  // ======== BEGIN: glibc sym table construction additions ========
  if ( runtime_ptr->get_write_symtab() ) {
    runtime_ptr->build_symtab();
    runtime_ptr->write_symtab();
  }
  // ======== END: glibc sym table construction additions ========
  runtime_ptr->write_trace();
  /*int mpi_rc, rank;
  mpi_rc = PMPI_Comm_rank( MPI_COMM_WORLD, &rank );
  double send_buffer[2] = { runtime_ptr->get_backtrace_elapsed_time(), runtime_ptr->get_write_log_elapsed_time() };
  double recv_buffer[2] = { 0.0, 0.0 };
  mpi_rc = PMPI_Reduce( &send_buffer, &recv_buffer, 2, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD );
  if ( rank == 0 ) {
    std::cout << "Backtrace total elapsed time: " << recv_buffer[0] << std::endl;
    std::cout << "Write log total elapsed time: " << recv_buffer[1] << std::endl;
    std::cout << "CSMPI Runtime shutting down..." << std::endl;
  }
  mpi_rc = PMPI_Barrier( MPI_COMM_WORLD );*/
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
    fn_to_last.insert( { fn_name, 0 } );
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


bool Runtime::should_trace(std::string fn_name) const {
  // Only trace if a tracing frequency is given for this function
  auto search = fn_to_freq.find(fn_name);
  if (search != fn_to_freq.end()) {
    // If the tracing frequency is set to 0, trace every call
    if ( fn_to_freq.at(fn_name) == 0 ) {
      return true;
    } 
    else {
      // If we are tracing this function at some different frequency, 
      // always trace the first call
      if ( fn_to_count.at(fn_name) == 0 ) {
        return true; 
      }
      // And trace if the we are on an instance of the function call that 
      // should be traced according to the set frequency for this function
      else if (fn_to_count.at(fn_name) == fn_to_last.at(fn_name) + fn_to_freq.at(fn_name)) {
        return true;
      }
    }
  }
  return false;
}


void Runtime::trace_callstack( std::string fn_name )
{
  const auto trace_call = should_trace(fn_name);
  
  auto backtrace_start_time = std::chrono::steady_clock::now();
  if ( trace_call ) {
    // Get callstack using config-specified backtrace implementation
    Callstack cs;
    auto impl = config.get_backtrace_impl();
#ifdef DET_LIBUNWIND
    if ( impl == "libunwind" ) {
      cs = backtrace_libunwind();
    }
#endif 
    if ( impl == "glibc" ) {
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
    auto callstack_id = std::make_pair( fn_to_count.at(fn_name), curr_callstack_id );
    fn_to_callstack_id_seq.at(fn_name).push_back( callstack_id );
    
    // Update the index at which we most recently traced this function
    fn_to_last.at(fn_name) = fn_to_count.at(fn_name);
  }
  
  // Track how long this tracing event took
  auto backtrace_end_time = std::chrono::steady_clock::now();
  std::chrono::duration<double> backtrace_elapsed_time = backtrace_end_time - backtrace_start_time;
  m_backtrace_elapsed_time += backtrace_elapsed_time.count();

  // Update number of times we've seen this function called
  auto search = fn_to_freq.find(fn_name);
  if (search != fn_to_freq.end()) {
    fn_to_count.at(fn_name) = fn_to_count.at(fn_name) + 1;
  }
}



// Get the callstack using libunwind
// Many thanks to Eli Bendersky's invaluable web presence 
// https://eli.thegreenplace.net/2015/programmatic-access-to-the-call-stack-in-c/
#ifdef DET_LIBUNWIND
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
#endif

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
     uint64_t f = (uint64_t)frames[i];
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
  mpi_rc = PMPI_Comm_rank( MPI_COMM_WORLD, &rank );
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

// ======== BEGIN: glibc sym table construction additions ========

// dl_iterate_phdr callback: fired once per loaded ELF object (main exe + every .so).
// Opens the object's file on disk, parses its ELF section headers to find .symtab
// (static symbols) and .dynsym (exported symbols), and inserts each defined FUNC
// symbol into addr_to_name keyed by its runtime virtual address (bias + st_value).
// Two passes ensure .symtab entries win over .dynsym for the same address, since
// .symtab is richer (includes internal/static functions not present in .dynsym).
static int build_symtab_callback( struct dl_phdr_info* info, size_t size, void* data )
{
  auto& addr_to_name = *static_cast<std::map<uint64_t, std::string>*>( data );

  // dlpi_name is empty for the main executable; /proc/self/exe is always valid
  const char* path = ( info->dlpi_name[0] == '\0' ) ? "/proc/self/exe"
                                                     : info->dlpi_name;
  uint64_t bias = (uint64_t)info->dlpi_addr;

  // Open and mmap the ELF file — the dynamic linker only loaded LOAD segments
  // into memory, so section headers (.symtab, .dynsym) must be read from disk
  int fd = open( path, O_RDONLY );
  if ( fd < 0 ) return 0;

  struct stat st;
  if ( fstat( fd, &st ) < 0 ) { close( fd ); return 0; }
  size_t file_size = (size_t)st.st_size;
  if ( file_size < sizeof( Elf64_Ehdr ) ) { close( fd ); return 0; }

  void* map = mmap( nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0 );
  close( fd );
  if ( map == MAP_FAILED ) return 0;

  const uint8_t*    base = static_cast<const uint8_t*>( map );
  const Elf64_Ehdr* ehdr = static_cast<const Elf64_Ehdr*>( map );

  // Validate ELF magic and class — skip 32-bit objects (Elf32_Sym has different layout)
  if ( ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
       ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
       ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
       ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
       ehdr->e_ident[EI_CLASS] != ELFCLASS64 ) {
    munmap( map, file_size );
    return 0;
  }

  // Bounds check the section header table before treating it as an array
  if ( ehdr->e_shoff == 0 || ehdr->e_shnum == 0 ||
       ehdr->e_shoff + (uint64_t)ehdr->e_shnum * sizeof( Elf64_Shdr ) > file_size ) {
    munmap( map, file_size );
    return 0;
  }

  const Elf64_Shdr* shdrs = reinterpret_cast<const Elf64_Shdr*>( base + ehdr->e_shoff );

  // Two passes: SHT_SYMTAB first, SHT_DYNSYM second.
  // emplace() never overwrites, so .symtab entries take priority.
  for ( int pass = 0; pass < 2; pass++ ) {
    Elf64_Word target_type = ( pass == 0 ) ? SHT_SYMTAB : SHT_DYNSYM;

    for ( int i = 0; i < ehdr->e_shnum; i++ ) {
      const Elf64_Shdr& shdr = shdrs[i];
      if ( shdr.sh_type != target_type ) continue;
      if ( shdr.sh_entsize == 0 )         continue;

      // sh_link for a symbol table section is the index of its string table
      if ( shdr.sh_link >= ehdr->e_shnum ) continue;
      const Elf64_Shdr& strtab_shdr = shdrs[shdr.sh_link];

      // Bounds check both sections within the mmap'd file
      if ( shdr.sh_offset + shdr.sh_size > file_size )                continue;
      if ( strtab_shdr.sh_offset + strtab_shdr.sh_size > file_size )  continue;

      const Elf64_Sym* syms       = reinterpret_cast<const Elf64_Sym*>( base + shdr.sh_offset );
      const char*      strtab     = reinterpret_cast<const char*>( base + strtab_shdr.sh_offset );
      size_t           strtab_size = strtab_shdr.sh_size;
      size_t           sym_count   = shdr.sh_size / shdr.sh_entsize;

      for ( size_t j = 0; j < sym_count; j++ ) {
        const Elf64_Sym& sym = syms[j];

        // Only defined function symbols — skip imports, data, and the null entry
        if ( ELF64_ST_TYPE( sym.st_info ) != STT_FUNC ) continue;
        if ( sym.st_value == 0 )                          continue;
        if ( sym.st_shndx == SHN_UNDEF )                 continue;
        if ( sym.st_name  >= strtab_size )               continue;

        // runtime address = ASLR load bias + ELF virtual address
        uint64_t    runtime_addr = bias + sym.st_value;
        const char* raw_name     = strtab + sym.st_name;

        // Demangle C++ names; fall back to raw mangled name on failure
        std::string name;
        int   status;
        char* demangled = abi::__cxa_demangle( raw_name, nullptr, nullptr, &status );
        if ( status == 0 && demangled != nullptr ) {
          name = demangled;
          std::free( demangled );
        } else {
          name = raw_name;
        }

        addr_to_name.emplace( runtime_addr, std::move( name ) );
      }
    }
  }

  munmap( map, file_size );
  return 0;  // 0 = continue iterating over remaining loaded objects
}

bool Runtime::get_write_symtab() const
{
  return this->config.get_write_symtab();
}

void Runtime::build_symtab()
{
  dl_iterate_phdr( build_symtab_callback, &addr_to_name );
}

void Runtime::write_symtab()
{
  int mpi_rc, rank;
  mpi_rc = PMPI_Comm_rank( MPI_COMM_WORLD, &rank );
  std::string symtab_path = config.get_trace_dir() + "/rank_" + std::to_string( rank ) + ".symtab";
  FILE* f = std::fopen( symtab_path.c_str(), "w" );
  for ( auto& kv : addr_to_name ) {
    std::fprintf( f, "0x%lx\t%s\n", kv.first, kv.second.c_str() );
  }
  std::fclose( f );
}

// Nearest-symbol lookup: frame addresses are return addresses (mid-function),
// so we find the largest known symbol start address <= the queried address.
std::string Runtime::lookup_symbol( uint64_t addr ) const
{
  if ( addr_to_name.empty() ) return "";
  auto it = addr_to_name.upper_bound( addr );  // first entry > addr
  if ( it == addr_to_name.begin() ) return "";  // addr below all known symbols
  --it;
  return it->second;
}

// ======== END: glibc sym table construction additions ========

// Convenience function to print the current state of the CSMPI runtime
void Runtime::print() const
{
  int mpi_rc, flag;
  mpi_rc = PMPI_Initialized( &flag );
  if ( flag ) {
    int rank;
    mpi_rc = PMPI_Comm_rank( MPI_COMM_WORLD, &rank );
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
}
