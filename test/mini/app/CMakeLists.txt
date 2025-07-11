# Mini test application

# Source files for the test
set(SOURCES
    main.cpp
    MiniTest.cpp
)

# Create test executable
add_executable(mini_test ${SOURCES})

# Link with generated code and runtime
target_link_libraries(mini_test PRIVATE minizs)

# Enable warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mini_test PRIVATE -Wall -Wextra -Werror)
elseif(MSVC)
    target_compile_options(mini_test PRIVATE /W4 /WX)
endif()

# Set runtime path for installed binary
set_target_properties(mini_test PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
)