# Zserio C++ runtime library.
#
# This CMake file defines a static library target named ZserioCppRuntime that contains the zserio C++ runtime.
#
# This CMake file is designed to be included directly without any further dependencies.
#
# An example application CMakeLists.txt follows:
#
# project(ZserioTestApp)
#
# add_subdirectory(runtime_libs/cpp ZserioCppRuntime)
#
# add_executable(ZserioTestApp HelloWorld.cpp)
# target_link_libraries(ZserioTestApp ZserioCppRuntime)

cmake_minimum_required(VERSION 3.15.0)

project(ZserioCppRuntime)

# Option for unsafe features
option(ZSERIO_CPP11_UNSAFE "Enable unsafe development features (NOT FOR PRODUCTION)" OFF)

set(ZSERIO_CPP_RUNTIME_LIB_SRCS
    zserio/pmr/AnyHolder.h
    zserio/pmr/ArrayTraits.h
    zserio/pmr/BitBuffer.h
    zserio/pmr/HeapOptionalHolder.h
    zserio/pmr/IService.h
    zserio/pmr/Map.h
    zserio/pmr/MemoryResource.cpp
    zserio/pmr/MemoryResource.h
    zserio/pmr/NewDeleteResource.cpp
    zserio/pmr/NewDeleteResource.h
    zserio/pmr/PolymorphicAllocator.h
    zserio/pmr/Set.h
    zserio/pmr/String.h
    zserio/pmr/UniquePtr.h
    zserio/pmr/Vector.h
    zserio/AllocatorHolder.h
    zserio/AllocatorPropagatingCopy.h
    zserio/AnyHolder.h
    zserio/Array.h
    zserio/ArrayTraits.h
    zserio/BitBuffer.h
    zserio/BitFieldUtil.cpp
    zserio/BitFieldUtil.h
    zserio/BitPositionUtil.h
    zserio/BitSizeOfCalculator.cpp
    zserio/BitSizeOfCalculator.h
    zserio/BitStreamReader.cpp
    zserio/BitStreamReader.h
    zserio/BitStreamWriter.cpp
    zserio/BitStreamWriter.h
    zserio/BuiltInOperators.cpp
    zserio/BuiltInOperators.h
    zserio/ConstraintException.h
    zserio/CppRuntimeException.cpp
    zserio/CppRuntimeException.h
    zserio/CppRuntimeVersion.h
    zserio/DeltaContext.h
    zserio/DeprecatedAttribute.h
    zserio/Enums.h
    zserio/ErrorCode.h
    zserio/FileUtil.cpp
    zserio/FileUtil.h
    zserio/FloatUtil.cpp
    zserio/FloatUtil.h
    zserio/HashCodeUtil.h
    zserio/IPubsub.h
    zserio/IService.h
    zserio/ISqliteDatabase.h
    zserio/ISqliteDatabaseReader.h
    zserio/IValidationObserver.h
    zserio/NoInit.h
    zserio/OptionalHolder.h
    zserio/ParsingInfo.h
    zserio/PubsubException.h
    zserio/RebindAlloc.h
    zserio/Result.h
    zserio/RuntimeArch.h
    zserio/SerializeUtil.h
    zserio/ServiceException.h
    zserio/SizeConvertUtil.cpp
    zserio/SizeConvertUtil.h
    zserio/Span.h
    zserio/SqliteConnection.h
    zserio/SqliteException.h
    zserio/SqliteFinalizer.h
    zserio/String.h
    zserio/StringConvertUtil.h
    zserio/StringView.h
    zserio/Traits.h
    zserio/Types.h
    zserio/UniquePtr.h
    zserio/ValidationSqliteUtil.h
    zserio/Vector.h
)

# Conditionally add unsafe sources
if(ZSERIO_CPP11_UNSAFE)
    message(WARNING "")
    message(WARNING "================================================")
    message(WARNING "UNSAFE EXTENSIONS ENABLED - NOT FOR PRODUCTION!")
    message(WARNING "This build includes non-functional-safety-ready code!")
    message(WARNING "================================================")
    message(WARNING "")
    
    list(APPEND ZSERIO_CPP_RUNTIME_LIB_SRCS
        # JSON support
        zserio/unsafe/JsonEncoder.h
        zserio/unsafe/JsonEncoder.cpp
        zserio/unsafe/JsonDecoder.h
        zserio/unsafe/JsonParser.h
        zserio/unsafe/JsonReader.h
        zserio/unsafe/JsonTokenizer.h
        zserio/unsafe/JsonTokenizer.cpp
        zserio/unsafe/JsonWriter.h
        # Reflection support
        zserio/unsafe/IReflectable.h
        zserio/unsafe/Reflectable.h
        zserio/unsafe/ReflectableUtil.h
        # Type information
        zserio/unsafe/ITypeInfo.h
        zserio/unsafe/TypeInfo.h
        zserio/unsafe/TypeInfoUtil.h
        zserio/unsafe/TypeInfoUtil.cpp
        # Walker functionality
        zserio/unsafe/Walker.h
        zserio/unsafe/WalkerConst.h
        zserio/unsafe/IWalkObserver.h
        zserio/unsafe/IWalkFilter.h
        # Debug/development
        zserio/unsafe/DebugStringUtil.h
        zserio/unsafe/ZserioTreeCreator.h
        zserio/unsafe/ZserioTreeCreator.cpp
        # PMR unsafe
        zserio/unsafe/pmr/IReflectable.h
        zserio/unsafe/pmr/Reflectable.h
        zserio/unsafe/pmr/ITypeInfo.h
    )
endif()

add_library(${PROJECT_NAME} STATIC ${ZSERIO_CPP_RUNTIME_LIB_SRCS})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/zserio FILES ${ZSERIO_CPP_RUNTIME_LIB_SRCS})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(ZSERIO_CPP11_UNSAFE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ZSERIO_CPP11_UNSAFE_MODE)
endif()