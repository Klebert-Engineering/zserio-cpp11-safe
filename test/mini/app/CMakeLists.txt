cmake_minimum_required(VERSION 3.15)
project(MiniTest CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find the zserio runtime library
find_library(ZSERIO_RUNTIME_LIB 
    NAMES ZserioCppRuntime
    PATHS 
        ${CMAKE_CURRENT_SOURCE_DIR}/../../build/runtime/runtime
        ${CMAKE_CURRENT_SOURCE_DIR}/../../build/runtime
    NO_DEFAULT_PATH
)

if(NOT ZSERIO_RUNTIME_LIB)
    message(FATAL_ERROR "Zserio runtime library not found. Build runtime first.")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../build/mini/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../runtime/src
)

# Source files
set(SOURCES
    main.cpp
    MiniTest.cpp
)

# Add all generated source files
file(GLOB GENERATED_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/../../build/mini/generated/minizs/*.cpp"
)

# Create executable
add_executable(mini_test ${SOURCES} ${GENERATED_SOURCES})

# Link with runtime library
target_link_libraries(mini_test ${ZSERIO_RUNTIME_LIB})

# Enable warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mini_test PRIVATE -Wall -Wextra -Werror)
elseif(MSVC)
    target_compile_options(mini_test PRIVATE /W4 /WX)
endif()