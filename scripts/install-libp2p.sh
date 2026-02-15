#!/bin/bash
# install-libp2p.sh - Install cpp-libp2p library for AILEE-Core
# 
# This script downloads, builds, and installs the cpp-libp2p library
# which is required for full P2P networking support in AILEE-Core.

set -e  # Exit on error

echo "============================================================"
echo "  cpp-libp2p Installation Script for AILEE-Core"
echo "============================================================"
echo ""

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    echo "Warning: Running as root. Libraries will be installed system-wide."
    SUDO=""
else
    echo "Note: sudo will be used for installation."
    SUDO="sudo"
fi

# Configuration
LIBP2P_VERSION="${LIBP2P_VERSION:-latest}"
BUILD_DIR="${BUILD_DIR:-/tmp/cpp-libp2p-build}"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
JOBS="${JOBS:-$(nproc)}"

echo "Configuration:"
echo "  Version: $LIBP2P_VERSION"
echo "  Build directory: $BUILD_DIR"
echo "  Install prefix: $INSTALL_PREFIX"
echo "  Parallel jobs: $JOBS"
echo ""

# Check prerequisites
echo "Checking prerequisites..."
command -v git >/dev/null 2>&1 || { echo "Error: git is required but not installed." >&2; exit 1; }
command -v cmake >/dev/null 2>&1 || { echo "Error: cmake is required but not installed." >&2; exit 1; }
command -v g++ >/dev/null 2>&1 || command -v clang++ >/dev/null 2>&1 || { echo "Error: C++ compiler is required." >&2; exit 1; }
echo "Prerequisites OK"
echo ""

# Install build dependencies
echo "Installing build dependencies..."
if [ -f /etc/debian_version ]; then
    # Debian/Ubuntu
    $SUDO apt-get update
    $SUDO apt-get install -y \
        build-essential \
        cmake \
        git \
        libssl-dev \
        libboost-all-dev \
        pkg-config
elif [ -f /etc/redhat-release ]; then
    # RedHat/CentOS/Fedora
    $SUDO yum install -y \
        gcc-c++ \
        cmake \
        git \
        openssl-devel \
        boost-devel
elif [ "$(uname)" == "Darwin" ]; then
    # macOS
    brew install cmake openssl boost
else
    echo "Warning: Unknown OS. Please install build dependencies manually."
fi
echo ""

# Create build directory
echo "Setting up build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
echo ""

# Clone cpp-libp2p repository
echo "Cloning cpp-libp2p repository..."
if [ -d "cpp-libp2p" ]; then
    echo "Repository already exists, updating..."
    cd cpp-libp2p
    git pull
else
    git clone --recursive https://github.com/libp2p/cpp-libp2p.git
    cd cpp-libp2p
fi
echo ""

# Note about Hunter package manager
echo "============================================================"
echo "  IMPORTANT: cpp-libp2p uses Hunter package manager"
echo "============================================================"
echo ""
echo "The cpp-libp2p library uses Hunter for dependency management."
echo "This means the first build will download and compile many"
echo "dependencies, which can take 30-60 minutes depending on your"
echo "system."
echo ""
echo "Hunter will cache compiled dependencies in:"
echo "  ~/.hunter/"
echo ""
echo "To use a shared Hunter cache for faster builds:"
echo "  export HUNTER_ROOT=/path/to/shared/hunter/cache"
echo ""
read -p "Continue with build? (y/n) " -n 1 -r
echo ""
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Build cancelled."
    exit 0
fi
echo ""

# Build cpp-libp2p
echo "Building cpp-libp2p..."
mkdir -p build
cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DBUILD_SHARED_LIBS=ON \
    -DTESTING=OFF

make -j"$JOBS"
echo ""

# Install cpp-libp2p
echo "Installing cpp-libp2p to $INSTALL_PREFIX..."
$SUDO make install
$SUDO ldconfig || true
echo ""

# Verify installation
echo "Verifying installation..."
if [ -f "$INSTALL_PREFIX/lib/libp2p.so" ] || [ -f "$INSTALL_PREFIX/lib/libp2p.dylib" ]; then
    echo "✓ libp2p library installed successfully"
else
    echo "✗ Warning: libp2p library not found in expected location"
fi

if [ -d "$INSTALL_PREFIX/include/libp2p" ]; then
    echo "✓ libp2p headers installed successfully"
else
    echo "✗ Warning: libp2p headers not found in expected location"
fi
echo ""

# Print next steps
echo "============================================================"
echo "  Installation Complete!"
echo "============================================================"
echo ""
echo "Next steps:"
echo ""
echo "1. Rebuild AILEE-Core with libp2p support:"
echo "   cd /path/to/AILEE-Protocol-Core-For-Bitcoin"
echo "   rm -rf build"
echo "   mkdir build && cd build"
echo "   cmake .."
echo "   make -j$JOBS"
echo ""
echo "2. Verify libp2p is detected:"
echo "   Look for: 'libp2p found: $INSTALL_PREFIX/lib/libp2p.so'"
echo "   Look for: 'libp2p support enabled'"
echo ""
echo "3. Test P2P networking:"
echo "   ./ailee_p2p_demo"
echo ""
echo "For more information, see:"
echo "  - docs/LIBP2P_INTEGRATION.md"
echo "  - https://github.com/libp2p/cpp-libp2p"
echo ""

# Cleanup option
read -p "Remove build directory? ($BUILD_DIR) (y/n) " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    cd /
    rm -rf "$BUILD_DIR"
    echo "Build directory removed."
fi

echo ""
echo "Done!"
