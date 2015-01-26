#!/bin/bash -
# Builds documentation

# Exits if specified executable is not available in $PATH.
# $1 - executable name
require_executable() {
  command -v "$1" > /dev/null
  if [[ $? -ne 0 ]]; then
    echo >&2 "Error: '$1' executable is not available in $PATH. Please install corresponding package."
    exit 1
  fi
}

require_executable doxygen

dir=$(cd $(dirname "$0"); pwd)
cd "$dir"
doxygen
cd -

# vim: et ts=2 sts=2 sw=2
