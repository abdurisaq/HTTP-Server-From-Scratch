cmake_minimum_required(VERSION 3.13)

project(network_application)

# Define the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Collect all source files for the server and client
file(GLOB SERVER_SOURCES dataServer/*.cpp dataServer/*.hpp)
file(GLOB CLIENT_SOURCES remoteClient/*.cpp remoteClient/*.hpp)

# Create the server executable
add_executable(server ${SERVER_SOURCES})

# Create the client executable
add_executable(client ${CLIENT_SOURCES})