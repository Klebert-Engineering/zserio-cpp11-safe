# Mini test CMakeLists.txt

# Generate code from schema
zserio_generate_cpp11safe(
    TARGET minizs
    SCHEMA schema/minizs.zs
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated"
    WITHOUT_SOURCES_AMALGAMATION
)

# Add the test application
add_subdirectory(app)

# Create test target
add_test(
    NAME mini_test
    COMMAND mini_test
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)