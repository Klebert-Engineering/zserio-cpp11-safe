#!/bin/bash

# Zserio C++11-Safe Extension Build Script
# This script builds the extension, runtime, and runs tests

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "=========================================="
echo "Zserio C++11-Safe Extension Build"
echo "=========================================="
echo

# Step 1: Extract zserio if not already done
ZSERIO_DIR="${BUILD_DIR}/zserio-2.16.1"
if [ ! -d "${ZSERIO_DIR}" ]; then
    echo "Extracting zserio release..."
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    unzip -q "${SCRIPT_DIR}/zserio_core/zserio-2.16.1-bin.zip" -d zserio-2.16.1
    cd "${SCRIPT_DIR}"
    echo "Zserio extracted to ${ZSERIO_DIR}"
else
    echo "Zserio already extracted"
fi

# Step 2: Build the C++11-safe extension
echo
echo "Building C++11-safe extension..."
cd "${PROJECT_ROOT}"

# Clean previous build
rm -rf "${BUILD_DIR}/extension"
mkdir -p "${BUILD_DIR}/extension/jar"

# Build with overridden paths
ant -Dzserio_core.jar_file="${ZSERIO_DIR}/zserio_libs/zserio_core.jar" \
    -Dzserio_cpp.build_dir="${BUILD_DIR}/extension" \
    -Dzserio_cpp.jar_dir="${BUILD_DIR}/extension/jar" \
    jar

if [ ! -f "${BUILD_DIR}/extension/jar/zserio_cpp.jar" ]; then
    echo "Error: Extension jar not created!"
    exit 1
fi

echo "Extension built successfully"

# Step 3: Build the runtime library
echo
echo "Building runtime library..."
RUNTIME_BUILD_DIR="${BUILD_DIR}/runtime"
mkdir -p "${RUNTIME_BUILD_DIR}"
cd "${RUNTIME_BUILD_DIR}"

# Configure and build using the runtime's CMakeLists.txt
cmake "${PROJECT_ROOT}/runtime" -DCMAKE_BUILD_TYPE=Release

make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

# Check if library was created in runtime subdirectory
if [ -f "${RUNTIME_BUILD_DIR}/runtime/libZserioCppRuntime.a" ]; then
    echo "Runtime library created at: ${RUNTIME_BUILD_DIR}/runtime/libZserioCppRuntime.a"
elif [ -f "${RUNTIME_BUILD_DIR}/libZserioCppRuntime.a" ]; then
    echo "Runtime library created at: ${RUNTIME_BUILD_DIR}/libZserioCppRuntime.a"
else
    echo "Error: Runtime library not created!"
    echo "Checked locations:"
    echo "  - ${RUNTIME_BUILD_DIR}/runtime/libZserioCppRuntime.a"
    echo "  - ${RUNTIME_BUILD_DIR}/libZserioCppRuntime.a"
    exit 1
fi

echo "Runtime built successfully"

# Step 4: Run mini test
echo
echo "Running mini test..."
cd "${SCRIPT_DIR}"
./mini/build.sh

echo
echo "=========================================="
echo "Build completed successfully!"
echo "=========================================="