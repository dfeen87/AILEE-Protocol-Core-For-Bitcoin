cmake_minimum_required(VERSION 3.10)
project(AILEE_Core)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL REQUIRED)

# Look for header files in the current root directory (.)
include_directories(.)

# Look for main.cpp in the current root directory
add_executable(ailee_node main.cpp)

target_link_libraries(ailee_node OpenSSL::SSL OpenSSL::Crypto)
