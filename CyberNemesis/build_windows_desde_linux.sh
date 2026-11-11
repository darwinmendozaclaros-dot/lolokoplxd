#!/bin/sh
set -e
cd "$(dirname "$0")"
cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-win -j
cmake --build build-win --target package_game
echo "Listo: release/CyberNemesis.exe y release/CyberNemesis-windows-x64.zip"
