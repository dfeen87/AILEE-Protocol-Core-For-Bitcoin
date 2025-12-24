# Build Instructions

## Requirements
- CMake ≥ 3.16
- C++17 compiler
- libcurl
- ZeroMQ
- jsoncpp

## Build
```bash
mkdir build
cd build
cmake ..
make
ctest
```

## Tests
Unit tests are optional and can be enabled with:
```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

If GoogleTest is not installed on your system, it will be fetched automatically during the configure step.
