#!/bin/bash

# Mini Test Build Script
# Generates C++ code from schema and builds/runs test

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${TEST_DIR}/build"
MINI_BUILD_DIR="${BUILD_DIR}/mini"

echo "Building mini test..."

# Check prerequisites
if [ ! -f "${BUILD_DIR}/extension/jar/zserio_cpp.jar" ]; then
    echo "Error: Extension not built. Run build_all.sh first!"
    exit 1
fi

# Check for runtime library in either location
if [ -f "${BUILD_DIR}/runtime/runtime/libZserioCppRuntime.a" ]; then
    RUNTIME_LIB_PATH="${BUILD_DIR}/runtime/runtime/libZserioCppRuntime.a"
elif [ -f "${BUILD_DIR}/runtime/libZserioCppRuntime.a" ]; then
    RUNTIME_LIB_PATH="${BUILD_DIR}/runtime/libZserioCppRuntime.a"
else
    echo "Error: Runtime not built. Run build_all.sh first!"
    exit 1
fi

# Step 1: Generate C++ code
echo "Generating C++ code from schema..."
mkdir -p "${MINI_BUILD_DIR}/generated"

# Use java with proper classpath
java -cp "${BUILD_DIR}/zserio-2.16.1/zserio_libs/zserio_core.jar:${BUILD_DIR}/extension/jar/zserio_cpp.jar" \
     zserio.tools.ZserioTool \
     -cpp "${MINI_BUILD_DIR}/generated" \
     -src "${SCRIPT_DIR}/schema" \
     minizs.zs

if [ ! -d "${MINI_BUILD_DIR}/generated/minizs" ]; then
    echo "Error: Code generation failed!"
    exit 1
fi

echo "Code generated successfully"

# Step 2: Build test application
echo "Building test application..."
mkdir -p "${MINI_BUILD_DIR}/app"
cd "${MINI_BUILD_DIR}/app"

cmake "${SCRIPT_DIR}/app" \
    -DCMAKE_BUILD_TYPE=Release

make

if [ ! -f "${MINI_BUILD_DIR}/app/mini_test" ]; then
    echo "Error: Test executable not created!"
    exit 1
fi

echo "Test application built successfully"

# Step 3: Run tests
echo
echo "Running tests..."
echo
./mini_test

# Return the exit code from the test
exit $?