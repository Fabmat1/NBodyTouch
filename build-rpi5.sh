#!/usr/bin/env bash
set -e

BUILD_DIR="build-rpi5"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-rpi5.cmake \
    -DUSE_SYSTEM_RAYLIB=OFF \
    -DRPI_BUILD=ON

cmake --build . -j$(nproc)

echo ""
echo "=== RPi 5 cross-build complete ==="
echo "Copy ${BUILD_DIR}/ to your Pi and run ./NBodyApp"
