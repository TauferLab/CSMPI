#!/usr/bin/env bash

#SBATCH -o csmpi_tests-%j.out
#SBATCH -e csmpi_tests-%j.err

n_messages=${1:-1000}

# Orient ourselves
system=$( hostname | sed 's/[0-9]*//g' )
build_dir="../build_${system}/"

# CSMPI library
csmpi_lib=${build_dir}/libcsmpi.so

# Test executable
test_bin=${build_dir}/ping_pong_test

# Run ping-pong test
echo "Running ping-pong test with # messages = ${n_messages}"
echo "Running without CSMPI"
time srun -N1 -n2 ${test_bin} ${n_messages}
echo "Running with glibc backtrace implementation"
time LD_PRELOAD=${csmpi_lib} CSMPI_CONFIG="./config/test_ping_pong_glibc.json" srun -N1 -n2 ${test_bin} ${n_messages}
echo "Running with libunwind backtrace implementation"
time LD_PRELOAD=${csmpi_lib} CSMPI_CONFIG="./config/test_ping_pong_libunwind.json" srun -N1 -n2 ${test_bin} ${n_messages}
echo "Running with libunwind backtrace implementation + in-place address translation (VERY SLOW. GENERALLY SPEAKING DO NOT DO THIS.)"
time LD_PRELOAD=${csmpi_lib} CSMPI_CONFIG="./config/test_ping_pong_libunwind_translate.json" srun -N1 -n2 ${test_bin} ${n_messages}
