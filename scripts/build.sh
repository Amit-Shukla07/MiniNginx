#!/usr/bin/env bash
set -e

BUILD_DIR="build"
BUILD_TYPE="Release"

echo "==> Configuring MiniNginx ($BUILD_TYPE)..."
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo "==> Compiling MiniNginx..."
cmake --build "$BUILD_DIR" -j

echo "==> MiniNginx built successfully in $BUILD_DIR/"
