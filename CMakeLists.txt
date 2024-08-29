cmake_minimum_required(VERSION 3.13)

project(network_application)

# Define the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Collect all source files for the server and client
file(GLOB SERVER_SOURCES dataServer/*.cpp dataServer/*.hpp)
file(GLOB CLIENT_SOURCES remoteClient/*.cpp remoteClient/*.hpp)
file(GLOB TEST_SOURCES remoteClient/keylogger.cpp remoteClient/*.hpp)

if(WIN32)
    message(STATUS "Configuring for Windows")
    # Add any Windows-specific compile options or libraries
    add_definitions(-DPLATFORM_WINDOWS)
    # Example: linking with ws2_32 for networking on Windows
    set(PLATFORM_LIBS ws2_32)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux")
    # Add any Linux-specific compile options or libraries
    add_definitions(-DPLATFORM_LINUX)
    # Example: linking with pthread for threading on Linux
    set(PLATFORM_LIBS pthread)
endif()

# Create the server executable
add_executable(server ${SERVER_SOURCES})

target_link_libraries(server ${PLATFORM_LIBS})

# Create the client executable
add_executable(client ${CLIENT_SOURCES})

target_link_libraries(client ${PLATFORM_LIBS})


add_executable(test_runner ${TEST_SOURCES})
target_link_libraries(test_runner ${PLATFORM_LIBS})
