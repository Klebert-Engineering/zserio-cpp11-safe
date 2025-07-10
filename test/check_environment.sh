#!/bin/bash

# Environment Check Script for Zserio C++11-Safe Extension
# This script verifies that all required tools are installed

echo "=========================================="
echo "Zserio C++11-Safe Build Environment Check"
echo "=========================================="
echo

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Track if all requirements are met
ALL_GOOD=1

# Function to check if a command exists
check_command() {
    local CMD=$1
    local MIN_VERSION=$2
    local VERSION_CMD=$3
    local DESCRIPTION=$4
    
    if command -v "$CMD" &> /dev/null; then
        if [ ! -z "$VERSION_CMD" ]; then
            VERSION=$($VERSION_CMD 2>&1 | head -1)
            echo -e "${GREEN}✓${NC} $DESCRIPTION: Found ($VERSION)"
        else
            echo -e "${GREEN}✓${NC} $DESCRIPTION: Found"
        fi
    else
        echo -e "${RED}✗${NC} $DESCRIPTION: Not found"
        ALL_GOOD=0
    fi
}

# Function to check Java version
check_java() {
    if command -v java &> /dev/null; then
        JAVA_VERSION=$(java -version 2>&1 | grep version | awk '{print $3}' | tr -d '"')
        JAVA_MAJOR=$(echo $JAVA_VERSION | cut -d. -f1)
        
        # Handle different Java version formats (1.8 vs 8)
        if [ "$JAVA_MAJOR" = "1" ]; then
            JAVA_MAJOR=$(echo $JAVA_VERSION | cut -d. -f2)
        fi
        
        if [ "$JAVA_MAJOR" -ge 8 ]; then
            echo -e "${GREEN}✓${NC} Java: Found version $JAVA_VERSION"
        else
            echo -e "${YELLOW}⚠${NC}  Java: Found version $JAVA_VERSION (version 8+ recommended)"
        fi
    else
        echo -e "${RED}✗${NC} Java: Not found (required)"
        ALL_GOOD=0
    fi
}

# Function to check CMake version
check_cmake() {
    if command -v cmake &> /dev/null; then
        CMAKE_VERSION=$(cmake --version | head -1 | awk '{print $3}')
        CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d. -f1)
        CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d. -f2)
        
        if [ "$CMAKE_MAJOR" -gt 3 ] || ([ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -ge 15 ]); then
            echo -e "${GREEN}✓${NC} CMake: Found version $CMAKE_VERSION"
        else
            echo -e "${YELLOW}⚠${NC}  CMake: Found version $CMAKE_VERSION (version 3.15+ required)"
            ALL_GOOD=0
        fi
    else
        echo -e "${RED}✗${NC} CMake: Not found (required)"
        ALL_GOOD=0
    fi
}

# Function to check C++ compiler
check_cpp_compiler() {
    CXX_FOUND=0
    
    # Check for g++
    if command -v g++ &> /dev/null; then
        GCC_VERSION=$(g++ --version | head -1)
        echo -e "${GREEN}✓${NC} g++: Found ($GCC_VERSION)"
        CXX_FOUND=1
    fi
    
    # Check for clang++
    if command -v clang++ &> /dev/null; then
        CLANG_VERSION=$(clang++ --version | head -1)
        echo -e "${GREEN}✓${NC} clang++: Found ($CLANG_VERSION)"
        CXX_FOUND=1
    fi
    
    # Check for cl.exe (MSVC)
    if command -v cl &> /dev/null; then
        MSVC_VERSION=$(cl 2>&1 | head -1)
        echo -e "${GREEN}✓${NC} MSVC: Found ($MSVC_VERSION)"
        CXX_FOUND=1
    fi
    
    if [ $CXX_FOUND -eq 0 ]; then
        echo -e "${RED}✗${NC} C++ Compiler: No supported compiler found (g++, clang++, or MSVC required)"
        ALL_GOOD=0
    fi
}

# Run checks
echo "Checking required tools..."
echo

check_java
check_command "ant" "" "ant -version" "Apache Ant"
check_cmake
check_cpp_compiler
check_command "make" "" "make --version" "Make"
check_command "unzip" "" "" "Unzip"

echo
echo "Checking optional tools..."
echo

check_command "nproc" "" "" "nproc (for parallel builds)"
check_command "git" "" "git --version" "Git"

# Platform-specific checks
echo
echo "Platform information:"
echo -e "  OS: $(uname -s)"
echo -e "  Architecture: $(uname -m)"

# Final summary
echo
echo "=========================================="
if [ $ALL_GOOD -eq 1 ]; then
    echo -e "${GREEN}All required tools are installed!${NC}"
    echo "You can run: ./test/build_all.sh"
else
    echo -e "${RED}Some required tools are missing!${NC}"
    echo
    echo "Installation hints:"
    echo "  macOS:   brew install openjdk ant cmake"
    echo "  Ubuntu:  sudo apt-get install default-jdk ant cmake build-essential"
    echo "  CentOS:  sudo yum install java-1.8.0-openjdk ant cmake gcc-c++"
    echo "  Windows: Install JDK, Ant, CMake, and Visual Studio or MinGW"
fi
echo "=========================================="

exit $((1 - ALL_GOOD))