#!/bin/sh
cd ..

set -e

# Create and navigate to the build directory
mkdir -p build
cd build

# Run CMake to configure the project
cmake ..

# Build the client executable
cmake --build . --target client

# Navigate back to the project root directory and run the client
cd ..
exec ./build/client "$@"
