#!/bin/bash
cd build && make clean && make -j$(nproc) && ctest --verbose
