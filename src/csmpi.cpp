
#include <mpi.h>
//#include <stdio.h>
//#include <stdlib.h>

#ifndef _EXTERN_C_
#ifdef __cplusplus
#define _EXTERN_C_ extern "C"
#else /* __cplusplus */
#define _EXTERN_C_
#endif /* __cplusplus */
#endif /* _EXTERN_C_ */

#ifdef MPICH_HAS_C2F
_EXTERN_C_ void *MPIR_ToPointer(int);
#endif // MPICH_HAS_C2F

#ifdef PIC
/* For shared libraries, declare these weak and figure out which one was linked
   based on which init wrapper was called.  See mpi_init wrappers.  */
#pragma weak pmpi_init
#pragma weak PMPI_INIT
#pragma weak pmpi_init_
#pragma weak pmpi_init__
#endif /* PIC */

_EXTERN_C_ void pmpi_init(MPI_Fint *ierr);
_EXTERN_C_ void PMPI_INIT(MPI_Fint *ierr);
_EXTERN_C_ void pmpi_init_(MPI_Fint *ierr);
_EXTERN_C_ void pmpi_init__(MPI_Fint *ierr);

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <cxxabi.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include <istream>
#include <ostream>
#include <iostream>
#include <vector>
#include <zlib.h>

// For fprintf logging
FILE* logfile_ptr;
int num_ranks = -1;

// For logging send initiations
int rank = -1;
int send_idx = 0;
int bsend_idx = 0;
int rsend_idx = 0;
int ssend_idx = 0;
int isend_idx = 0;
int ibsend_idx = 0;
int irsend_idx = 0;
int issend_idx = 0;

// For logging recv request initiations
int recv_idx = 0;
int irecv_idx = 0;

// For logging recv completions
#define RECV_REQUEST (152)
int wait_idx = 0;
int waitany_idx = 0;
int waitsome_idx = 0;
int waitall_idx = 0;
int test_idx = 0;
int testsome_idx = 0;
int testany_idx = 0;

// For logging probe stuff
int probe_idx = 0;
int iprobe_idx = 0;

// For logging bookkeeping functions
int comm_rank_idx = 0;
int comm_size_idx = 0;

// Profiling
size_t unwind_time_msec = 0;
size_t write_time_msec = 0;
clock_t unwind_start;
clock_t write_start;
clock_t unwind_duration;
clock_t write_duration;

unsigned int n_unmatched_tests;

//#define SANITY_CHECK_ON_FINALIZE
std::vector< std::vector <std::string> > callstacks;
std::vector< std::string > signatures;

// Call this function to get a backtrace.
void backtrace() {
  // Intermediary vars for pushing to the trace data structure
  std::vector<std::string> callstack;
  char buf[1024];
  unw_cursor_t cursor;
  unw_context_t context;

  // Initialize cursor to current frame for local unwinding.
  unw_getcontext(&context);
  unw_init_local(&cursor, &context);

  // Unwind frames one by one, going up the frame stack.
  while (unw_step(&cursor) > 0) {
    unw_word_t offset, pc;
    unw_get_reg(&cursor, UNW_REG_IP, &pc);
    if (pc == 0) {
      break;
    }
    char sym[256];
    if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
      char* nameptr = sym;
      int status;
      char* demangled = abi::__cxa_demangle(sym, nullptr, nullptr, &status);
      if (status == 0) {
        nameptr = demangled;
      }
      std::snprintf(buf, sizeof(buf), "0x%lx: (%s+0x%lx)\n", pc, nameptr, offset);
      std::string buf_str = buf;
      callstack.push_back(buf_str);
      memset(buf, '\0', sizeof(buf));
      std::free(demangled);
    } 
    else {
      callstack.push_back(" -- error: unable to obtain symbol name for this frame\n");

    }
  }
  callstack.push_back("end callstack\n");
  callstacks.push_back(callstack);
}

// Write the trace out during MPI_Finalize
void write_trace() {
  for (int i=0; i<callstacks.size(); i++) {
    fprintf(logfile_ptr, "%s", signatures[i].c_str());
    for (int j=0; j<callstacks[i].size(); j++) {
      fprintf(logfile_ptr, "%s", callstacks[i][j].c_str());
    }
  }
}


// Sets up everything for logging
#define CSMPI_ENV_NAME_DIR "CSMPI_DIR"
void csmpi_init() {
  // Get logfile dir path from environment var
  std::string csmpi_logfile_dir_path;
  char* env_var;
  env_var = getenv(CSMPI_ENV_NAME_DIR);
  if (NULL == env_var) {
    printf("getenv failed, specify CSMPI_DIR\n");
    exit(0);
  } else {
    csmpi_logfile_dir_path = env_var; 
    mkdir(csmpi_logfile_dir_path.c_str(), S_IRWXU);
  }
  // Get comm size
  PMPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
  // Get rank and set global rank variable
  int my_rank, mpi_err;
  mpi_err = PMPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  if (mpi_err != MPI_SUCCESS) {
    printf("MPI_Comm_rank failed during csmpi_init\n");
    exit(0);
  } 
  rank = my_rank;
  // Check that global rank variable has been properly set
  if (rank < 0) {
    printf("Rank incorrect\n");
    exit(0);
  }
  // Open logfile for this rank
  std::string logfile_path = csmpi_logfile_dir_path + "/rank_" + std::to_string((unsigned long long)my_rank) + ".csmpi";
  logfile_ptr = fopen(logfile_path.c_str(), "w");
}


/* ================================================================
 * Initialization, finalization, and bookkeeping functions
 * ================================================================
 */
