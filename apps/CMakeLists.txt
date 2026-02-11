# Top-level helper: build all FRANK OS apps
# Each app is a standalone Pico SDK project (like MOS2 apps).
#
# To build all apps:
#   cd apps && ./build.sh
#
# To build a single app:
#   cd apps/source/minesweeper
#   mkdir build && cd build && cmake .. && make
cmake_minimum_required(VERSION 3.13)
project(frankos-apps NONE)

# Each app is built independently since they each init the Pico SDK.
# This file exists for documentation purposes.
