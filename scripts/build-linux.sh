#!/bin/bash
# Build svanipp for Linux
# Usage: ./scripts/build-linux.sh [Release|Debug]

set -e

BUILD_TYPE="${1:-Release}"

echo "Building svanipp ($BUILD_TYPE)..."

cmake -S . -B build -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build build --parallel

echo ""
echo "Build complete: build/svanipp"