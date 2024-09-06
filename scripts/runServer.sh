#!/bin/sh

cd ..

set -e

# Create and navigate to the build directory
mkdir -p build
cd build

# Run CMake to configure the project
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

# Build the server executable
cmake --build . --target server

# Navigate back to the project root directory and run the server
cd ..
exec ./build/server "$@"
