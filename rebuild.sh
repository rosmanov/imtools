#!/bin/bash -
set -x
rm -R CMakeCache.txt CMakeFiles
cmake $@ .
cores=$(grep -m1 'cores' /proc/cpuinfo | sed -e 's/[^0-9]//g' -)
((++cores))
make -j$cores
