#!/usr/bin/env python3

import argparse
import json

default_mpi_fns = [ "MPI_Send", 
                    "MPI_Bsend",
                    "MPI_Rsend",
                    "MPI_Ssend",
                    "MPI_Isend",
                    "MPI_Ibsend",
                    "MPI_Irsend",
                    "MPI_Issend",
                    "MPI_Recv",
                    "MPI_Wait",
                    "MPI_Waitany",
                    "MPI_Waitsome",
                    "MPI_Waitall",
                    "MPI_Test",
                    "MPI_Testany",
                    "MPI_Testsome",
                    "MPI_Testall",
                    "MPI_Probe",
                    "MPI_Iprobe" 
                  ]


def main( output, trace_dir, function_to_freq_file, trace_unmatched, translate_in_place, demangle_in_place ):
    # Grab default functions and frequencies if none are specified
    if function_to_freq_file == None:
        function_to_freq = { fn:0 for fn in default_mpi_fns }
    else:
        with open( function_to_freq_file, "r" ) as infile:
            function_to_freq = json.load( infile )
    # Build dict of configuration options
    config = {}
    config[ "mpi_functions" ] = []
    for fn,freq in function_to_freq.items():
        config[ "mpi_functions" ].append( { "name":fn, "freq":freq } )
    config["trace_dir"] = trace_dir
    config["trace_unmatched"] = trace_unmatched
    config["translate_in_place"] = translate_in_place
    config["demangle_in_place"] = demangle_in_place
    with open( output, "w" ) as outfile:
        json.dump( config, outfile, indent=4 )



if __name__ == "__main__":
    desc = "A script to generate a CSMPI configuration file"
    parser = argparse.ArgumentParser( description = desc )
    parser.add_argument("-o", "--output", required=True,
                        help="The path this configuration file will be written to")
    parser.add_argument("-f", "--functions", required=False, default=None,
                        help="A JSON file containing a list of MPI functions to trace and their associated tracing frequencies")
    parser.add_argument("-d", "--trace_dir", required=True,
                        help="The directory that CSMPI should write its trace files to")
    parser.add_argument("-u", "--trace_unmatched", default=False, action="store_true",
                        help="CSMPI should trace call stacks even for unmatched test and probe functions")
    parser.add_argument("-t", "--translate_in_place", default=False, action="store_true",
                        help="CSMPI should translate addresses in call stacks at runtime")
    parser.add_argument("-m", "--demangle_in_place", default=False, action="store_true",
                        help="CSMPI should demangle function names at runtime")
    args = parser.parse_args()

    main( args.output, 
          args.trace_dir, 
          args.functions, 
          args.trace_unmatched, 
          args.translate_in_place, 
          args.demangle_in_place )
