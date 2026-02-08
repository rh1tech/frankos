#!/bin/bash
# Build Rhea OS for RP2350
rm -rf ./build
mkdir build
cd build
cmake -DBOARD_VARIANT=M2 ..
make -j4
