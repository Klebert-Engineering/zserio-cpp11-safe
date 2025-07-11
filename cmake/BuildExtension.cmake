# BuildExtension.cmake
# Module to build the Zserio C++11 Safe Java extension

include(ExternalProject)

function(build_zserio_extension)
    # Check for Java
    find_package(Java COMPONENTS Runtime REQUIRED)
    
    # Check for Ant
    find_program(ANT_EXECUTABLE
        NAMES ant
        DOC "Apache Ant build tool"
    )
    
    if(NOT ANT_EXECUTABLE)
        message(FATAL_ERROR "Apache Ant not found. Please install Ant or set ANT_EXECUTABLE.")
    endif()
    
    message(STATUS "Found Ant: ${ANT_EXECUTABLE}")
    message(STATUS "Found Java: ${Java_JAVA_EXECUTABLE}")
    
    # Set up paths
    set(EXTENSION_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    set(EXTENSION_BUILD_DIR "${CMAKE_BINARY_DIR}/extension")
    
    # Check if zserio core needs to be extracted
    set(ZSERIO_ZIP "${EXTENSION_SOURCE_DIR}/dep/zserio-2.16.1-bin.zip")
    set(ZSERIO_EXTRACT_DIR "${CMAKE_BINARY_DIR}/zserio-2.16.1")
    
    if(NOT EXISTS "${ZSERIO_CORE_JAR}")
        if(EXISTS "${ZSERIO_ZIP}")
            message(STATUS "Extracting zserio core from ${ZSERIO_ZIP}")
            # Create the extraction directory
            file(MAKE_DIRECTORY "${ZSERIO_EXTRACT_DIR}")
            # Extract the zip file
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E tar xf "${ZSERIO_ZIP}"
                WORKING_DIRECTORY "${ZSERIO_EXTRACT_DIR}"
                RESULT_VARIABLE extract_result
            )
            if(NOT extract_result EQUAL 0)
                message(FATAL_ERROR "Failed to extract zserio core zip")
            endif()
            # Check if the JAR was extracted
            if(NOT EXISTS "${ZSERIO_CORE_JAR}")
                message(FATAL_ERROR "Failed to find zserio_core.jar after extraction")
            endif()
        else()
            message(FATAL_ERROR "Zserio core JAR not found at ${ZSERIO_CORE_JAR} and no zip file available at ${ZSERIO_ZIP}")
        endif()
    endif()
    
    # Use ExternalProject to build the extension
    ExternalProject_Add(zserio_extension
        SOURCE_DIR "${EXTENSION_SOURCE_DIR}"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${ANT_EXECUTABLE}
            -f "${EXTENSION_SOURCE_DIR}/build.xml"
            -Dzserio_core.jar_file="${ZSERIO_CORE_JAR}"
            jar_without_javadocs
        BUILD_IN_SOURCE TRUE
        INSTALL_COMMAND ""
    )
    
    # Create a target that other targets can depend on
    add_custom_target(extension_built ALL
        DEPENDS zserio_extension
    )
    
    # Set properties for other CMake files
    set_property(GLOBAL PROPERTY ZSERIO_EXTENSION_JAR "${ZSERIO_EXTENSION_JAR}")
    set_property(GLOBAL PROPERTY ZSERIO_CORE_JAR "${ZSERIO_CORE_JAR}")
    
    # Export variables to parent scope
    set(ZSERIO_EXTENSION_JAR "${ZSERIO_EXTENSION_JAR}" PARENT_SCOPE)
    set(ZSERIO_CORE_JAR "${ZSERIO_CORE_JAR}" PARENT_SCOPE)
    
    message(STATUS "Extension JAR will be built by ant at: ${ZSERIO_EXTENSION_JAR}")
endfunction()

# Function to check if extension is built
function(check_extension_built)
    if(NOT EXISTS "${ZSERIO_EXTENSION_JAR}")
        message(FATAL_ERROR "Zserio extension JAR not found at ${ZSERIO_EXTENSION_JAR}. Build the extension first.")
    endif()
    if(NOT EXISTS "${ZSERIO_CORE_JAR}")
        message(FATAL_ERROR "Zserio core JAR not found at ${ZSERIO_CORE_JAR}")
    endif()
endfunction()