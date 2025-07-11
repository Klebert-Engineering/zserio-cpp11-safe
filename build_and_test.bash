#!/bin/bash

# C++11 Safe Extension Build and Test Script
# This script builds the Java extension, C++ runtime, and runs tests using CMake

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Print colored output
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Function to check prerequisites
check_prerequisites() {
    print_info "Checking prerequisites..."
    
    # Check Java
    if ! command -v java &> /dev/null; then
        print_error "Java is required but not installed"
        exit 1
    fi
    
    # Check Ant
    if ! command -v ant &> /dev/null; then
        print_error "Apache Ant is required but not installed"
        exit 1
    fi
    
    # Check CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake is required but not installed"
        exit 1
    fi
    
    # Check C++ compiler
    if ! command -v c++ &> /dev/null && ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        print_error "C++ compiler is required but not installed"
        exit 1
    fi
    
    print_info "All prerequisites found"
}

# Main build function
main() {
    print_info "Starting C++11 Safe Extension build and test..."
    
    # Check prerequisites
    check_prerequisites
    
    # Clean build directory if requested
    if [ "${CLEAN_BUILD}" = "YES" ]; then
        print_info "Cleaning build directory..."
        rm -rf "${BUILD_DIR}"
    fi
    
    # Create build directory
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    
    # Configure with CMake
    print_info "Configuring with CMake..."
    
    # For clean builds with tests, we need a two-stage approach
    if [ "${CLEAN_BUILD}" = "YES" ] && [ "${BUILD_TESTS}" = "ON" ] && [ "${BUILD_EXTENSION}" = "ON" ]; then
        print_info "Clean build detected - using two-stage build process..."
        
        # Stage 1: Build extension and runtime without tests
        print_info "Stage 1: Building extension and runtime..."
        cmake "${SCRIPT_DIR}" \
            -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
            -DBUILD_EXTENSION=${BUILD_EXTENSION} \
            -DBUILD_RUNTIME=${BUILD_RUNTIME} \
            -DBUILD_TESTS=OFF \
            -DRUN_TESTS=OFF
        
        cmake --build . --parallel --target extension_built
        
        # Stage 2: Reconfigure with tests
        print_info "Stage 2: Reconfiguring with tests..."
        cmake "${SCRIPT_DIR}" \
            -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
            -DBUILD_EXTENSION=${BUILD_EXTENSION} \
            -DBUILD_RUNTIME=${BUILD_RUNTIME} \
            -DBUILD_TESTS=${BUILD_TESTS} \
            -DRUN_TESTS=${RUN_TESTS}
    else
        # Normal configure
        cmake "${SCRIPT_DIR}" \
            -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
            -DBUILD_EXTENSION=${BUILD_EXTENSION} \
            -DBUILD_RUNTIME=${BUILD_RUNTIME} \
            -DBUILD_TESTS=${BUILD_TESTS} \
            -DRUN_TESTS=${RUN_TESTS}
    fi
    
    # Build
    print_info "Building..."
    cmake --build . --parallel
    
    # Run tests
    print_info "Running tests..."
    ctest --output-on-failure
    
    print_info "Build and test completed successfully!"
}

# Parse command line arguments
BUILD_TYPE="Release"
BUILD_EXTENSION=ON
BUILD_RUNTIME=ON
BUILD_TESTS=ON
RUN_TESTS=ON
CLEAN_BUILD=NO

while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN_BUILD=YES
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --no-extension)
            BUILD_EXTENSION=OFF
            shift
            ;;
        --no-runtime)
            BUILD_RUNTIME=OFF
            shift
            ;;
        --no-tests)
            BUILD_TESTS=OFF
            RUN_TESTS=OFF
            shift
            ;;
        --no-run-tests)
            RUN_TESTS=OFF
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --clean          Remove build directory before building"
            echo "  --debug          Build in Debug mode (default: Release)"
            echo "  --no-extension   Skip building the Java extension"
            echo "  --no-runtime     Skip building the C++ runtime"
            echo "  --no-tests       Skip building tests"
            echo "  --no-run-tests   Build tests but don't run them"
            echo "  --help,-h        Show this help message"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Run main function
main