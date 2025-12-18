#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright © 2025 pqTox Contributors
#
# Cross-compile pqTox for Windows using MinGW-w64
#
# Prerequisites:
#   sudo apt-get install mingw-w64 mingw-w64-tools cmake ninja-build
#
# This script expects pre-built Windows dependencies in $DEPS_DIR
# including: libsodium, opus, libvpx, Qt6, and toxcore-pq

set -eu -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Configuration
ARCH="${ARCH:-x86_64}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
DEPS_DIR="${DEPS_DIR:-$HOME/pq-windows-deps}"
TOXCORE_DIR="${TOXCORE_DIR:-$HOME/toxcore-pq-windows}"
QT_DIR="${QT_DIR:-$HOME/qt6-mingw}"
OUTPUT_DIR="${OUTPUT_DIR:-$PROJECT_DIR/build-windows}"

if [ "$ARCH" = "x86_64" ]; then
    TOOLCHAIN_PREFIX="x86_64-w64-mingw32"
    TOOLCHAIN_FILE="$PROJECT_DIR/cmake/toolchain-mingw64.cmake"
elif [ "$ARCH" = "i686" ]; then
    TOOLCHAIN_PREFIX="i686-w64-mingw32"
    TOOLCHAIN_FILE="$PROJECT_DIR/cmake/toolchain-mingw32.cmake"
else
    echo "Error: Unsupported architecture '$ARCH'. Use x86_64 or i686."
    exit 1
fi

echo "=============================================="
echo "pqTox Windows Cross-Compilation"
echo "=============================================="
echo "Architecture: $ARCH"
echo "Build type:   $BUILD_TYPE"
echo "Dependencies: $DEPS_DIR"
echo "Toxcore:      $TOXCORE_DIR"
echo "Qt6:          $QT_DIR"
echo "Output:       $OUTPUT_DIR"
echo "=============================================="

# Check for toolchain
if ! command -v ${TOOLCHAIN_PREFIX}-gcc &> /dev/null; then
    echo "Error: MinGW toolchain not found. Install with:"
    echo "  sudo apt-get install mingw-w64"
    exit 1
fi

# Check for dependencies
for dir in "$DEPS_DIR" "$TOXCORE_DIR"; do
    if [ ! -d "$dir" ]; then
        echo "Warning: Directory not found: $dir"
        echo "You may need to build dependencies first."
    fi
done

# Create build directory
mkdir -p "$OUTPUT_DIR"
cd "$OUTPUT_DIR"

# Configure
echo ""
echo "Configuring..."
cmake "$PROJECT_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_PREFIX_PATH="$DEPS_DIR;$TOXCORE_DIR;$QT_DIR" \
    -DSTRICT_OPTIONS=ON \
    -DUPDATE_CHECK=OFF \
    "$@"

# Build
echo ""
echo "Building..."
cmake --build . --parallel $(nproc)

echo ""
echo "=============================================="
echo "Build complete!"
echo "Output: $OUTPUT_DIR"
echo "=============================================="