/* ================== C Wrappers for MPI_Init ================== */
_EXTERN_C_ int PMPI_Init(int *arg_0, char ***arg_1);
_EXTERN_C_ int MPI_Init(int *arg_0, char ***arg_1) { 
    int _wrap_py_return_val = 0;
 
{
  _wrap_py_return_val = PMPI_Init(arg_0, arg_1);
  csmpi_init();
  char buf[256];
  std::snprintf(buf, sizeof(buf), "call=init\n");
  std::string buf_str = buf;
  signatures.push_back(buf_str);

  unwind_start = clock();
  backtrace();
  unwind_duration = clock() - unwind_start;
  unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
}
    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Init_thread ================== */
_EXTERN_C_ int PMPI_Init_thread(int *arg_0, char ***arg_1, int arg_2, int *arg_3);
_EXTERN_C_ int MPI_Init_thread(int *arg_0, char ***arg_1, int arg_2, int *arg_3) { 
    int _wrap_py_return_val = 0;
 
{
  _wrap_py_return_val = PMPI_Init_thread(arg_0, arg_1, arg_2, arg_3);
  csmpi_init();
  char buf[256];
  std::snprintf(buf, sizeof(buf), "call=init_thread\n");
  std::string buf_str = buf;
  signatures.push_back(buf_str);

  unwind_start = clock();
  backtrace();
  unwind_duration = clock() - unwind_start;
  unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
}
    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Finalize ================== */
_EXTERN_C_ int PMPI_Finalize();
_EXTERN_C_ int MPI_Finalize() { 
    int _wrap_py_return_val = 0;
 
{
  _wrap_py_return_val = PMPI_Finalize();
  char buf[256];
  std::snprintf(buf, sizeof(buf), "call=finalize\n");
  std::string buf_str = buf;
  signatures.push_back(buf_str);

  unwind_start = clock();
  backtrace();
  unwind_duration = clock() - unwind_start;
  unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);

#ifdef SANITY_CHECK_ON_FINALIZE
  // Print sanity check of trace for rank 0
  if (rank == 0) {
    for(int i=0; i<callstacks.size(); i++) {
      for (int j=0; j<callstacks[i].size(); j++) {
        std::cout << callstacks[i][j] << callstacks[i][j].size() << std::endl;
      }
      std::cout << std::endl;
    }
  }
#endif

  write_start = clock();
  write_trace();
  write_duration = clock() - write_start;
  write_time_msec += (size_t)(write_duration * 1000 / CLOCKS_PER_SEC);
  
  // Close trace file
  int err;
  err = fclose(logfile_ptr);

  printf("Rank %d - unwind time = %llu, write time = %llu, number of unmatched tests = %llu\n", rank, unwind_time_msec, write_time_msec, n_unmatched_tests);
}
    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Comm_rank ================== */
_EXTERN_C_ int PMPI_Comm_rank(MPI_Comm arg_0, int *arg_1);
_EXTERN_C_ int MPI_Comm_rank(MPI_Comm arg_0, int *arg_1) { 
    int _wrap_py_return_val = 0;
 
{
  _wrap_py_return_val = PMPI_Comm_rank(arg_0, arg_1);
  char buf[256];
  std::snprintf(buf, sizeof(buf), "call=comm_rank,%d\n", comm_rank_idx);
  std::string buf_str = buf;
  signatures.push_back(buf_str);
  comm_rank_idx++;

  unwind_start = clock();
  backtrace();
  unwind_duration = clock() - unwind_start;
  unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
}
    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Comm_size ================== */
_EXTERN_C_ int PMPI_Comm_size(MPI_Comm arg_0, int *arg_1);
_EXTERN_C_ int MPI_Comm_size(MPI_Comm arg_0, int *arg_1) { 
    int _wrap_py_return_val = 0;
 
{
  _wrap_py_return_val = PMPI_Comm_size(arg_0, arg_1);
  char buf[256];
  std::snprintf(buf, sizeof(buf), "call=comm_size,%d\n", comm_size_idx);
  std::string buf_str = buf;
  signatures.push_back(buf_str);
  comm_size_idx++;

  unwind_start = clock();
  backtrace();
  unwind_duration = clock() - unwind_start;
  unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
}
    return _wrap_py_return_val;
}



/* ================================================================
 * Send-initiation functions
 * ================================================================
 */
/* ================== C Wrappers for MPI_Send ================== */
_EXTERN_C_ int PMPI_Send(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Send(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5) { 
    int _wrap_py_return_val = 0;
 
{
  _wrap_py_return_val = PMPI_Send(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  char buf[256];
  std::snprintf(buf, sizeof(buf), "call=send,%d,%d,%d,%d\n", send_idx, arg_1, arg_3, arg_4);
  std::string buf_str = buf;
  signatures.push_back(buf_str);
  send_idx++;

  unwind_start = clock();
  backtrace();
  unwind_duration = clock() - unwind_start;
  unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);

}
    return _wrap_py_return_val;
}
/* ================== C Wrappers for MPI_Isend ================== */
_EXTERN_C_ int PMPI_Isend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
_EXTERN_C_ int MPI_Isend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) { 
    int _wrap_py_return_val = 0;
 
{
  _wrap_py_return_val = PMPI_Isend(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  char buf[256];
  std::snprintf(buf, sizeof(buf), "call=isend,%d,%d,%d,%d\n", isend_idx, arg_1, arg_3, arg_4);
  std::string buf_str = buf;
  signatures.push_back(buf_str);
  isend_idx++;
  
  unwind_start = clock();
  backtrace();
  unwind_duration = clock() - unwind_start;
  unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
}
    return _wrap_py_return_val;
}

/* ================================================================
 * Recv-initiation functions
 * ================================================================
 */
/* ================== C Wrappers for MPI_Irecv ================== */
_EXTERN_C_ int PMPI_Irecv(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
_EXTERN_C_ int MPI_Irecv(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) { 
    int _wrap_py_return_val = 0;
 
{
  _wrap_py_return_val = PMPI_Irecv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  char buf[256];
  std::snprintf(buf, sizeof(buf), "call=irecv,%d,%d,%d,%d\n", irecv_idx, arg_1, arg_3, arg_4);
  std::string buf_str = buf;
  signatures.push_back(buf_str);
  irecv_idx += 1;
  
  unwind_start = clock();
  backtrace();
  unwind_duration = clock() - unwind_start;
  unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
}
    return _wrap_py_return_val;
}
/* ================== C Wrappers for MPI_Recv ================== */
_EXTERN_C_ int PMPI_Recv(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Status *arg_6);
_EXTERN_C_ int MPI_Recv(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Status *arg_6) { 
    int _wrap_py_return_val = 0;
 
{
  _wrap_py_return_val = PMPI_Recv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  char buf[256];
  std::snprintf(buf, sizeof(buf), "call=recv,%d,%d,%d,%d\n", recv_idx, arg_1, arg_3, arg_4);
  std::string buf_str = buf;
  signatures.push_back(buf_str);
  recv_idx += 1;
  
  unwind_start = clock();
  backtrace();
  unwind_duration = clock() - unwind_start;
  unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
}
    return _wrap_py_return_val;
}

/* ================================================================
 * Request-completion functions
 * ================================================================
 */
/* ================== C Wrappers for MPI_Test ================== */
_EXTERN_C_ int PMPI_Test(MPI_Request *arg_0, int *arg_1, MPI_Status *arg_2);
_EXTERN_C_ int MPI_Test(MPI_Request *arg_0, int *arg_1, MPI_Status *arg_2) { 
    int _wrap_py_return_val = 0;

{
  _wrap_py_return_val = PMPI_Test(arg_0, arg_1, arg_2);
  // Case: Matched test
  if (*arg_1) {
    // Subcase: Status fields filled
    if (arg_2 != MPI_STATUS_IGNORE) {
      char buf[256];
      std::snprintf(buf, sizeof(buf), "call=mtest,%d,%d,%d\n", test_idx, arg_2->MPI_SOURCE, arg_2->MPI_TAG);
      std::string buf_str = buf;
      signatures.push_back(buf_str);
      test_idx++;
  
      unwind_start = clock();
      backtrace();
      unwind_duration = clock() - unwind_start;
      unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
    }
    // Subcase: Ignore status
    else {
      char buf[256];
      std::snprintf(buf, sizeof(buf), "call=mtest_statusignore,%d\n", test_idx);
      std::string buf_str = buf;
      signatures.push_back(buf_str);
      test_idx++;
      
      unwind_start = clock();
      backtrace();
      unwind_duration = clock() - unwind_start;
      unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
    }
  }
  // Case: Unmatched test
  else {
    char buf[256];
    std::snprintf(buf, sizeof(buf), "call=umtest,%d\n", test_idx);
    std::string buf_str = buf;
    signatures.push_back(buf_str);
    test_idx++;

    n_unmatched_tests += 1;

    unwind_start = clock();
    backtrace();
    unwind_duration = clock() - unwind_start;
    unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
  }
}
    return _wrap_py_return_val;
}

/////* ================== C Wrappers for MPI_Testall ================== */
////_EXTERN_C_ int PMPI_Testall(int arg_0, MPI_Request *arg_1, int *arg_2, MPI_Status *arg_3);
////_EXTERN_C_ int MPI_Testall(int arg_0, MPI_Request *arg_1, int *arg_2, MPI_Status *arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Testall(arg_0, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
///* ================== C Wrappers for MPI_Testany ================== */
//_EXTERN_C_ int PMPI_Testany(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4);
//_EXTERN_C_ int MPI_Testany(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4) { 
//    int _wrap_py_return_val = 0;
// 
//{
//  _wrap_py_return_val = PMPI_Testany(arg_0, arg_1, arg_2, arg_3, arg_4);
//  // Case: Matched test
//  if (*arg_3) {
//    // Subcase: Status fields filled
//    if (arg_4 != MPI_STATUS_IGNORE) {
//      MPI_Status stat = arg_4[(*arg_2)];
//      fprintf(logfile_ptr, "call=testany,1,%d,%d,%d\n", testany_idx, stat.MPI_SOURCE, stat.MPI_TAG);
//      testany_idx++;
//      backtrace();
//    }
//    // Subcase: Ignore status
//    else {
//      //fprintf(logfile_ptr, "mti,%d\n", testany_idx);
//    }
//  }
//  // Case: Unmatched test
//  else {
//    fprintf(logfile_ptr, "call=testany,0\n");
//    //backtrace();
//    testany_idx++;
//  }
//}
//    return _wrap_py_return_val;
//}
//
///* ================== C Wrappers for MPI_Testsome ================== */
//_EXTERN_C_ int PMPI_Testsome(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4);
//_EXTERN_C_ int MPI_Testsome(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4) { 
//    int _wrap_py_return_val = 0;
// 
//{
//  _wrap_py_return_val = PMPI_Testsome(arg_0, arg_1, arg_2, arg_3, arg_4);
//  int num_original_reqs = arg_0;
//  int num_completed_reqs = *arg_2;
//  int i, idx;
//  // Check if a match has been made
//  if (num_completed_reqs > 0) {
//    // Only attempt to log message matching information if statuses are filled in.
//    if (arg_4 != MPI_STATUSES_IGNORE) {
//      fprintf(logfile_ptr, "call=testsome,1,%d,%d,%d", testsome_idx, num_original_reqs, num_completed_reqs);
//      for (i=0; i<num_completed_reqs; i++) {
//        idx = arg_3[i];
//        if (0 <= arg_4[idx].MPI_SOURCE <= num_ranks) {
//          fprintf(logfile_ptr, ",%d,%d", arg_4[idx].MPI_SOURCE, arg_4[idx].MPI_TAG);
//        }
//      }
//      fprintf(logfile_ptr, "\n");
//      backtrace();
//    }
//  }
//  else {
//    fprintf(logfile_ptr, "call=testsome,0,\n");
//    //backtrace();
//  }
//  testsome_idx++;
//}
//    return _wrap_py_return_val;
//}
///* ================== C Wrappers for MPI_Wait ================== */
//_EXTERN_C_ int PMPI_Wait(MPI_Request *arg_0, MPI_Status *arg_1);
//_EXTERN_C_ int MPI_Wait(MPI_Request *arg_0, MPI_Status *arg_1) { 
//    int _wrap_py_return_val = 0;
// 
//{
//  _wrap_py_return_val = PMPI_Wait(arg_0, arg_1);
//  fprintf(logfile_ptr, "call=wait,%d,%d\n", arg_1->MPI_SOURCE, arg_1->MPI_TAG);
//  wait_idx++;
//  backtrace();
//}
//    return _wrap_py_return_val;
//}
//
/* ================== C Wrappers for MPI_Waitall ================== */
_EXTERN_C_ int PMPI_Waitall(int arg_0, MPI_Request *arg_1, MPI_Status *arg_2);
_EXTERN_C_ int MPI_Waitall(int arg_0, MPI_Request *arg_1, MPI_Status *arg_2) { 
    int _wrap_py_return_val = 0;
 
{
  _wrap_py_return_val = PMPI_Waitall(arg_0, arg_1, arg_2);
  char buf[256];
  std::snprintf(buf, sizeof(buf), "call=waitall,%d\n", waitall_idx);
  std::string buf_str = buf;
  signatures.push_back(buf_str);
  waitall_idx++;

  unwind_start = clock();
  backtrace();
  unwind_duration = clock() - unwind_start;
  unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
}
    return _wrap_py_return_val;
}

///* ================== C Wrappers for MPI_Waitany ================== */
//_EXTERN_C_ int PMPI_Waitany(int arg_0, MPI_Request *arg_1, int *arg_2, MPI_Status *arg_3);
//_EXTERN_C_ int MPI_Waitany(int arg_0, MPI_Request *arg_1, int *arg_2, MPI_Status *arg_3) { 
//    int _wrap_py_return_val = 0;
// 
//{
//  _wrap_py_return_val = PMPI_Waitany(arg_0, arg_1, arg_2, arg_3);
//  if (arg_3 != MPI_STATUS_IGNORE) {
//    fprintf(logfile_ptr, "call=waitany,%d,%d\n", arg_3->MPI_SOURCE, arg_3->MPI_TAG);
//    waitany_idx++;
//    backtrace();
//  }
//  else {
//    //fprintf(logfile_ptr, "call=waitany, rank=%d, idx=%d, src=-1, tag=-1\n", rank, waitany_idx);
//    //waitany_idx++;
//    //backtrace();
//  }
//}
//    return _wrap_py_return_val;
//}
//
///* ================== C Wrappers for MPI_Waitsome ================== */
//_EXTERN_C_ int PMPI_Waitsome(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4);
//_EXTERN_C_ int MPI_Waitsome(int arg_0, MPI_Request *arg_1, int *arg_2, int *arg_3, MPI_Status *arg_4) { 
//    int _wrap_py_return_val = 0;
// 
//{
//  _wrap_py_return_val = PMPI_Waitsome(arg_0, arg_1, arg_2, arg_3, arg_4);
//  int incount = arg_0;
//  int outcount = *arg_2;
//  int i, idx;
//  // Check that requests were not ignored
//  if (outcount != MPI_UNDEFINED) {
//    // Check that statuses were actually filled
//    if (arg_4 != MPI_STATUSES_IGNORE) {
//      fprintf(logfile_ptr, "call=waitsome,%d,%d,%d", waitsome_idx, incount, outcount); 
//      for (i=0; i<outcount; i++) {
//        idx = arg_3[i];
//        if (0 <= arg_4[idx].MPI_SOURCE <= num_ranks) {
//          fprintf(logfile_ptr, ",%d,%d", arg_4[idx].MPI_SOURCE, arg_4[idx].MPI_TAG);
//        }
//      }
//      fprintf(logfile_ptr, "\n");
//      backtrace();
//    }
//  }
//  waitsome_idx++;
//}
//    return _wrap_py_return_val;
//}



///* ================== C Wrappers for MPI_Probe ================== */
//_EXTERN_C_ int PMPI_Probe(int arg_0, int arg_1, MPI_Comm arg_2, MPI_Status *arg_3);
//_EXTERN_C_ int MPI_Probe(int arg_0, int arg_1, MPI_Comm arg_2, MPI_Status *arg_3) { 
//    int _wrap_py_return_val = 0;
// 
//{
//  _wrap_py_return_val = PMPI_Probe(arg_0, arg_1, arg_2, arg_3);
//}
//    return _wrap_py_return_val;
//}

/* ================== C Wrappers for MPI_Iprobe ================== */
_EXTERN_C_ int PMPI_Iprobe(int arg_0, int arg_1, MPI_Comm arg_2, int *flag, MPI_Status *arg_4);
_EXTERN_C_ int MPI_Iprobe(int arg_0, int arg_1, MPI_Comm arg_2, int *flag, MPI_Status *arg_4) { 
    int _wrap_py_return_val = 0;
 
{
  _wrap_py_return_val = PMPI_Iprobe(arg_0, arg_1, arg_2, flag, arg_4);
  // Case 1: Probe indicates message arrival
  if (*flag) {
    if (arg_4 != MPI_STATUS_IGNORE) {
      // Log signature
      char buf[256];
      std::snprintf(buf, sizeof(buf), "call=miprobe,%d,%d,%d\n", iprobe_idx, arg_4->MPI_SOURCE, arg_4->MPI_TAG);
      std::string buf_str = buf;
      signatures.push_back(buf_str);
      iprobe_idx++;
      // Log callstack
      unwind_start = clock();
      backtrace();
      unwind_duration = clock() - unwind_start;
      unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
    }
    // Subcase: Ignore status
    else {
      // Log signature
      char buf[256];
      std::snprintf(buf, sizeof(buf), "call=miprobe_statusignore,%d\n", iprobe_idx);
      std::string buf_str = buf;
      signatures.push_back(buf_str);
      iprobe_idx++;
      // Log callstack
      unwind_start = clock();
      backtrace();
      unwind_duration = clock() - unwind_start;
      unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
    }
  }
  // Case 2: Probe indicates no message arrival
  else {
    // Log signature
    char buf[256];
    std::snprintf(buf, sizeof(buf), "call=uiprobe,%d\n", iprobe_idx);
    std::string buf_str = buf;
    signatures.push_back(buf_str);
    iprobe_idx++;
    // Log callstack
    unwind_start = clock();
    backtrace();
    unwind_duration = clock() - unwind_start;
    unwind_time_msec += (size_t)(unwind_duration * 1000 / CLOCKS_PER_SEC);
  }
}
    return _wrap_py_return_val;
}
















// Abandon all hope 



















///* ================== C Wrappers for MPI_Bsend ================== */
//_EXTERN_C_ int PMPI_Bsend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5);
//_EXTERN_C_ int MPI_Bsend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5) { 
//    int _wrap_py_return_val = 0;
// 
//{
//  _wrap_py_return_val = PMPI_Bsend(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
//  //// Write properties of this send initiation to logfile 
//  //// such that sender-nondeterminism can be detected by 
//  //// comparing logs across multiple runs. 
//  //fprintf(logfile_ptr, "call=bsend, rank=%d, idx=%d, count=%d, dtype=%d, dest=%d, tag=%d, comm=%d\n",
//  //        rank, bsend_idx, arg_1, arg_2, arg_3, arg_4, arg_5);
//  //bsend_idx++;
//  // Write callstack so we know how this send is being initiated. 
//  //backtrace();
//}
//    return _wrap_py_return_val;
//}
///* ================== C Wrappers for MPI_Ibsend ================== */
//_EXTERN_C_ int PMPI_Ibsend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
//_EXTERN_C_ int MPI_Ibsend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) { 
//    int _wrap_py_return_val = 0;
// 
//{
//  _wrap_py_return_val = PMPI_Ibsend(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
//  //// Write properties of this send initiation to logfile 
//  //// such that sender-nondeterminism can be detected by 
//  //// comparing logs across multiple runs. 
//  //fprintf(logfile_ptr, "call=ibsend, rank=%d, idx=%d, count=%d, dtype=%d, dest=%d, tag=%d, comm=%d\n",
//  //        rank, ibsend_idx, arg_1, arg_2, arg_3, arg_4, arg_5);
//  //ibsend_idx++;
//  //// Write callstack so we know how this send is being initiated. 
//  //backtrace();
//}
//    return _wrap_py_return_val;
//}
///* ================== C Wrappers for MPI_Irsend ================== */
//_EXTERN_C_ int PMPI_Irsend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
//_EXTERN_C_ int MPI_Irsend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) { 
//    int _wrap_py_return_val = 0;
// 
//{
//  _wrap_py_return_val = PMPI_Irsend(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
//  //// Write properties of this send initiation to logfile 
//  //// such that sender-nondeterminism can be detected by 
//  //// comparing logs across multiple runs. 
//  //fprintf(logfile_ptr, "call=irsend, rank=%d, idx=%d, count=%d, dtype=%d, dest=%d, tag=%d, comm=%d\n",
//  //        rank, irsend_idx, arg_1, arg_2, arg_3, arg_4, arg_5);
//  //irsend_idx++;
//  //// Write callstack so we know how this send is being initiated. 
//  //backtrace();
//}
//    return _wrap_py_return_val;
//}
///* ================== C Wrappers for MPI_Issend ================== */
//_EXTERN_C_ int PMPI_Issend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
//_EXTERN_C_ int MPI_Issend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) { 
//    int _wrap_py_return_val = 0;
// 
//{
//  _wrap_py_return_val = PMPI_Issend(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
//  //// Write properties of this send initiation to logfile 
//  //// such that sender-nondeterminism can be detected by 
//  //// comparing logs across multiple runs. 
//  //fprintf(logfile_ptr, "call=issend, rank=%d, idx=%d, count=%d, dtype=%d, dest=%d, tag=%d, comm=%d\n",
//  //        rank, issend_idx, arg_1, arg_2, arg_3, arg_4, arg_5);
//  //issend_idx++;
//  //// Write callstack so we know how this send is being initiated. 
//  //backtrace();
//}
//    return _wrap_py_return_val;
//}
///* ================== C Wrappers for MPI_Rsend ================== */
//_EXTERN_C_ int PMPI_Rsend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5);
//_EXTERN_C_ int MPI_Rsend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5) { 
//    int _wrap_py_return_val = 0;
// 
//{
//  _wrap_py_return_val = PMPI_Rsend(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
//  //// Write properties of this send initiation to logfile 
//  //// such that sender-nondeterminism can be detected by 
//  //// comparing logs across multiple runs. 
//  //fprintf(logfile_ptr, "call=rsend, rank=%d, idx=%d, count=%d, dtype=%d, dest=%d, tag=%d, comm=%d\n",
//  //        rank, rsend_idx, arg_1, arg_2, arg_3, arg_4, arg_5);
//  //rsend_idx++;
//  //// Write callstack so we know how this send is being initiated. 
//  //backtrace();
//}
//    return _wrap_py_return_val;
//}
///* ================== C Wrappers for MPI_Ssend ================== */
//_EXTERN_C_ int PMPI_Ssend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5);
//_EXTERN_C_ int MPI_Ssend(const void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5) { 
//    int _wrap_py_return_val = 0;
// 
//{
//  _wrap_py_return_val = PMPI_Ssend(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
//  //// Write properties of this send initiation to logfile 
//  //// such that sender-nondeterminism can be detected by 
//  //// comparing logs across multiple runs. 
//  //fprintf(logfile_ptr, "call=ssend, rank=%d, idx=%d, count=%d, dtype=%d, dest=%d, tag=%d, comm=%d\n",
//  //        rank, ssend_idx, arg_1, arg_2, arg_3, arg_4, arg_5);
//  //ssend_idx++;
//  //// Write callstack so we know how this send is being initiated. 
//  //backtrace();
//}
//    return _wrap_py_return_val;
//}
//
/////* ================== C Wrappers for MPI_Abort ================== */
////_EXTERN_C_ int PMPI_Abort(MPI_Comm arg_0, int arg_1);
////_EXTERN_C_ int MPI_Abort(MPI_Comm arg_0, int arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Abort(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Address ================== */
////_EXTERN_C_ int PMPI_Address(void *arg_0, MPI_Aint *arg_1);
////_EXTERN_C_ int MPI_Address(void *arg_0, MPI_Aint *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Address(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Allgather ================== */
////_EXTERN_C_ int PMPI_Allgather(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6);
////_EXTERN_C_ int MPI_Allgather(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Allgather(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Allgatherv ================== */
////_EXTERN_C_ int PMPI_Allgatherv(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int *arg_4, int *arg_5, MPI_Datatype arg_6, MPI_Comm arg_7);
////_EXTERN_C_ int MPI_Allgatherv(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int *arg_4, int *arg_5, MPI_Datatype arg_6, MPI_Comm arg_7) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Allgatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Allreduce ================== */
////_EXTERN_C_ int PMPI_Allreduce(void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5);
////_EXTERN_C_ int MPI_Allreduce(void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Allreduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Alltoall ================== */
////_EXTERN_C_ int PMPI_Alltoall(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6);
////_EXTERN_C_ int MPI_Alltoall(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Alltoall(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Alltoallv ================== */
////_EXTERN_C_ int PMPI_Alltoallv(void *arg_0, int *arg_1, int *arg_2, MPI_Datatype arg_3, void *arg_4, int *arg_5, int *arg_6, MPI_Datatype arg_7, MPI_Comm arg_8);
////_EXTERN_C_ int MPI_Alltoallv(void *arg_0, int *arg_1, int *arg_2, MPI_Datatype arg_3, void *arg_4, int *arg_5, int *arg_6, MPI_Datatype arg_7, MPI_Comm arg_8) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Alltoallv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Attr_delete ================== */
////_EXTERN_C_ int PMPI_Attr_delete(MPI_Comm arg_0, int arg_1);
////_EXTERN_C_ int MPI_Attr_delete(MPI_Comm arg_0, int arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Attr_delete(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Attr_get ================== */
////_EXTERN_C_ int PMPI_Attr_get(MPI_Comm arg_0, int arg_1, void *arg_2, int *arg_3);
////_EXTERN_C_ int MPI_Attr_get(MPI_Comm arg_0, int arg_1, void *arg_2, int *arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Attr_get(arg_0, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Attr_put ================== */
////_EXTERN_C_ int PMPI_Attr_put(MPI_Comm arg_0, int arg_1, void *arg_2);
////_EXTERN_C_ int MPI_Attr_put(MPI_Comm arg_0, int arg_1, void *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Attr_put(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Barrier ================== */
////_EXTERN_C_ int PMPI_Barrier(MPI_Comm arg_0);
////_EXTERN_C_ int MPI_Barrier(MPI_Comm arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Barrier(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Bcast ================== */
////_EXTERN_C_ int PMPI_Bcast(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, MPI_Comm arg_4);
////_EXTERN_C_ int MPI_Bcast(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, MPI_Comm arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Bcast(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
//
/////* ================== C Wrappers for MPI_Bsend_init ================== */
////_EXTERN_C_ int PMPI_Bsend_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
////_EXTERN_C_ int MPI_Bsend_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Bsend_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Buffer_attach ================== */
////_EXTERN_C_ int PMPI_Buffer_attach(void *arg_0, int arg_1);
////_EXTERN_C_ int MPI_Buffer_attach(void *arg_0, int arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Buffer_attach(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Buffer_detach ================== */
////_EXTERN_C_ int PMPI_Buffer_detach(void *arg_0, int *arg_1);
////_EXTERN_C_ int MPI_Buffer_detach(void *arg_0, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Buffer_detach(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Cancel ================== */
////_EXTERN_C_ int PMPI_Cancel(MPI_Request *arg_0);
////_EXTERN_C_ int MPI_Cancel(MPI_Request *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Cancel(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Cart_coords ================== */
////_EXTERN_C_ int PMPI_Cart_coords(MPI_Comm arg_0, int arg_1, int arg_2, int *arg_3);
////_EXTERN_C_ int MPI_Cart_coords(MPI_Comm arg_0, int arg_1, int arg_2, int *arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Cart_coords(arg_0, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Cart_create ================== */
////_EXTERN_C_ int PMPI_Cart_create(MPI_Comm arg_0, int arg_1, int *arg_2, int *arg_3, int arg_4, MPI_Comm *arg_5);
////_EXTERN_C_ int MPI_Cart_create(MPI_Comm arg_0, int arg_1, int *arg_2, int *arg_3, int arg_4, MPI_Comm *arg_5) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Cart_create(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Cart_get ================== */
////_EXTERN_C_ int PMPI_Cart_get(MPI_Comm arg_0, int arg_1, int *arg_2, int *arg_3, int *arg_4);
////_EXTERN_C_ int MPI_Cart_get(MPI_Comm arg_0, int arg_1, int *arg_2, int *arg_3, int *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Cart_get(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Cart_map ================== */
////_EXTERN_C_ int PMPI_Cart_map(MPI_Comm arg_0, int arg_1, int *arg_2, int *arg_3, int *arg_4);
////_EXTERN_C_ int MPI_Cart_map(MPI_Comm arg_0, int arg_1, int *arg_2, int *arg_3, int *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Cart_map(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Cart_rank ================== */
////_EXTERN_C_ int PMPI_Cart_rank(MPI_Comm arg_0, int *arg_1, int *arg_2);
////_EXTERN_C_ int MPI_Cart_rank(MPI_Comm arg_0, int *arg_1, int *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Cart_rank(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Cart_shift ================== */
////_EXTERN_C_ int PMPI_Cart_shift(MPI_Comm arg_0, int arg_1, int arg_2, int *arg_3, int *arg_4);
////_EXTERN_C_ int MPI_Cart_shift(MPI_Comm arg_0, int arg_1, int arg_2, int *arg_3, int *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Cart_shift(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Cart_sub ================== */
////_EXTERN_C_ int PMPI_Cart_sub(MPI_Comm arg_0, int *arg_1, MPI_Comm *arg_2);
////_EXTERN_C_ int MPI_Cart_sub(MPI_Comm arg_0, int *arg_1, MPI_Comm *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Cart_sub(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Cartdim_get ================== */
////_EXTERN_C_ int PMPI_Cartdim_get(MPI_Comm arg_0, int *arg_1);
////_EXTERN_C_ int MPI_Cartdim_get(MPI_Comm arg_0, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Cartdim_get(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Comm_compare ================== */
////_EXTERN_C_ int PMPI_Comm_compare(MPI_Comm arg_0, MPI_Comm arg_1, int *arg_2);
////_EXTERN_C_ int MPI_Comm_compare(MPI_Comm arg_0, MPI_Comm arg_1, int *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Comm_compare(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Comm_create ================== */
////_EXTERN_C_ int PMPI_Comm_create(MPI_Comm arg_0, MPI_Group arg_1, MPI_Comm *arg_2);
////_EXTERN_C_ int MPI_Comm_create(MPI_Comm arg_0, MPI_Group arg_1, MPI_Comm *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Comm_create(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Comm_dup ================== */
////_EXTERN_C_ int PMPI_Comm_dup(MPI_Comm arg_0, MPI_Comm *arg_1);
////_EXTERN_C_ int MPI_Comm_dup(MPI_Comm arg_0, MPI_Comm *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Comm_dup(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Comm_free ================== */
////_EXTERN_C_ int PMPI_Comm_free(MPI_Comm *arg_0);
////_EXTERN_C_ int MPI_Comm_free(MPI_Comm *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Comm_free(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Comm_get_name ================== */
////_EXTERN_C_ int PMPI_Comm_get_name(MPI_Comm arg_0, char *arg_1, int *arg_2);
////_EXTERN_C_ int MPI_Comm_get_name(MPI_Comm arg_0, char *arg_1, int *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Comm_get_name(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Comm_group ================== */
////_EXTERN_C_ int PMPI_Comm_group(MPI_Comm arg_0, MPI_Group *arg_1);
////_EXTERN_C_ int MPI_Comm_group(MPI_Comm arg_0, MPI_Group *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Comm_group(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Comm_remote_group ================== */
////_EXTERN_C_ int PMPI_Comm_remote_group(MPI_Comm arg_0, MPI_Group *arg_1);
////_EXTERN_C_ int MPI_Comm_remote_group(MPI_Comm arg_0, MPI_Group *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Comm_remote_group(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Comm_remote_size ================== */
////_EXTERN_C_ int PMPI_Comm_remote_size(MPI_Comm arg_0, int *arg_1);
////_EXTERN_C_ int MPI_Comm_remote_size(MPI_Comm arg_0, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Comm_remote_size(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Comm_set_name ================== */
////_EXTERN_C_ int PMPI_Comm_set_name(MPI_Comm arg_0, char *arg_1);
////_EXTERN_C_ int MPI_Comm_set_name(MPI_Comm arg_0, char *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Comm_set_name(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Comm_split ================== */
////_EXTERN_C_ int PMPI_Comm_split(MPI_Comm arg_0, int arg_1, int arg_2, MPI_Comm *arg_3);
////_EXTERN_C_ int MPI_Comm_split(MPI_Comm arg_0, int arg_1, int arg_2, MPI_Comm *arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Comm_split(arg_0, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Comm_test_inter ================== */
////_EXTERN_C_ int PMPI_Comm_test_inter(MPI_Comm arg_0, int *arg_1);
////_EXTERN_C_ int MPI_Comm_test_inter(MPI_Comm arg_0, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Comm_test_inter(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Dims_create ================== */
////_EXTERN_C_ int PMPI_Dims_create(int arg_0, int arg_1, int *arg_2);
////_EXTERN_C_ int MPI_Dims_create(int arg_0, int arg_1, int *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Dims_create(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Errhandler_create ================== */
////_EXTERN_C_ int PMPI_Errhandler_create(MPI_Handler_function *arg_0, MPI_Errhandler *arg_1);
////_EXTERN_C_ int MPI_Errhandler_create(MPI_Handler_function *arg_0, MPI_Errhandler *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Errhandler_create(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Errhandler_free ================== */
////_EXTERN_C_ int PMPI_Errhandler_free(MPI_Errhandler *arg_0);
////_EXTERN_C_ int MPI_Errhandler_free(MPI_Errhandler *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Errhandler_free(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Errhandler_get ================== */
////_EXTERN_C_ int PMPI_Errhandler_get(MPI_Comm arg_0, MPI_Errhandler *arg_1);
////_EXTERN_C_ int MPI_Errhandler_get(MPI_Comm arg_0, MPI_Errhandler *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Errhandler_get(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Errhandler_set ================== */
////_EXTERN_C_ int PMPI_Errhandler_set(MPI_Comm arg_0, MPI_Errhandler arg_1);
////_EXTERN_C_ int MPI_Errhandler_set(MPI_Comm arg_0, MPI_Errhandler arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Errhandler_set(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Error_class ================== */
////_EXTERN_C_ int PMPI_Error_class(int arg_0, int *arg_1);
////_EXTERN_C_ int MPI_Error_class(int arg_0, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Error_class(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Error_string ================== */
////_EXTERN_C_ int PMPI_Error_string(int arg_0, char *arg_1, int *arg_2);
////_EXTERN_C_ int MPI_Error_string(int arg_0, char *arg_1, int *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Error_string(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_close ================== */
////_EXTERN_C_ int PMPI_File_close(MPI_File *arg_0);
////_EXTERN_C_ int MPI_File_close(MPI_File *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_close(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_delete ================== */
////_EXTERN_C_ int PMPI_File_delete(char *arg_0, MPI_Info arg_1);
////_EXTERN_C_ int MPI_File_delete(char *arg_0, MPI_Info arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_delete(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_get_amode ================== */
////_EXTERN_C_ int PMPI_File_get_amode(MPI_File arg_0, int *arg_1);
////_EXTERN_C_ int MPI_File_get_amode(MPI_File arg_0, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_get_amode(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_get_atomicity ================== */
////_EXTERN_C_ int PMPI_File_get_atomicity(MPI_File arg_0, int *arg_1);
////_EXTERN_C_ int MPI_File_get_atomicity(MPI_File arg_0, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_get_atomicity(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_get_byte_offset ================== */
////_EXTERN_C_ int PMPI_File_get_byte_offset(MPI_File arg_0, MPI_Offset arg_1, MPI_Offset *arg_2);
////_EXTERN_C_ int MPI_File_get_byte_offset(MPI_File arg_0, MPI_Offset arg_1, MPI_Offset *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_get_byte_offset(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_get_errhandler ================== */
////_EXTERN_C_ int PMPI_File_get_errhandler(MPI_File arg_0, MPI_Errhandler *arg_1);
////_EXTERN_C_ int MPI_File_get_errhandler(MPI_File arg_0, MPI_Errhandler *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_get_errhandler(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_get_group ================== */
////_EXTERN_C_ int PMPI_File_get_group(MPI_File arg_0, MPI_Group *arg_1);
////_EXTERN_C_ int MPI_File_get_group(MPI_File arg_0, MPI_Group *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_get_group(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_get_info ================== */
////_EXTERN_C_ int PMPI_File_get_info(MPI_File arg_0, MPI_Info *arg_1);
////_EXTERN_C_ int MPI_File_get_info(MPI_File arg_0, MPI_Info *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_get_info(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_get_position ================== */
////_EXTERN_C_ int PMPI_File_get_position(MPI_File arg_0, MPI_Offset *arg_1);
////_EXTERN_C_ int MPI_File_get_position(MPI_File arg_0, MPI_Offset *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_get_position(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_get_position_shared ================== */
////_EXTERN_C_ int PMPI_File_get_position_shared(MPI_File arg_0, MPI_Offset *arg_1);
////_EXTERN_C_ int MPI_File_get_position_shared(MPI_File arg_0, MPI_Offset *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_get_position_shared(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_get_size ================== */
////_EXTERN_C_ int PMPI_File_get_size(MPI_File arg_0, MPI_Offset *arg_1);
////_EXTERN_C_ int MPI_File_get_size(MPI_File arg_0, MPI_Offset *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_get_size(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_get_type_extent ================== */
////_EXTERN_C_ int PMPI_File_get_type_extent(MPI_File arg_0, MPI_Datatype arg_1, MPI_Aint *arg_2);
////_EXTERN_C_ int MPI_File_get_type_extent(MPI_File arg_0, MPI_Datatype arg_1, MPI_Aint *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_get_type_extent(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_get_view ================== */
////_EXTERN_C_ int PMPI_File_get_view(MPI_File arg_0, MPI_Offset *arg_1, MPI_Datatype *arg_2, MPI_Datatype *arg_3, char *arg_4);
////_EXTERN_C_ int MPI_File_get_view(MPI_File arg_0, MPI_Offset *arg_1, MPI_Datatype *arg_2, MPI_Datatype *arg_3, char *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_get_view(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_iread ================== */
////_EXTERN_C_ int PMPI_File_iread(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPIO_Request *arg_4);
////_EXTERN_C_ int MPI_File_iread(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPIO_Request *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_iread(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_iread_at ================== */
////_EXTERN_C_ int PMPI_File_iread_at(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4, MPIO_Request *arg_5);
////_EXTERN_C_ int MPI_File_iread_at(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4, MPIO_Request *arg_5) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_iread_at(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_iread_shared ================== */
////_EXTERN_C_ int PMPI_File_iread_shared(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPIO_Request *arg_4);
////_EXTERN_C_ int MPI_File_iread_shared(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPIO_Request *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_iread_shared(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_iwrite ================== */
////_EXTERN_C_ int PMPI_File_iwrite(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPIO_Request *arg_4);
////_EXTERN_C_ int MPI_File_iwrite(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPIO_Request *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_iwrite(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_iwrite_at ================== */
////_EXTERN_C_ int PMPI_File_iwrite_at(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4, MPIO_Request *arg_5);
////_EXTERN_C_ int MPI_File_iwrite_at(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4, MPIO_Request *arg_5) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_iwrite_at(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_iwrite_shared ================== */
////_EXTERN_C_ int PMPI_File_iwrite_shared(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPIO_Request *arg_4);
////_EXTERN_C_ int MPI_File_iwrite_shared(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPIO_Request *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_iwrite_shared(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_open ================== */
////_EXTERN_C_ int PMPI_File_open(MPI_Comm arg_0, char *arg_1, int arg_2, MPI_Info arg_3, MPI_File *arg_4);
////_EXTERN_C_ int MPI_File_open(MPI_Comm arg_0, char *arg_1, int arg_2, MPI_Info arg_3, MPI_File *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_open(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_preallocate ================== */
////_EXTERN_C_ int PMPI_File_preallocate(MPI_File arg_0, MPI_Offset arg_1);
////_EXTERN_C_ int MPI_File_preallocate(MPI_File arg_0, MPI_Offset arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_preallocate(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_read ================== */
////_EXTERN_C_ int PMPI_File_read(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4);
////_EXTERN_C_ int MPI_File_read(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_read(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_read_all ================== */
////_EXTERN_C_ int PMPI_File_read_all(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4);
////_EXTERN_C_ int MPI_File_read_all(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_read_all(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_read_all_begin ================== */
////_EXTERN_C_ int PMPI_File_read_all_begin(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3);
////_EXTERN_C_ int MPI_File_read_all_begin(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_read_all_begin(arg_0, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_read_all_end ================== */
////_EXTERN_C_ int PMPI_File_read_all_end(MPI_File arg_0, void *arg_1, MPI_Status *arg_2);
////_EXTERN_C_ int MPI_File_read_all_end(MPI_File arg_0, void *arg_1, MPI_Status *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_read_all_end(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_read_at ================== */
////_EXTERN_C_ int PMPI_File_read_at(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4, MPI_Status *arg_5);
////_EXTERN_C_ int MPI_File_read_at(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4, MPI_Status *arg_5) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_read_at(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_read_at_all ================== */
////_EXTERN_C_ int PMPI_File_read_at_all(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4, MPI_Status *arg_5);
////_EXTERN_C_ int MPI_File_read_at_all(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4, MPI_Status *arg_5) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_read_at_all(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_read_at_all_begin ================== */
////_EXTERN_C_ int PMPI_File_read_at_all_begin(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4);
////_EXTERN_C_ int MPI_File_read_at_all_begin(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_read_at_all_begin(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_read_at_all_end ================== */
////_EXTERN_C_ int PMPI_File_read_at_all_end(MPI_File arg_0, void *arg_1, MPI_Status *arg_2);
////_EXTERN_C_ int MPI_File_read_at_all_end(MPI_File arg_0, void *arg_1, MPI_Status *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_read_at_all_end(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_read_ordered ================== */
////_EXTERN_C_ int PMPI_File_read_ordered(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4);
////_EXTERN_C_ int MPI_File_read_ordered(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_read_ordered(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_read_ordered_begin ================== */
////_EXTERN_C_ int PMPI_File_read_ordered_begin(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3);
////_EXTERN_C_ int MPI_File_read_ordered_begin(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_read_ordered_begin(arg_0, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_read_ordered_end ================== */
////_EXTERN_C_ int PMPI_File_read_ordered_end(MPI_File arg_0, void *arg_1, MPI_Status *arg_2);
////_EXTERN_C_ int MPI_File_read_ordered_end(MPI_File arg_0, void *arg_1, MPI_Status *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_read_ordered_end(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_read_shared ================== */
////_EXTERN_C_ int PMPI_File_read_shared(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4);
////_EXTERN_C_ int MPI_File_read_shared(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_read_shared(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_seek ================== */
////_EXTERN_C_ int PMPI_File_seek(MPI_File arg_0, MPI_Offset arg_1, int arg_2);
////_EXTERN_C_ int MPI_File_seek(MPI_File arg_0, MPI_Offset arg_1, int arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_seek(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_seek_shared ================== */
////_EXTERN_C_ int PMPI_File_seek_shared(MPI_File arg_0, MPI_Offset arg_1, int arg_2);
////_EXTERN_C_ int MPI_File_seek_shared(MPI_File arg_0, MPI_Offset arg_1, int arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_seek_shared(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_set_atomicity ================== */
////_EXTERN_C_ int PMPI_File_set_atomicity(MPI_File arg_0, int arg_1);
////_EXTERN_C_ int MPI_File_set_atomicity(MPI_File arg_0, int arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_set_atomicity(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_set_errhandler ================== */
////_EXTERN_C_ int PMPI_File_set_errhandler(MPI_File arg_0, MPI_Errhandler arg_1);
////_EXTERN_C_ int MPI_File_set_errhandler(MPI_File arg_0, MPI_Errhandler arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_set_errhandler(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_set_info ================== */
////_EXTERN_C_ int PMPI_File_set_info(MPI_File arg_0, MPI_Info arg_1);
////_EXTERN_C_ int MPI_File_set_info(MPI_File arg_0, MPI_Info arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_set_info(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_set_size ================== */
////_EXTERN_C_ int PMPI_File_set_size(MPI_File arg_0, MPI_Offset arg_1);
////_EXTERN_C_ int MPI_File_set_size(MPI_File arg_0, MPI_Offset arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_set_size(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_set_view ================== */
////_EXTERN_C_ int PMPI_File_set_view(MPI_File arg_0, MPI_Offset arg_1, MPI_Datatype arg_2, MPI_Datatype arg_3, char *arg_4, MPI_Info arg_5);
////_EXTERN_C_ int MPI_File_set_view(MPI_File arg_0, MPI_Offset arg_1, MPI_Datatype arg_2, MPI_Datatype arg_3, char *arg_4, MPI_Info arg_5) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_set_view(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_sync ================== */
////_EXTERN_C_ int PMPI_File_sync(MPI_File arg_0);
////_EXTERN_C_ int MPI_File_sync(MPI_File arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_sync(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_write ================== */
////_EXTERN_C_ int PMPI_File_write(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4);
////_EXTERN_C_ int MPI_File_write(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_write(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_write_all ================== */
////_EXTERN_C_ int PMPI_File_write_all(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4);
////_EXTERN_C_ int MPI_File_write_all(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_write_all(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_write_all_begin ================== */
////_EXTERN_C_ int PMPI_File_write_all_begin(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3);
////_EXTERN_C_ int MPI_File_write_all_begin(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_write_all_begin(arg_0, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_write_all_end ================== */
////_EXTERN_C_ int PMPI_File_write_all_end(MPI_File arg_0, void *arg_1, MPI_Status *arg_2);
////_EXTERN_C_ int MPI_File_write_all_end(MPI_File arg_0, void *arg_1, MPI_Status *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_write_all_end(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_write_at ================== */
////_EXTERN_C_ int PMPI_File_write_at(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4, MPI_Status *arg_5);
////_EXTERN_C_ int MPI_File_write_at(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4, MPI_Status *arg_5) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_write_at(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_write_at_all ================== */
////_EXTERN_C_ int PMPI_File_write_at_all(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4, MPI_Status *arg_5);
////_EXTERN_C_ int MPI_File_write_at_all(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4, MPI_Status *arg_5) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_write_at_all(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_write_at_all_begin ================== */
////_EXTERN_C_ int PMPI_File_write_at_all_begin(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4);
////_EXTERN_C_ int MPI_File_write_at_all_begin(MPI_File arg_0, MPI_Offset arg_1, void *arg_2, int arg_3, MPI_Datatype arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_write_at_all_begin(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_write_at_all_end ================== */
////_EXTERN_C_ int PMPI_File_write_at_all_end(MPI_File arg_0, void *arg_1, MPI_Status *arg_2);
////_EXTERN_C_ int MPI_File_write_at_all_end(MPI_File arg_0, void *arg_1, MPI_Status *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_write_at_all_end(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_write_ordered ================== */
////_EXTERN_C_ int PMPI_File_write_ordered(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4);
////_EXTERN_C_ int MPI_File_write_ordered(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_write_ordered(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_write_ordered_begin ================== */
////_EXTERN_C_ int PMPI_File_write_ordered_begin(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3);
////_EXTERN_C_ int MPI_File_write_ordered_begin(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_write_ordered_begin(arg_0, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_write_ordered_end ================== */
////_EXTERN_C_ int PMPI_File_write_ordered_end(MPI_File arg_0, void *arg_1, MPI_Status *arg_2);
////_EXTERN_C_ int MPI_File_write_ordered_end(MPI_File arg_0, void *arg_1, MPI_Status *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_write_ordered_end(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_File_write_shared ================== */
////_EXTERN_C_ int PMPI_File_write_shared(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4);
////_EXTERN_C_ int MPI_File_write_shared(MPI_File arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Status *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_File_write_shared(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
//
/////* ================== C Wrappers for MPI_Finalized ================== */
////_EXTERN_C_ int PMPI_Finalized(int *arg_0);
////_EXTERN_C_ int MPI_Finalized(int *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Finalized(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Gather ================== */
////_EXTERN_C_ int PMPI_Gather(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7);
////_EXTERN_C_ int MPI_Gather(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Gather(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Gatherv ================== */
////_EXTERN_C_ int PMPI_Gatherv(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int *arg_4, int *arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8);
////_EXTERN_C_ int MPI_Gatherv(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int *arg_4, int *arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Gatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Get_count ================== */
////_EXTERN_C_ int PMPI_Get_count(MPI_Status *arg_0, MPI_Datatype arg_1, int *arg_2);
////_EXTERN_C_ int MPI_Get_count(MPI_Status *arg_0, MPI_Datatype arg_1, int *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Get_count(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Get_elements ================== */
////_EXTERN_C_ int PMPI_Get_elements(MPI_Status *arg_0, MPI_Datatype arg_1, int *arg_2);
////_EXTERN_C_ int MPI_Get_elements(MPI_Status *arg_0, MPI_Datatype arg_1, int *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Get_elements(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Get_processor_name ================== */
////_EXTERN_C_ int PMPI_Get_processor_name(char *arg_0, int *arg_1);
////_EXTERN_C_ int MPI_Get_processor_name(char *arg_0, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Get_processor_name(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Get_version ================== */
////_EXTERN_C_ int PMPI_Get_version(int *arg_0, int *arg_1);
////_EXTERN_C_ int MPI_Get_version(int *arg_0, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Get_version(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Graph_create ================== */
////_EXTERN_C_ int PMPI_Graph_create(MPI_Comm arg_0, int arg_1, int *arg_2, int *arg_3, int arg_4, MPI_Comm *arg_5);
////_EXTERN_C_ int MPI_Graph_create(MPI_Comm arg_0, int arg_1, int *arg_2, int *arg_3, int arg_4, MPI_Comm *arg_5) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Graph_create(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Graph_get ================== */
////_EXTERN_C_ int PMPI_Graph_get(MPI_Comm arg_0, int arg_1, int arg_2, int *arg_3, int *arg_4);
////_EXTERN_C_ int MPI_Graph_get(MPI_Comm arg_0, int arg_1, int arg_2, int *arg_3, int *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Graph_get(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Graph_map ================== */
////_EXTERN_C_ int PMPI_Graph_map(MPI_Comm arg_0, int arg_1, int *arg_2, int *arg_3, int *arg_4);
////_EXTERN_C_ int MPI_Graph_map(MPI_Comm arg_0, int arg_1, int *arg_2, int *arg_3, int *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Graph_map(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Graph_neighbors ================== */
////_EXTERN_C_ int PMPI_Graph_neighbors(MPI_Comm arg_0, int arg_1, int arg_2, int *arg_3);
////_EXTERN_C_ int MPI_Graph_neighbors(MPI_Comm arg_0, int arg_1, int arg_2, int *arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Graph_neighbors(arg_0, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Graph_neighbors_count ================== */
////_EXTERN_C_ int PMPI_Graph_neighbors_count(MPI_Comm arg_0, int arg_1, int *arg_2);
////_EXTERN_C_ int MPI_Graph_neighbors_count(MPI_Comm arg_0, int arg_1, int *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Graph_neighbors_count(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Graphdims_get ================== */
////_EXTERN_C_ int PMPI_Graphdims_get(MPI_Comm arg_0, int *arg_1, int *arg_2);
////_EXTERN_C_ int MPI_Graphdims_get(MPI_Comm arg_0, int *arg_1, int *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Graphdims_get(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Group_compare ================== */
////_EXTERN_C_ int PMPI_Group_compare(MPI_Group arg_0, MPI_Group arg_1, int *arg_2);
////_EXTERN_C_ int MPI_Group_compare(MPI_Group arg_0, MPI_Group arg_1, int *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Group_compare(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Group_difference ================== */
////_EXTERN_C_ int PMPI_Group_difference(MPI_Group arg_0, MPI_Group arg_1, MPI_Group *arg_2);
////_EXTERN_C_ int MPI_Group_difference(MPI_Group arg_0, MPI_Group arg_1, MPI_Group *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Group_difference(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Group_excl ================== */
////_EXTERN_C_ int PMPI_Group_excl(MPI_Group group, int arg_1, int *arg_2, MPI_Group *arg_3);
////_EXTERN_C_ int MPI_Group_excl(MPI_Group group, int arg_1, int *arg_2, MPI_Group *arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Group_excl(group, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Group_free ================== */
////_EXTERN_C_ int PMPI_Group_free(MPI_Group *arg_0);
////_EXTERN_C_ int MPI_Group_free(MPI_Group *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Group_free(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Group_incl ================== */
////_EXTERN_C_ int PMPI_Group_incl(MPI_Group group, int arg_1, int *arg_2, MPI_Group *arg_3);
////_EXTERN_C_ int MPI_Group_incl(MPI_Group group, int arg_1, int *arg_2, MPI_Group *arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Group_incl(group, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Group_intersection ================== */
////_EXTERN_C_ int PMPI_Group_intersection(MPI_Group arg_0, MPI_Group arg_1, MPI_Group *arg_2);
////_EXTERN_C_ int MPI_Group_intersection(MPI_Group arg_0, MPI_Group arg_1, MPI_Group *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Group_intersection(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Group_range_excl ================== */
////_EXTERN_C_ int PMPI_Group_range_excl(MPI_Group group, int arg_1, int arg_2[][3], MPI_Group *arg_3);
////_EXTERN_C_ int MPI_Group_range_excl(MPI_Group group, int arg_1, int arg_2[][3], MPI_Group *arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Group_range_excl(group, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Group_range_incl ================== */
////_EXTERN_C_ int PMPI_Group_range_incl(MPI_Group group, int arg_1, int arg_2[][3], MPI_Group *arg_3);
////_EXTERN_C_ int MPI_Group_range_incl(MPI_Group group, int arg_1, int arg_2[][3], MPI_Group *arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Group_range_incl(group, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Group_rank ================== */
////_EXTERN_C_ int PMPI_Group_rank(MPI_Group group, int *arg_1);
////_EXTERN_C_ int MPI_Group_rank(MPI_Group group, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Group_rank(group, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Group_size ================== */
////_EXTERN_C_ int PMPI_Group_size(MPI_Group group, int *arg_1);
////_EXTERN_C_ int MPI_Group_size(MPI_Group group, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Group_size(group, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Group_translate_ranks ================== */
////_EXTERN_C_ int PMPI_Group_translate_ranks(MPI_Group arg_0, int arg_1, int *arg_2, MPI_Group arg_3, int *arg_4);
////_EXTERN_C_ int MPI_Group_translate_ranks(MPI_Group arg_0, int arg_1, int *arg_2, MPI_Group arg_3, int *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Group_translate_ranks(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Group_union ================== */
////_EXTERN_C_ int PMPI_Group_union(MPI_Group arg_0, MPI_Group arg_1, MPI_Group *arg_2);
////_EXTERN_C_ int MPI_Group_union(MPI_Group arg_0, MPI_Group arg_1, MPI_Group *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Group_union(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
//
/////* ================== C Wrappers for MPI_Info_create ================== */
////_EXTERN_C_ int PMPI_Info_create(MPI_Info *arg_0);
////_EXTERN_C_ int MPI_Info_create(MPI_Info *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Info_create(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Info_delete ================== */
////_EXTERN_C_ int PMPI_Info_delete(MPI_Info arg_0, char *arg_1);
////_EXTERN_C_ int MPI_Info_delete(MPI_Info arg_0, char *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Info_delete(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Info_dup ================== */
////_EXTERN_C_ int PMPI_Info_dup(MPI_Info arg_0, MPI_Info *arg_1);
////_EXTERN_C_ int MPI_Info_dup(MPI_Info arg_0, MPI_Info *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Info_dup(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Info_free ================== */
////_EXTERN_C_ int PMPI_Info_free(MPI_Info *info);
////_EXTERN_C_ int MPI_Info_free(MPI_Info *info) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Info_free(info);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Info_get ================== */
////_EXTERN_C_ int PMPI_Info_get(MPI_Info arg_0, char *arg_1, int arg_2, char *arg_3, int *arg_4);
////_EXTERN_C_ int MPI_Info_get(MPI_Info arg_0, char *arg_1, int arg_2, char *arg_3, int *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Info_get(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Info_get_nkeys ================== */
////_EXTERN_C_ int PMPI_Info_get_nkeys(MPI_Info arg_0, int *arg_1);
////_EXTERN_C_ int MPI_Info_get_nkeys(MPI_Info arg_0, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Info_get_nkeys(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Info_get_nthkey ================== */
////_EXTERN_C_ int PMPI_Info_get_nthkey(MPI_Info arg_0, int arg_1, char *arg_2);
////_EXTERN_C_ int MPI_Info_get_nthkey(MPI_Info arg_0, int arg_1, char *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Info_get_nthkey(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Info_get_valuelen ================== */
////_EXTERN_C_ int PMPI_Info_get_valuelen(MPI_Info arg_0, char *arg_1, int *arg_2, int *arg_3);
////_EXTERN_C_ int MPI_Info_get_valuelen(MPI_Info arg_0, char *arg_1, int *arg_2, int *arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Info_get_valuelen(arg_0, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Info_set ================== */
////_EXTERN_C_ int PMPI_Info_set(MPI_Info arg_0, char *arg_1, char *arg_2);
////_EXTERN_C_ int MPI_Info_set(MPI_Info arg_0, char *arg_1, char *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Info_set(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
//
//
/////* ================== C Wrappers for MPI_Initialized ================== */
////_EXTERN_C_ int PMPI_Initialized(int *arg_0);
////_EXTERN_C_ int MPI_Initialized(int *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Initialized(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Intercomm_create ================== */
////_EXTERN_C_ int PMPI_Intercomm_create(MPI_Comm arg_0, int arg_1, MPI_Comm arg_2, int arg_3, int arg_4, MPI_Comm *arg_5);
////_EXTERN_C_ int MPI_Intercomm_create(MPI_Comm arg_0, int arg_1, MPI_Comm arg_2, int arg_3, int arg_4, MPI_Comm *arg_5) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Intercomm_create(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Intercomm_merge ================== */
////_EXTERN_C_ int PMPI_Intercomm_merge(MPI_Comm arg_0, int arg_1, MPI_Comm *arg_2);
////_EXTERN_C_ int MPI_Intercomm_merge(MPI_Comm arg_0, int arg_1, MPI_Comm *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Intercomm_merge(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Keyval_create ================== */
////_EXTERN_C_ int PMPI_Keyval_create(MPI_Copy_function *arg_0, MPI_Delete_function *arg_1, int *arg_2, void *arg_3);
////_EXTERN_C_ int MPI_Keyval_create(MPI_Copy_function *arg_0, MPI_Delete_function *arg_1, int *arg_2, void *arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Keyval_create(arg_0, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Keyval_free ================== */
////_EXTERN_C_ int PMPI_Keyval_free(int *arg_0);
////_EXTERN_C_ int MPI_Keyval_free(int *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Keyval_free(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Op_create ================== */
////_EXTERN_C_ int PMPI_Op_create(MPI_User_function *arg_0, int arg_1, MPI_Op *arg_2);
////_EXTERN_C_ int MPI_Op_create(MPI_User_function *arg_0, int arg_1, MPI_Op *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Op_create(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Op_free ================== */
////_EXTERN_C_ int PMPI_Op_free(MPI_Op *arg_0);
////_EXTERN_C_ int MPI_Op_free(MPI_Op *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Op_free(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Pack ================== */
////_EXTERN_C_ int PMPI_Pack(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, int *arg_5, MPI_Comm arg_6);
////_EXTERN_C_ int MPI_Pack(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, int *arg_5, MPI_Comm arg_6) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Pack(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Pack_size ================== */
////_EXTERN_C_ int PMPI_Pack_size(int arg_0, MPI_Datatype arg_1, MPI_Comm arg_2, int *arg_3);
////_EXTERN_C_ int MPI_Pack_size(int arg_0, MPI_Datatype arg_1, MPI_Comm arg_2, int *arg_3) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Pack_size(arg_0, arg_1, arg_2, arg_3);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Pcontrol ================== */
////_EXTERN_C_ int PMPI_Pcontrol(const int arg_0, ...);
////_EXTERN_C_ int MPI_Pcontrol(const int arg_0, ...) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Pcontrol(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Recv_init ================== */
////_EXTERN_C_ int PMPI_Recv_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
////_EXTERN_C_ int MPI_Recv_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Recv_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Reduce ================== */
////_EXTERN_C_ int PMPI_Reduce(void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, int arg_5, MPI_Comm arg_6);
////_EXTERN_C_ int MPI_Reduce(void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, int arg_5, MPI_Comm arg_6) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Reduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Reduce_scatter ================== */
////_EXTERN_C_ int PMPI_Reduce_scatter(void *arg_0, void *arg_1, int *arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5);
////_EXTERN_C_ int MPI_Reduce_scatter(void *arg_0, void *arg_1, int *arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Reduce_scatter(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Register_datarep ================== */
////_EXTERN_C_ int PMPI_Register_datarep(char *arg_0, MPI_Datarep_conversion_function *arg_1, MPI_Datarep_conversion_function *arg_2, MPI_Datarep_extent_function *arg_3, void *arg_4);
////_EXTERN_C_ int MPI_Register_datarep(char *arg_0, MPI_Datarep_conversion_function *arg_1, MPI_Datarep_conversion_function *arg_2, MPI_Datarep_extent_function *arg_3, void *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Register_datarep(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Request_free ================== */
////_EXTERN_C_ int PMPI_Request_free(MPI_Request *arg_0);
////_EXTERN_C_ int MPI_Request_free(MPI_Request *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Request_free(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
//
/////* ================== C Wrappers for MPI_Rsend_init ================== */
////_EXTERN_C_ int PMPI_Rsend_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
////_EXTERN_C_ int MPI_Rsend_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Rsend_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Scan ================== */
////_EXTERN_C_ int PMPI_Scan(void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5);
////_EXTERN_C_ int MPI_Scan(void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Scan(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Scatter ================== */
////_EXTERN_C_ int PMPI_Scatter(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7);
////_EXTERN_C_ int MPI_Scatter(void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Scatter(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Scatterv ================== */
////_EXTERN_C_ int PMPI_Scatterv(void *arg_0, int *arg_1, int *arg_2, MPI_Datatype arg_3, void *arg_4, int arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8);
////_EXTERN_C_ int MPI_Scatterv(void *arg_0, int *arg_1, int *arg_2, MPI_Datatype arg_3, void *arg_4, int arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Scatterv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
////}
////    return _wrap_py_return_val;
////}
////
//
/////* ================== C Wrappers for MPI_Send_init ================== */
////_EXTERN_C_ int PMPI_Send_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
////_EXTERN_C_ int MPI_Send_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Send_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Sendrecv ================== */
////_EXTERN_C_ int PMPI_Sendrecv(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, void *arg_5, int arg_6, MPI_Datatype arg_7, int arg_8, int arg_9, MPI_Comm arg_10, MPI_Status *arg_11);
////_EXTERN_C_ int MPI_Sendrecv(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, void *arg_5, int arg_6, MPI_Datatype arg_7, int arg_8, int arg_9, MPI_Comm arg_10, MPI_Status *arg_11) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Sendrecv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8, arg_9, arg_10, arg_11);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Sendrecv_replace ================== */
////_EXTERN_C_ int PMPI_Sendrecv_replace(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, int arg_5, int arg_6, MPI_Comm arg_7, MPI_Status *arg_8);
////_EXTERN_C_ int MPI_Sendrecv_replace(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, int arg_5, int arg_6, MPI_Comm arg_7, MPI_Status *arg_8) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Sendrecv_replace(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
////}
////    return _wrap_py_return_val;
////}
////
//
/////* ================== C Wrappers for MPI_Ssend_init ================== */
////_EXTERN_C_ int PMPI_Ssend_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6);
////_EXTERN_C_ int MPI_Ssend_init(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, int arg_4, MPI_Comm arg_5, MPI_Request *arg_6) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Ssend_init(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Start ================== */
////_EXTERN_C_ int PMPI_Start(MPI_Request *arg_0);
////_EXTERN_C_ int MPI_Start(MPI_Request *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Start(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Startall ================== */
////_EXTERN_C_ int PMPI_Startall(int arg_0, MPI_Request *arg_1);
////_EXTERN_C_ int MPI_Startall(int arg_0, MPI_Request *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Startall(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Status_set_cancelled ================== */
////_EXTERN_C_ int PMPI_Status_set_cancelled(MPI_Status *arg_0, int arg_1);
////_EXTERN_C_ int MPI_Status_set_cancelled(MPI_Status *arg_0, int arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Status_set_cancelled(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Status_set_elements ================== */
////_EXTERN_C_ int PMPI_Status_set_elements(MPI_Status *arg_0, MPI_Datatype arg_1, int arg_2);
////_EXTERN_C_ int MPI_Status_set_elements(MPI_Status *arg_0, MPI_Datatype arg_1, int arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Status_set_elements(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
//
/////* ================== C Wrappers for MPI_Test_cancelled ================== */
////_EXTERN_C_ int PMPI_Test_cancelled(MPI_Status *arg_0, int *arg_1);
////_EXTERN_C_ int MPI_Test_cancelled(MPI_Status *arg_0, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Test_cancelled(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
//
/////* ================== C Wrappers for MPI_Topo_test ================== */
////_EXTERN_C_ int PMPI_Topo_test(MPI_Comm arg_0, int *arg_1);
////_EXTERN_C_ int MPI_Topo_test(MPI_Comm arg_0, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Topo_test(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_commit ================== */
////_EXTERN_C_ int PMPI_Type_commit(MPI_Datatype *arg_0);
////_EXTERN_C_ int MPI_Type_commit(MPI_Datatype *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_commit(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_contiguous ================== */
////_EXTERN_C_ int PMPI_Type_contiguous(int arg_0, MPI_Datatype arg_1, MPI_Datatype *arg_2);
////_EXTERN_C_ int MPI_Type_contiguous(int arg_0, MPI_Datatype arg_1, MPI_Datatype *arg_2) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_contiguous(arg_0, arg_1, arg_2);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_create_darray ================== */
////_EXTERN_C_ int PMPI_Type_create_darray(int arg_0, int arg_1, int arg_2, int *arg_3, int *arg_4, int *arg_5, int *arg_6, int arg_7, MPI_Datatype arg_8, MPI_Datatype *arg_9);
////_EXTERN_C_ int MPI_Type_create_darray(int arg_0, int arg_1, int arg_2, int *arg_3, int *arg_4, int *arg_5, int *arg_6, int arg_7, MPI_Datatype arg_8, MPI_Datatype *arg_9) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_create_darray(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8, arg_9);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_create_indexed_block ================== */
////_EXTERN_C_ int PMPI_Type_create_indexed_block(int arg_0, int arg_1, int *arg_2, MPI_Datatype arg_3, MPI_Datatype *arg_4);
////_EXTERN_C_ int MPI_Type_create_indexed_block(int arg_0, int arg_1, int *arg_2, MPI_Datatype arg_3, MPI_Datatype *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_create_indexed_block(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_create_subarray ================== */
////_EXTERN_C_ int PMPI_Type_create_subarray(int arg_0, int *arg_1, int *arg_2, int *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Datatype *arg_6);
////_EXTERN_C_ int MPI_Type_create_subarray(int arg_0, int *arg_1, int *arg_2, int *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Datatype *arg_6) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_create_subarray(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_extent ================== */
////_EXTERN_C_ int PMPI_Type_extent(MPI_Datatype arg_0, MPI_Aint *arg_1);
////_EXTERN_C_ int MPI_Type_extent(MPI_Datatype arg_0, MPI_Aint *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_extent(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_free ================== */
////_EXTERN_C_ int PMPI_Type_free(MPI_Datatype *arg_0);
////_EXTERN_C_ int MPI_Type_free(MPI_Datatype *arg_0) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_free(arg_0);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_get_contents ================== */
////_EXTERN_C_ int PMPI_Type_get_contents(MPI_Datatype arg_0, int arg_1, int arg_2, int arg_3, int *arg_4, MPI_Aint *arg_5, MPI_Datatype *arg_6);
////_EXTERN_C_ int MPI_Type_get_contents(MPI_Datatype arg_0, int arg_1, int arg_2, int arg_3, int *arg_4, MPI_Aint *arg_5, MPI_Datatype *arg_6) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_get_contents(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_get_envelope ================== */
////_EXTERN_C_ int PMPI_Type_get_envelope(MPI_Datatype arg_0, int *arg_1, int *arg_2, int *arg_3, int *arg_4);
////_EXTERN_C_ int MPI_Type_get_envelope(MPI_Datatype arg_0, int *arg_1, int *arg_2, int *arg_3, int *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_get_envelope(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_hindexed ================== */
////_EXTERN_C_ int PMPI_Type_hindexed(int arg_0, int *arg_1, MPI_Aint *arg_2, MPI_Datatype arg_3, MPI_Datatype *arg_4);
////_EXTERN_C_ int MPI_Type_hindexed(int arg_0, int *arg_1, MPI_Aint *arg_2, MPI_Datatype arg_3, MPI_Datatype *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_hindexed(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_hvector ================== */
////_EXTERN_C_ int PMPI_Type_hvector(int arg_0, int arg_1, MPI_Aint arg_2, MPI_Datatype arg_3, MPI_Datatype *arg_4);
////_EXTERN_C_ int MPI_Type_hvector(int arg_0, int arg_1, MPI_Aint arg_2, MPI_Datatype arg_3, MPI_Datatype *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_hvector(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_indexed ================== */
////_EXTERN_C_ int PMPI_Type_indexed(int arg_0, int *arg_1, int *arg_2, MPI_Datatype arg_3, MPI_Datatype *arg_4);
////_EXTERN_C_ int MPI_Type_indexed(int arg_0, int *arg_1, int *arg_2, MPI_Datatype arg_3, MPI_Datatype *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_indexed(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_lb ================== */
////_EXTERN_C_ int PMPI_Type_lb(MPI_Datatype arg_0, MPI_Aint *arg_1);
////_EXTERN_C_ int MPI_Type_lb(MPI_Datatype arg_0, MPI_Aint *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_lb(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_size ================== */
////_EXTERN_C_ int PMPI_Type_size(MPI_Datatype arg_0, int *arg_1);
////_EXTERN_C_ int MPI_Type_size(MPI_Datatype arg_0, int *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_size(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_struct ================== */
////_EXTERN_C_ int PMPI_Type_struct(int arg_0, int *arg_1, MPI_Aint *arg_2, MPI_Datatype *arg_3, MPI_Datatype *arg_4);
////_EXTERN_C_ int MPI_Type_struct(int arg_0, int *arg_1, MPI_Aint *arg_2, MPI_Datatype *arg_3, MPI_Datatype *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_struct(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_ub ================== */
////_EXTERN_C_ int PMPI_Type_ub(MPI_Datatype arg_0, MPI_Aint *arg_1);
////_EXTERN_C_ int MPI_Type_ub(MPI_Datatype arg_0, MPI_Aint *arg_1) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_ub(arg_0, arg_1);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Type_vector ================== */
////_EXTERN_C_ int PMPI_Type_vector(int arg_0, int arg_1, int arg_2, MPI_Datatype arg_3, MPI_Datatype *arg_4);
////_EXTERN_C_ int MPI_Type_vector(int arg_0, int arg_1, int arg_2, MPI_Datatype arg_3, MPI_Datatype *arg_4) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Type_vector(arg_0, arg_1, arg_2, arg_3, arg_4);
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Unpack ================== */
////_EXTERN_C_ int PMPI_Unpack(void *arg_0, int arg_1, int *arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6);
////_EXTERN_C_ int MPI_Unpack(void *arg_0, int arg_1, int *arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6) { 
////    int _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Unpack(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
////}
////    return _wrap_py_return_val;
////}
////
//
/////* ================== C Wrappers for MPI_Wtick ================== */
////_EXTERN_C_ double PMPI_Wtick();
////_EXTERN_C_ double MPI_Wtick() { 
////    double _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Wtick();
////}
////    return _wrap_py_return_val;
////}
////
/////* ================== C Wrappers for MPI_Wtime ================== */
////_EXTERN_C_ double PMPI_Wtime();
////_EXTERN_C_ double MPI_Wtime() { 
////    double _wrap_py_return_val = 0;
//// 
////{
////  _wrap_py_return_val = PMPI_Wtime();
////}
////    return _wrap_py_return_val;
////}
////
////
