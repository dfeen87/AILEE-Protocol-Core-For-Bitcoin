cmake_minimum_required(VERSION 3.10)
project(AILEE_Core VERSION 1.2.1 LANGUAGES CXX)

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

# 2. CURL (Required for BitcoinRPCClient.h & BitcoinAdapter RPC)
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

# 4. JsonCpp (For RPC parsing in BitcoinAdapter)
pkg_check_modules(JSONCPP jsoncpp)
if(NOT JSONCPP_FOUND)
    find_package(jsoncpp QUIET)
    if(jsoncpp_FOUND)
        set(JSONCPP_LIBRARIES jsoncpp_lib)
        get_target_property(JSONCPP_INCLUDE_DIRS jsoncpp_lib INTERFACE_INCLUDE_DIRECTORIES)
    endif()
endif()

# 5. nlohmann/json (Header-only JSON for multi-chain adapters)
find_package(nlohmann_json QUIET)

# 6. yaml-cpp (Required for policy config hot-reload system)
find_package(yaml-cpp QUIET)

# 7. Threads (Required for running ZMQ listener in background)
find_package(Threads REQUIRED)

# ============================================================================
# Include Directories
# ============================================================================

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include # Best practice to keep headers here
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/l1
    ${CMAKE_CURRENT_SOURCE_DIR}/src/l2
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/security
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/recovery
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
)

if(ZeroMQ_INCLUDE_DIR)
    include_directories(${ZeroMQ_INCLUDE_DIR})
endif()

# Add yaml-cpp include if found
if(yaml-cpp_FOUND)
    include_directories(${YAML_CPP_INCLUDE_DIR})
endif()

# ============================================================================
# Source & Header Files
# ============================================================================

# Explicitly list headers so they appear in IDEs (VS Code / CLion)
set(AILEE_HEADERS
    src/l2/ailee_tps_engine.h
    src/l2/ailee_gold_bridge.h
    src/recovery/ailee_recovery_protocol.h
    src/l1/BitcoinZMQListener.h
    src/l1/BitcoinRPCClient.h
    src/telemetry/ailee_energy_telemetry.h
    src/security/ailee_circuit_breaker.h
    src/l2/ailee_sidechain_bridge.h
    include/ailee_dao_governance.h
    include/Global_Seven.h
    include/AmbientAI.h
    include/L2State.h
)

# Policy System Headers (new files)
set(POLICY_HEADERS
    include/policies.h
    include/metrics.h
    include/expr.h
    config/config_types.h
    config/config_loader.h
    config/config_hot_reload.h
    include/config_types.h
    include/config_loader.h
    include/config_hot_reload.h
)

# Adapter implementation sources
set(ADAPTER_SOURCES
    src/l1/BitcoinAdapter.cpp
    src/l1/EthereumAdapter.cpp
    src/l1/PolygonAdapter.cpp
    src/l1/SolanaAdapter.cpp
    src/l1/AvalancheAdapter.cpp
    src/l1/LitecoinAdapter.cpp
    src/l1/DogecoinAdapter.cpp
    src/l1/AdapterRegistry.cpp
    src/l1/AILEEMempoolAdapter.cpp
    src/l1/AILEENetworkAdapter.cpp
    src/l1/AILEEEnergyAdapter.cpp
    src/l1/EvmBaseAdapter.cpp
    src/l1/CardanoAdapter.cpp
    src/l1/PolkadotAdapter.cpp
    src/l1/LitecoinAdapter.cpp
    src/l1/DogecoinAdapter.cpp
    src/l1/AdapterRegistry.cpp
)

# AmbientAI Sources (new mesh intelligence layer)
set(AMBIENT_SOURCES
    src/AmbientAI.cpp
)

# Policy System Sources
set(POLICY_SOURCES
    src/security/policies.cpp
    src/telemetry/metrics.cpp
    src/policies.cpp
    src/metrics.cpp
    src/expr.cpp
    src/config_loader.cpp
    src/config_hot_reload.cpp
)

