#!/bin/bash -
# Generates reports for the `gcov` and `gprof` tools.

set -o nounset

[[ `command -v gcov` && `command -v gprof` ]] || \
  (echo >&2 "Error: gcov and gprof tools are not found"; exit 1)

gcov_pfx=("immerge")
gprof_log="gprof.log"

check_file() {
  if [[ -f "$1" ]]; then
    echo "* Generated $1 file"
  else
    echo >&2 "!! Error: failed to generate file ${1}.
    You may need to run the bin/${1} ... to create
    CMakeFiles/${f}.dir/src/${f}.cxx.{gcda,gcno} files."
    exit 1
  fi
}

for f in $gcov_pfx; do
  gcov --no-output -o CMakeFiles/${f}.dir/src/${f}.cxx.o -d src bin/${f}
  check_file "${f}.cxx.gcov"
done

gprof -b --no-graph -l bin/immerge > $gprof_log
check_file $gprof_log

# vim: et ts=2 sts=2 sw=2
