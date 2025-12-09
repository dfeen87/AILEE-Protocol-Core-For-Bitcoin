cmake_minimum_required(VERSION 3.10)
project(AILEE_Core VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# ============================================================================
# Dependencies
# ============================================================================

# OpenSSL (cryptographic functions)
find_package(OpenSSL REQUIRED)

# CURL (HTTP/HTTPS for RPC)
find_package(CURL REQUIRED)

# JsonCpp (JSON parsing for RPC responses)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
if(NOT JSONCPP_FOUND)
    message(STATUS "jsoncpp not found via pkg-config, trying find_package")
    find_package(jsoncpp REQUIRED)
    set(JSONCPP_LIBRARIES jsoncpp_lib)
    get_target_property(JSONCPP_INCLUDE_DIRS jsoncpp_lib INTERFACE_INCLUDE_DIRECTORIES)
endif()

# ZeroMQ with C++ bindings (cppzmq)
find_package(ZeroMQ REQUIRED)
find_package(cppzmq REQUIRED)

# Threads (for multi-threading)
find_package(Threads REQUIRED)

# Optional: GoogleTest for unit tests
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
endif()

# ============================================================================
# Include Directories
# ============================================================================

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${ZeroMQ_INCLUDE_DIR}
)

# ============================================================================
# Source Files
# ============================================================================

set(ADAPTER_SOURCES
    BitcoinAdapter.cpp
    EthereumAdapter.cpp
    PolygonAdapter.cpp
    SolanaAdapter.cpp
    AvalancheAdapter.cpp
    CardanoAdapter.cpp
    PolkadotAdapter.cpp
    LitecoinAdapter.cpp
    DogecoinAdapter.cpp
    AdapterRegistry.cpp
)

set(CORE_SOURCES
    main.cpp
    ${ADAPTER_SOURCES}
)

# ============================================================================
# Main Executable
# ============================================================================

add_executable(ailee_node ${CORE_SOURCES})

target_link_libraries(ailee_node
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    libzmq
    cppzmq
    Threads::Threads
)

# ============================================================================
# Optional: Separate Library Target
# ============================================================================

# Create a library for adapters (useful for testing)
add_library(ailee_adapters STATIC ${ADAPTER_SOURCES})

target_link_libraries(ailee_adapters
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    libzmq
    cppzmq
    Threads::Threads
)

# ============================================================================
# Unit Tests (if enabled)
# ============================================================================

if(BUILD_TESTS)
    add_executable(ailee_tests
        AdapterRegistryTests.cpp
    )
    
    target_link_libraries(ailee_tests
        ailee_adapters
        GTest::GTest
        GTest::Main
    )
    
    add_test(NAME AdapterTests COMMAND ailee_tests)
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS ailee_node DESTINATION bin)
install(FILES Global_Seven.h DESTINATION include/ailee)

# ============================================================================
# Package Configuration
# ============================================================================

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/AILEEConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# ============================================================================
# Status Output
# ============================================================================

message(STATUS "=============================================================")
message(STATUS "AILEE Core Configuration")
message(STATUS "=============================================================")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "OpenSSL Version: ${OPENSSL_VERSION}")
message(STATUS "CURL Version: ${CURL_VERSION_STRING}")
message(STATUS "ZeroMQ Found: ${ZeroMQ_FOUND}")
message(STATUS "Build Tests: ${BUILD_TESTS}")
message(STATUS "=============================================================")
