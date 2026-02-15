# FindLibp2p.cmake - Find libp2p C++ library
# This module defines:
#  LIBP2P_FOUND - System has libp2p
#  LIBP2P_INCLUDE_DIRS - The libp2p include directories
#  LIBP2P_LIBRARIES - The libraries needed to use libp2p
#  LIBP2P_VERSION - The version of libp2p found

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LIBP2P QUIET libp2p)
endif()

# Find include directory
find_path(LIBP2P_INCLUDE_DIR
    NAMES libp2p/host/host.hpp
    HINTS
        ${PC_LIBP2P_INCLUDE_DIRS}
        /usr/local/include
        /usr/include
    PATH_SUFFIXES libp2p
)

# Find library
find_library(LIBP2P_LIBRARY
    NAMES p2p libp2p
    HINTS
        ${PC_LIBP2P_LIBRARY_DIRS}
        /usr/local/lib
        /usr/lib
)

# Set variables
if(LIBP2P_INCLUDE_DIR)
    set(LIBP2P_INCLUDE_DIRS ${LIBP2P_INCLUDE_DIR})
endif()

if(LIBP2P_LIBRARY)
    set(LIBP2P_LIBRARIES ${LIBP2P_LIBRARY})
endif()

if(PC_LIBP2P_VERSION)
    set(LIBP2P_VERSION ${PC_LIBP2P_VERSION})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libp2p
    FOUND_VAR LIBP2P_FOUND
    REQUIRED_VARS
        LIBP2P_LIBRARY
        LIBP2P_INCLUDE_DIR
    VERSION_VAR LIBP2P_VERSION
)

mark_as_advanced(
    LIBP2P_INCLUDE_DIR
    LIBP2P_LIBRARY
)
