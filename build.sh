#!/bin/bash

# default build type
BUILD_TYPE=Release

# check provided argument
if [ "$#" -eq 1 ]; then
  ARG=$1
  if [ "$ARG" == "debug" ] || [ "$ARG" == "Debug" ]; then
    BUILD_TYPE=Debug
  elif [ "$ARG" == "release" ] || [ "$ARG" == "Release" ]; then
    BUILD_TYPE=Release
  else
    echo "Unknown build type: $ARG"
    echo "Usage: $0 [Debug|Release]"
    exit 1
  fi
else
  echo "No build type specified, defaulting to Release"
  echo "Usage: $0 [debug|release]"
fi

# create and enter build directory
mkdir -p build
cd build

# cmake with specified build type
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

# build the project
make