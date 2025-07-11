cmake_minimum_required(VERSION 3.15)

# Project definition
project(zserio-cpp11-safe 
    VERSION 1.2.1
    DESCRIPTION "C++11 Safe Extension for Zserio"
    LANGUAGES CXX
)

# Options
option(BUILD_EXTENSION "Build the Java extension (requires Java and Ant)" ON)
option(BUILD_RUNTIME "Build the C++ runtime library" ON)
option(BUILD_TESTS "Build tests" ON)
option(RUN_TESTS "Run tests after building" ON)
option(USE_CCACHE "Use ccache if available" ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Module path for custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Use ccache if available
if(USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    endif()
endif()

# Global properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Extension JAR and core locations
# Ant build.xml will put the JAR in its default location
get_filename_component(ANT_BUILD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(ZSERIO_EXTENSION_JAR "${ANT_BUILD_ROOT}/build/compiler/extensions/cpp/11/jar/zserio_cpp.jar")
set(ZSERIO_CORE_DIR "${CMAKE_BINARY_DIR}/zserio-2.16.1")
set(ZSERIO_CORE_JAR "${ZSERIO_CORE_DIR}/zserio_libs/zserio_core.jar")

# Build the Java extension if requested
if(BUILD_EXTENSION)
    include(BuildExtension)
    build_zserio_extension()
endif()

# Build the C++ runtime
if(BUILD_RUNTIME)
    add_subdirectory(runtime)
endif()

# Include the code generator module
if(BUILD_EXTENSION OR EXISTS "${ZSERIO_EXTENSION_JAR}")
    include(ZserioGenerator)
endif()

# Build tests
if(BUILD_TESTS)
    enable_testing()
    
    # Check if we have the extension built
    if(NOT EXISTS "${ZSERIO_EXTENSION_JAR}" AND NOT BUILD_EXTENSION)
        message(WARNING "Extension JAR not found and BUILD_EXTENSION is OFF. Tests will not be built.")
    else()
        # Mini test
        add_subdirectory(test/mini)
        
        # Add more tests here as needed
    endif()
endif()

# No installation needed - this is a unified build system
# The runtime library and tests are built together

# Status messages
message(STATUS "")
message(STATUS "C++11 Safe Extension Configuration:")
message(STATUS "  CMake version:     ${CMAKE_VERSION}")
message(STATUS "  C++ compiler:      ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ standard:      C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build extension:   ${BUILD_EXTENSION}")
message(STATUS "  Build runtime:     ${BUILD_RUNTIME}")
message(STATUS "  Build tests:       ${BUILD_TESTS}")
message(STATUS "  Run tests:         ${RUN_TESTS}")
message(STATUS "")

# Print usage instructions
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(STATUS "Build with: cmake --build ${CMAKE_BINARY_DIR}")
    if(BUILD_TESTS AND RUN_TESTS)
        message(STATUS "Run tests with: ctest --test-dir ${CMAKE_BINARY_DIR}")
    endif()
    message(STATUS "")
endif()