# Main Executable Sources
set(CORE_SOURCES
    src/main.cpp
    src/orchestration/Orchestrator.cpp
    src/security/ailee_circuit_breaker.cpp
    src/security/zk_proofs.cpp
    src/l1/AnchorCommitment.cpp
    src/l2/L2State.cpp
    ${ADAPTER_SOURCES}
    ${AMBIENT_SOURCES}
    ${POLICY_SOURCES}
    ${AILEE_HEADERS}
    ${POLICY_HEADERS}
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

# --- 2. L2 Verification Tool ---
add_executable(ailee_l2_verify
    src/tools/ailee_l2_verify.cpp
    src/security/zk_proofs.cpp
    src/l1/AnchorCommitment.cpp
    src/l2/L2State.cpp
)

target_link_libraries(ailee_l2_verify
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(JSONCPP_FOUND OR jsoncpp_FOUND)
    target_compile_definitions(ailee_node PRIVATE AILEE_HAS_JSONCPP=1)
endif()

# Link yaml-cpp if found (for policy system)
if(yaml-cpp_FOUND)
    target_link_libraries(ailee_node PRIVATE yaml-cpp)
else()
    message(WARNING "yaml-cpp not found - policy hot-reload system will be disabled")
endif()

# Robust ZeroMQ Linking
if(TARGET cppzmq)
    target_link_libraries(ailee_node PRIVATE cppzmq)
    target_compile_definitions(ailee_node PRIVATE AILEE_HAS_ZMQ=1)
elseif(ZeroMQ_LIBRARY)
    # Fallback if the target doesn't exist (manual linking)
    target_link_libraries(ailee_node PRIVATE ${ZeroMQ_LIBRARY})
    target_compile_definitions(ailee_node PRIVATE AILEE_HAS_ZMQ=1)
else()
    message(WARNING "ZeroMQ not found - real-time Bitcoin event listening will be disabled")
endif()

# --- 2. Adapter Library (Static) ---
# Useful for testing adapters without running the full node
add_library(ailee_adapters STATIC 
    ${ADAPTER_SOURCES}
    ${AMBIENT_SOURCES}
    ${POLICY_SOURCES}
    src/security/zk_proofs.cpp
)

target_link_libraries(ailee_adapters
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    Threads::Threads
)

if(JSONCPP_FOUND OR jsoncpp_FOUND)
    target_compile_definitions(ailee_adapters PRIVATE AILEE_HAS_JSONCPP=1)
endif()

# Link nlohmann/json if available (header-only)
if(nlohmann_json_FOUND)
    target_link_libraries(ailee_adapters PRIVATE nlohmann_json::nlohmann_json)
endif()

# Link yaml-cpp to library as well
if(yaml-cpp_FOUND)
    target_link_libraries(ailee_adapters PRIVATE yaml-cpp)
endif()

# Robust ZeroMQ Linking for library
if(TARGET cppzmq)
    target_link_libraries(ailee_adapters PRIVATE cppzmq)
    target_compile_definitions(ailee_adapters PRIVATE AILEE_HAS_ZMQ=1)
elseif(ZeroMQ_LIBRARY)
    target_link_libraries(ailee_adapters PRIVATE ${ZeroMQ_LIBRARY})
    target_compile_definitions(ailee_adapters PRIVATE AILEE_HAS_ZMQ=1)
endif()

# ============================================================================
# Unit Tests (Optional)
# ============================================================================

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        add_library(minigtest STATIC
            tests/minigtest/minigtest.cpp
        )
        target_include_directories(minigtest PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/minigtest
        )

        add_library(minigtest_main STATIC
            tests/minigtest/minigtest_main.cpp
        )
        target_link_libraries(minigtest_main PUBLIC minigtest)

        add_library(GTest::gtest ALIAS minigtest)
        add_library(GTest::gtest_main ALIAS minigtest_main)
    endif()
    enable_testing()

    add_executable(ailee_tests 
        tests/AdapterRegistryTests.cpp
    )
    
    target_link_libraries(ailee_tests
        PRIVATE
        ailee_adapters
        GTest::gtest
        GTest::gtest_main
    )
    
    add_test(NAME AdapterTests COMMAND ailee_tests)
    
    # Optional: Add policy system tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/PolicyTests.cpp")
        add_executable(policy_tests tests/PolicyTests.cpp)
        target_link_libraries(policy_tests
            PRIVATE
            ailee_adapters
            GTest::gtest
            GTest::gtest_main
        )
        add_test(NAME PolicyTests COMMAND policy_tests)
    endif()
endif()

# ============================================================================
# Installation
# ============================================================================

# Install the binary
install(TARGETS ailee_node DESTINATION bin)

# Install the Library Headers (So others can build on AILEE)
install(FILES 
    ${AILEE_HEADERS}
    ${POLICY_HEADERS}
    DESTINATION include/ailee
)

# Install default config file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.yaml")
    install(FILES config.yaml DESTINATION etc/ailee)
endif()

# ============================================================================
# Post-Build: Create output directories
# ============================================================================

# Create directories for policy system outputs
add_custom_command(TARGET ailee_node POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        ${CMAKE_BINARY_DIR}/out
    COMMENT "Creating output directories for metrics"
)

# ============================================================================
# Package Configuration (Output Info)
# ============================================================================

message(STATUS "=============================================================")
message(STATUS "   AILEE Core v${PROJECT_VERSION} Build Configuration        ")
message(STATUS "=============================================================")
message(STATUS "   Build Type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "   C++ Standard:     ${CMAKE_CXX_STANDARD}")
message(STATUS "   Optimization:     ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "-------------------------------------------------------------")
message(STATUS "   OpenSSL:          ${OPENSSL_VERSION}")
message(STATUS "   CURL:             ${CURL_VERSION_STRING}")
message(STATUS "   ZeroMQ:           ${ZeroMQ_FOUND}")
if(ZeroMQ_FOUND)
    message(STATUS "     - Include:      ${ZeroMQ_INCLUDE_DIR}")
    message(STATUS "     - Library:      ${ZeroMQ_LIBRARY}")
endif()
message(STATUS "   JsonCpp:          ${JSONCPP_FOUND}")
message(STATUS "   yaml-cpp:         ${yaml-cpp_FOUND}")
message(STATUS "-------------------------------------------------------------")
message(STATUS "   Core Modules:     TPS Engine, Recovery, Gold Bridge")
message(STATUS "   Network:          Sidechain Bridge, ZMQ Listener")
message(STATUS "   Governance:       DAO, Circuit Breaker")
message(STATUS "   Multi-Chain:      Global Seven (7+ adapters)")
message(STATUS "   AmbientAI:        Mesh Intelligence (Federated)")
message(STATUS "   Policy System:    ${yaml-cpp_FOUND}")
message(STATUS "   Build Tests:      ${BUILD_TESTS}")
message(STATUS "=============================================================")

# Warnings for missing optional dependencies
if(NOT yaml-cpp_FOUND)
    message(STATUS "")
    message(STATUS "⚠️  WARNING: yaml-cpp not found")
    message(STATUS "    Policy hot-reload system will be disabled")
    message(STATUS "    Install: sudo apt-get install libyaml-cpp-dev")
    message(STATUS "")
endif()

if(NOT ZeroMQ_FOUND)
    message(STATUS "")
    message(STATUS "⚠️  WARNING: ZeroMQ not found")
    message(STATUS "    Real-time Bitcoin monitoring will fall back to polling")
    message(STATUS "    Install: sudo apt-get install libzmq3-dev libcppzmq-dev")
    message(STATUS "")
endif()
