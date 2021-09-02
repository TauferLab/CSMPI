# A PMPI module for tracing call-stacks

## Summary: 
CSMPI is a PMPI module for tracing call-stacks of MPI functions.
You may want to know which chains of function calls in your application end in 
calls to certain MPI functions; CSMPI can help you find out.

## Installation:
* Run `one_step_build.sh`

## Usage:
* Link CSMPI with your MPI application explicitly, via `LD_PRELOAD`, or via a 
  "tool-stacking" layer like PnMPI (https://github.com/LLNL/PnMPI).
* Pick a configuration file from the `config` subdirectory or create one using
  `config/generate_config.py`.
* Set `CSMPI_CONFIG` environment variable to the path of your configuration file.
* When the application CSMPI is linked to calls `MPI_Finalize`, CSMPI will
  write out one trace file per MPI rank. 

## Configuration:
* CSMPI supports tracing callstacks with: 
  * libunwind 
  * glibc backtrace. 

## Tracing Overhead:
Callstack tracing can impose large runtime overheads. Hence you should:
* Use the glibc backtrace configuration rather than libunwind if possible
* Do not do in-place address translation or name-demangling without a good reason
* Set your tracing frequency carefully
* Avoid tracing as many MPI functions as possible 

## Dependencies:
* CMake
* JSON for Modern C++ (https://github.com/nlohmann/json)
* Boost (Serialization, MPI)
* libunwind (Optional)

## Copyright and License:

Copyright (c) 2021, Global Computing Lab

ANACIN-X is distributed under terms of the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0) with LLVM Exceptions.

See [LICENSE](https://github.com/TauferLab/Src_CSMPI/blob/master/LICENSE) for more details.
