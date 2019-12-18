#!/usr/bin/env bash

export CC=mpicc
export CXX=mpicxx
rm -rf $(pwd)/build && mkdir -p $(pwd)/build && cd build
cmake -DBOOST_ROOT=$HOME/repos/spack/opt/spack/linux-rhel7-power9le/gcc-9.2.0/boost-1.70.0-4yq3t45pazg3zrrek2a2hnt5efglmsmv/ \
      ..
make
