# ZserioGenerator.cmake
# Module to generate C++11-safe code from Zserio schemas

# Function to generate C++ code from a .zs schema
function(zserio_generate_cpp11safe)
    set(options WITH_REFLECTION WITH_TYPE_INFO WITHOUT_SOURCES_AMALGAMATION)
    set(oneValueArgs TARGET SCHEMA OUTPUT_DIR)
    set(multiValueArgs IMPORT_DIRS EXTRA_ARGS)
    cmake_parse_arguments(ZSERIO "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Validate arguments
    if(NOT ZSERIO_TARGET)
        message(FATAL_ERROR "zserio_generate_cpp11safe: TARGET is required")
    endif()
    
    if(NOT ZSERIO_SCHEMA)
        message(FATAL_ERROR "zserio_generate_cpp11safe: SCHEMA is required")
    endif()
    
    if(NOT ZSERIO_OUTPUT_DIR)
        set(ZSERIO_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    endif()
    
    # Get absolute path to schema
    get_filename_component(ZSERIO_SCHEMA_ABS "${ZSERIO_SCHEMA}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    get_filename_component(SCHEMA_NAME "${ZSERIO_SCHEMA}" NAME_WE)
    get_filename_component(SCHEMA_DIR "${ZSERIO_SCHEMA_ABS}" DIRECTORY)
    
    # Check that schema exists
    if(NOT EXISTS "${ZSERIO_SCHEMA_ABS}")
        message(FATAL_ERROR "Schema file not found: ${ZSERIO_SCHEMA_ABS}")
    endif()
    
    # Get JAR paths
    get_property(extension_jar GLOBAL PROPERTY ZSERIO_EXTENSION_JAR)
    get_property(core_jar GLOBAL PROPERTY ZSERIO_CORE_JAR)
    
    if(NOT extension_jar OR NOT EXISTS "${extension_jar}")
        set(extension_jar "${ZSERIO_EXTENSION_JAR}")
    endif()
    
    if(NOT core_jar OR NOT EXISTS "${core_jar}")
        set(core_jar "${ZSERIO_CORE_JAR}")
    endif()
    
    # Verify core JAR exists (this should be extracted first)
    if(NOT EXISTS "${core_jar}")
        message(FATAL_ERROR "Core JAR not found: ${core_jar}")
    endif()
    
    # Extension JAR check is deferred - we'll add dependency instead
    
    # Build command line arguments
    set(ZSERIO_ARGS)
    
    # Add import directories - use the schema directory as the source
    list(APPEND ZSERIO_ARGS -src "${SCHEMA_DIR}")
    foreach(import_dir ${ZSERIO_IMPORT_DIRS})
        get_filename_component(import_dir_abs "${import_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        list(APPEND ZSERIO_ARGS -src "${import_dir_abs}")
    endforeach()
    
    # Add output directory
    list(APPEND ZSERIO_ARGS -cpp11safe "${ZSERIO_OUTPUT_DIR}")
    
    # Add optional flags
    if(ZSERIO_WITH_REFLECTION)
        list(APPEND ZSERIO_ARGS -withReflectionCode)
    endif()
    
    if(ZSERIO_WITH_TYPE_INFO)
        list(APPEND ZSERIO_ARGS -withTypeInfoCode)
    endif()
    
    if(ZSERIO_WITHOUT_SOURCES_AMALGAMATION)
        list(APPEND ZSERIO_ARGS -withoutSourcesAmalgamation)
    endif()
    
    # Add extra arguments
    if(ZSERIO_EXTRA_ARGS)
        list(APPEND ZSERIO_ARGS ${ZSERIO_EXTRA_ARGS})
    endif()
    
    # Add schema file (just the filename since src is set to SCHEMA_DIR)
    get_filename_component(SCHEMA_FILENAME "${ZSERIO_SCHEMA}" NAME)
    list(APPEND ZSERIO_ARGS "${SCHEMA_FILENAME}")
    
    # Create output directory
    file(MAKE_DIRECTORY "${ZSERIO_OUTPUT_DIR}")
    
    # Find Java
    find_package(Java COMPONENTS Runtime REQUIRED)
    
    # Check if we need to regenerate (schema changed or output doesn't exist)
    set(REGENERATE FALSE)
    if(NOT EXISTS "${ZSERIO_OUTPUT_DIR}/${SCHEMA_NAME}")
        set(REGENERATE TRUE)
    else()
        # Check if schema is newer than generated files
        file(GLOB GENERATED_FILES "${ZSERIO_OUTPUT_DIR}/${SCHEMA_NAME}/*")
        if(GENERATED_FILES)
            list(GET GENERATED_FILES 0 FIRST_GENERATED_FILE)
            if("${ZSERIO_SCHEMA_ABS}" IS_NEWER_THAN "${FIRST_GENERATED_FILE}")
                set(REGENERATE TRUE)
            endif()
        else()
            set(REGENERATE TRUE)
        endif()
    endif()
    
    # Check if extension JAR exists
    if(NOT EXISTS "${extension_jar}")
        # Extension not built yet - create a placeholder target
        message(STATUS "Extension JAR not found, deferring code generation for ${ZSERIO_SCHEMA}")
        add_library(${ZSERIO_TARGET} INTERFACE)
        add_custom_target(${ZSERIO_TARGET}_generate
            COMMAND ${CMAKE_COMMAND} -E echo "Skipping generation - extension not built yet"
        )
        target_include_directories(${ZSERIO_TARGET} INTERFACE
            $<BUILD_INTERFACE:${ZSERIO_OUTPUT_DIR}>
            $<INSTALL_INTERFACE:include>
        )
        target_link_libraries(${ZSERIO_TARGET} INTERFACE ZserioCppRuntime)
        # Set a property to indicate this needs generation later
        set_property(GLOBAL PROPERTY ZSERIO_NEEDS_GENERATION_${ZSERIO_TARGET} TRUE)
        return()
    endif()
    
    # REFACTORING MODE: Generation disabled - working with previously generated sources
    # TODO: Re-enable this once generator is adapted to work with refactored runtime
    if(FALSE) # DISABLED: if(REGENERATE)
        message(STATUS "Generating C++11-safe code from ${ZSERIO_SCHEMA}")
        
        # execute_process(
        #     COMMAND ${Java_JAVA_EXECUTABLE}
        #         -cp "${core_jar}:${extension_jar}"
        #         zserio.tools.ZserioTool
        #         ${ZSERIO_ARGS}
        #     WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        #     RESULT_VARIABLE GENERATION_RESULT
        #     OUTPUT_VARIABLE GENERATION_OUTPUT
        #     ERROR_VARIABLE GENERATION_ERROR
        # )
        # 
        # if(NOT GENERATION_RESULT EQUAL 0)
        #     message(FATAL_ERROR "Code generation failed:\n${GENERATION_ERROR}")
        # endif()
    endif()
    
    # In refactoring mode, we expect generated sources to already exist
    message(STATUS "REFACTORING MODE: Using existing generated sources for ${ZSERIO_SCHEMA} (generation disabled)")
    
    # Find generated source files
    file(GLOB GENERATED_CPP_SOURCES "${ZSERIO_OUTPUT_DIR}/${SCHEMA_NAME}/*.cpp")
    file(GLOB GENERATED_H_SOURCES "${ZSERIO_OUTPUT_DIR}/${SCHEMA_NAME}/*.h")
    
    # Create a dummy target for dependencies
    add_custom_target(${ZSERIO_TARGET}_generate
        DEPENDS ${GENERATED_CPP_SOURCES} ${GENERATED_H_SOURCES}
    )
    
    # Create library with generated sources
    if(GENERATED_CPP_SOURCES)
        # We have C++ sources, create a static library
        add_library(${ZSERIO_TARGET} STATIC ${GENERATED_CPP_SOURCES})
        # Set C++ standard for generated code
        target_compile_features(${ZSERIO_TARGET} PUBLIC cxx_std_11)
    else()
        # No C++ sources (maybe header-only?), create interface library
        add_library(${ZSERIO_TARGET} INTERFACE)
    endif()
    
    # Add dependency on the generate target
    add_dependencies(${ZSERIO_TARGET} ${ZSERIO_TARGET}_generate)
    
    # Add include directories - both for the target itself and for consumers
    if(GENERATED_CPP_SOURCES)
        target_include_directories(${ZSERIO_TARGET} PUBLIC
            $<BUILD_INTERFACE:${ZSERIO_OUTPUT_DIR}>
            $<INSTALL_INTERFACE:include>
        )
        # Link to runtime
        target_link_libraries(${ZSERIO_TARGET} PUBLIC ZserioCppRuntime)
    else()
        target_include_directories(${ZSERIO_TARGET} INTERFACE
            $<BUILD_INTERFACE:${ZSERIO_OUTPUT_DIR}>
            $<INSTALL_INTERFACE:include>
        )
        # Link to runtime
        target_link_libraries(${ZSERIO_TARGET} INTERFACE ZserioCppRuntime)
    endif()
    
    # Make sure extension is built first if we're building it
    if(TARGET extension_built)
        add_dependencies(${ZSERIO_TARGET}_generate extension_built)
    endif()
    
    # Export generated directory for other functions
    set_property(TARGET ${ZSERIO_TARGET} PROPERTY ZSERIO_GENERATED_DIR "${ZSERIO_OUTPUT_DIR}")
    set_property(TARGET ${ZSERIO_TARGET} PROPERTY ZSERIO_SCHEMA_NAME "${SCHEMA_NAME}")
endfunction()

# Helper function to add multiple schemas
function(zserio_add_schemas)
    set(options)
    set(oneValueArgs TARGET_PREFIX OUTPUT_DIR)
    set(multiValueArgs SCHEMAS IMPORT_DIRS OPTIONS)
    cmake_parse_arguments(SCHEMAS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    foreach(schema ${SCHEMAS_SCHEMAS})
        get_filename_component(schema_name "${schema}" NAME_WE)
        set(target_name "${SCHEMAS_TARGET_PREFIX}_${schema_name}")
        
        zserio_generate_cpp11safe(
            TARGET ${target_name}
            SCHEMA ${schema}
            OUTPUT_DIR ${SCHEMAS_OUTPUT_DIR}
            IMPORT_DIRS ${SCHEMAS_IMPORT_DIRS}
            ${SCHEMAS_OPTIONS}
        )
    endforeach()
endfunction()

# Function to get generated sources (for IDE integration)
function(zserio_get_generated_sources target out_var)
    get_property(gen_dir TARGET ${target} PROPERTY ZSERIO_GENERATED_DIR)
    get_property(schema_name TARGET ${target} PROPERTY ZSERIO_SCHEMA_NAME)
    
    if(gen_dir AND schema_name)
        file(GLOB_RECURSE generated_sources
            "${gen_dir}/${schema_name}/*.h"
            "${gen_dir}/${schema_name}/*.cpp"
        )
        set(${out_var} ${generated_sources} PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()