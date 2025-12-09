cmake_minimum_required(VERSION 3.10)
project(AILEE_Core VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# Compiler & Standard Settings
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type default to Release for performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler optimization flags (Critical for TPS Engine)
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native") # Optimize for local CPU
endif()

# ============================================================================
# Dependencies
# ============================================================================

# 1. OpenSSL (Required for SHA256 in Recovery Protocol & Gold Bridge)
find_package(OpenSSL REQUIRED)

# 2. CURL (Required for BitcoinRPCClient.h)
find_package(CURL REQUIRED)

# 3. ZeroMQ & cppzmq (Required for BitcoinZMQListener.h)
# We use PkgConfig as a fallback for Linux/Unix environments
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LIBZMQ QUIET libzmq)
endif()

find_package(ZeroMQ QUIET)
if(NOT ZeroMQ_FOUND)
    # If standard find fails, try PkgConfig hints
    find_path(ZeroMQ_INCLUDE_DIR zmq.h HINTS ${PC_LIBZMQ_INCLUDE_DIRS})
    find_library(ZeroMQ_LIBRARY zmq HINTS ${PC_LIBZMQ_LIBRARY_DIRS})
endif()

# Find the C++ bindings (header only usually)
find_package(cppzmq QUIET)

# 4. JsonCpp (For future RPC parsing updates)
pkg_check_modules(JSONCPP jsoncpp)
if(NOT JSONCPP_FOUND)
    find_package(jsoncpp QUIET)
endif()

# 5. Threads (Required for running ZMQ listener in background)
find_package(Threads REQUIRED)

# ============================================================================
# Include Directories
# ============================================================================

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include # Best practice to keep headers here
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${ZeroMQ_INCLUDE_DIR}
)

# ============================================================================
# Source & Header Files
# ============================================================================

# Explicitly list headers so they appear in IDEs (VS Code / CLion)
set(AILEE_HEADERS
    ailee_tps_engine.h
    ailee_gold_bridge.h
    ailee_recovery_protocol.h
    ailee_bitcoin_zmq_listener.h
    ailee_bitcoin_rpc_client.h
    ailee_energy_telemetry.h
    ailee_circuit_breaker.h
)

# Your Adapter implementation sources
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

# Main Executable Sources
set(CORE_SOURCES
    main.cpp
    ${ADAPTER_SOURCES}
    ${AILEE_HEADERS} 
)

# ============================================================================
# Build Targets
# ============================================================================

# --- 1. The Main Node Executable ---
add_executable(ailee_node ${CORE_SOURCES})

target_link_libraries(ailee_node
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    Threads::Threads
)

# Robust ZeroMQ Linking
if(TARGET cppzmq)
    target_link_libraries(ailee_node PRIVATE cppzmq)
else()
    # Fallback if the target doesn't exist (manual linking)
    target_link_libraries(ailee_node PRIVATE ${ZeroMQ_LIBRARY})
endif()

# --- 2. Adapter Library (Static) ---
# Useful for testing adapters without running the full node
add_library(ailee_adapters STATIC ${ADAPTER_SOURCES})
target_link_libraries(ailee_adapters
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
    Threads::Threads
)

# ============================================================================
# Unit Tests (Optional)
# ============================================================================

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()

    add_executable(ailee_tests AdapterRegistryTests.cpp)
    target_link_libraries(ailee_tests
        PRIVATE
        ailee_adapters
        GTest::GTest
        GTest::Main
    )
    add_test(NAME AdapterTests COMMAND ailee_tests)
endif()

# ============================================================================
# Installation
# ============================================================================

# Install the binary
install(TARGETS ailee_node DESTINATION bin)

# Install the Library Headers (So others can build on AILEE)
install(FILES 
    ${AILEE_HEADERS}
    Global_Seven.h 
    DESTINATION include/ailee
)

# ============================================================================
# Package Configuration (Output Info)
# ============================================================================

message(STATUS "=============================================================")
message(STATUS "   AILEE Core v${PROJECT_VERSION} Build Configuration        ")
message(STATUS "=============================================================")
message(STATUS "   Build Type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "   C++ Standard:     ${CMAKE_CXX_STANDARD}")
message(STATUS "   OpenSSL:          Found")
message(STATUS "   CURL:             Found")
message(STATUS "   ZeroMQ:           ${ZeroMQ_FOUND} (Include: ${ZeroMQ_INCLUDE_DIR})")
message(STATUS "   Modules:          TPS, Recovery, Gold, ZMQ, RPC")
message(STATUS "=============================================================")
