#!/bin/bash
# Rebuilds project with profiling and coverage information
# for the `gprof` and `gcov` tools

set -o nounset

rm -rf CMakeCache.txt CMakeFiles
cmake -DIMTOOLS_DEBUG=ON \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
  -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -pg" .
make clean
make # VERBOSE=1

# vim: et ts=2 sts=2 sw=2
