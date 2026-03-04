#!/usr/bin/env bash
set -e

BUILD_DIR="build-native"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_SYSTEM_RAYLIB=ON \
    -DRPI_BUILD=OFF

cmake --build . -j$(nproc)

echo ""
echo "=== Native build complete ==="
echo "Run: ./${BUILD_DIR}/NBodyApp"
