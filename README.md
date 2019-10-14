# A PMPI module for tracing call-stacks

## Summary:
CSMPI is a PMPI module for tracing call-stacks of MPI functions. For instance, 
you may want to know which chains of function calls in your application end in
calls to certain MPI functions; CSMPI can help you find out!

## Usage:
* CSMPI functions more or less like other PMPI-based tools. Once built, you may either link it with your MPI application directly, interpose it with `LD_PRELOAD`, or use a "tool-stacking" layer like PnMPI (https://github.com/LLNL/PnMPI).
* You will however need to point CSMPI to a configuration file by setting the `CSMPI_CONFIG` environment variable. See the `config` subdirectory for example configuration files or read the `Configuration` section below.
* When the application CSMPI is linked to calls `MPI_Finalize`, CSMPI will write out one trace file per MPI rank. Depending on how you have set the tracing frequency, or if you are simply tracing a long-running execution, these trace files may be very large. If you are on a cluster or some other environment where you share a login node with other users, you will want to make sure the tracefiles **NOT** being written to, e.g., your home directory but rather to, e.g., your scratch space on a parallel filesystem. 

## Configuration:
* CSMPI supports tracing callstacks with either libunwind or with glibc's backtrace. 

## Tracing Overhead:
* A detailed study of tracing overhead is in progress, but for the meantime it suffices to say that callstack tracing is slow. Hence you should:
* Use the glibc backtrace configuration rather than libunwind if possible
* Set your tracing frequency carefully
* Avoid tracing as many MPI functions as possible (i.e., have an idea of what you're looking for before using this tool)

## Dependencies:
* JSON for Modern C++ (https://github.com/nlohmann/json)
* Boost (Serialization, MPI)
* pyelftools (https://github.com/eliben/pyelftools) (Optional, used for analysis scripts not CSMPI itself)
