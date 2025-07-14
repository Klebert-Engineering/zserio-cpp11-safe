# C++11 Safe Generator for Zserio

The C++11 Safe extension is a specialized variant of zserio's C++ support, designed specifically for use in functionally critical systems where reliability and safety are paramount. Unlike the standard C++ extension, this variant provides **exception-free operation** - a critical requirement for safety-critical applications where exception handling may introduce unpredictability or is prohibited by coding standards.

## Key Features

- **Exception-Free Design**: All error handling is performed without throwing exceptions, making it suitable for safety-critical systems
- **Functional Safety Focus**: Designed to meet the stringent requirements of functional safety standards
- **Self-Contained Runtime**: Includes its own runtime library optimized for safety-critical applications

This extension generates C++ [serialization API](#serialization-api) from the Zserio schema together with [additional API](#additional-api), providing the same functionality as the standard C++ extension but with safety-critical optimizations.

## Repository Structure

```
safe-main/
├── src/             # Java extension (compiler plugin)
├── runtime/         # C++ runtime library
│   ├── src/         # Runtime source code
│   └── test/        # Runtime unit tests
├── freemarker/      # Code generation templates
├── test/            # Integration tests
├── cmake/           # CMake modules and helpers
├── dep/             # Bundled dependencies
└── build_and_test.bash  # Quick build script
```

### Key Components

- **Java Extension** (`src/`): Compiler plugin that generates C++ code from Zserio schemas
- **C++ Runtime** (`runtime/`): Exception-free runtime library for safety-critical systems
- **Templates** (`freemarker/`): FreeMarker templates that define generated code structure
- **Build System**: CMake-based with convenient wrapper scripts

## Building and Using This Extension

This extension provides its own C++ runtime library specifically designed for safety-critical applications. Unlike the standard zserio C++ extension, all components (compiler plugin and runtime) are self-contained within this repository.

The extension uses `-cpp11safe` as its command line option (instead of `-cpp`) to distinguish it from the standard C++ extension.

> **Development Note**: At this early stage, the repository is intentionally self-contained to streamline development. It includes a bundled zserio release (normally from [zserio releases](https://github.com/ndsev/zserio/releases)) and doesn't rely on external tools like [zserio-cmake-helper](https://github.com/Klebert-engineering/zserio-cmake-helper), which would require adaptation for this custom extension. Once the extension matures, these dependencies will be properly externalized.

### Prerequisites

#### Required Tools

- **Java 8 or higher** - Required for building and running the compiler plugin
- **Apache Ant 1.10+** - Required for building the Java extension
- **CMake 3.15+** - Required for building the C++ runtime library
<!-- TODO: Update these minimum versions to versions as new as possible -->
- **C++11 compatible compiler** - One of the following:
  - GCC 4.8+
  - Clang 3.3+
  - Apple Clang (Xcode 10+)
  - MSVC 2017+
  - MinGW 7.5.0+

#### Supported Platforms

- 64-bit Linux
- 64-bit Windows
- macOS (with Xcode Command Line Tools)

#### Platform-Specific Requirements

- **macOS**: Xcode Command Line Tools required
  ```bash
  xcode-select --install
  ```
- **Linux**: build-essential package required
  ```bash
  sudo apt-get install build-essential
  ```
- **Windows**: Visual Studio 2017+ with C++ workload or MinGW-w64

#### Quick Environment Check

Before building, verify your environment has all required tools:

```bash
./test/check_environment.sh
```

This script will check for all prerequisites and report any missing dependencies.

#### Installation Examples

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install default-jdk ant cmake build-essential
```

**macOS (with Homebrew):**
```bash
brew install openjdk ant cmake
```

**CentOS/RHEL/Fedora:**
```bash
sudo yum install java-1.8.0-openjdk-devel ant cmake gcc-c++
# or for newer versions:
sudo dnf install java-11-openjdk-devel ant cmake gcc-c++
```

**Windows:**
- Install [Java JDK](https://adoptium.net/)
- Install [Apache Ant](https://ant.apache.org/bindownload.cgi)
- Install [CMake](https://cmake.org/download/)
- Install Visual Studio 2017+ with C++ workload or [MinGW-w64](https://www.mingw-w64.org/)

> **Note:** The C++11 Safe extension generates code compliant with the C++11 standard (ISO/IEC 14882:2011). Although newer compilers are not explicitly tested, they should work as long as they maintain backward compatibility with C++11.

### Building the Extension

#### Quick Build (Recommended)

The easiest way to build everything is using the provided build script:

```bash
./build_and_test.bash --clean
```

Make sure to run a clean build the first time after cloning the repository. After that you can omit the `--clean` option.

This will:
- Check all prerequisites (Java, Ant, CMake, C++ compiler)
- Build the Java extension (compiler plugin)
- Build the C++ runtime library
- Generate code from test schemas
- Build and run tests

For more control, you can use options:
```bash
./build_and_test.bash --clean          # Clean build
./build_and_test.bash --help           # Show all options
./build_and_test.bash --debug          # Debug build
./build_and_test.bash --no-tests       # Skip tests
./build_and_test.bash --no-extension   # Skip Java build (if already built)
```

#### Manual Build with CMake

Alternatively, use the standard CMake workflow:

```bash
mkdir build && cd build
cmake ..                    # Configure
cmake --build . --parallel  # Build
ctest                       # Run tests
```

CMake options:
- `-DBUILD_EXTENSION=ON/OFF` - Build the Java extension (default: ON)
- `-DBUILD_RUNTIME=ON/OFF` - Build the C++ runtime (default: ON)
- `-DBUILD_TESTS=ON/OFF` - Build tests (default: ON)
- `-DCMAKE_BUILD_TYPE=Debug/Release` - Build type (default: Release)

### Using the Extension

There are three ways to use this C++11-safe extension:

**Note:** After building with the new CMake system, the extension JAR will be at `build/compiler/extensions/cpp/11/jar/zserio_cpp.jar` and the runtime library at `build/lib/libZserioCppRuntime.a`.

After building both components, you can use the extension directly with the Java command:

```bash
java -cp "path/to/zserio_core.jar:path/to/zserio_cpp.jar" \
     zserio.tools.ZserioTool \
     -cpp11safe <output_dir> \
     -src <source_dir> \
     <schema.zs>
```

Example:
```bash
java -cp "build/zserio-2.16.1/zserio_libs/zserio_core.jar:build/compiler/extensions/cpp/11/jar/zserio_cpp.jar" \
     zserio.tools.ZserioTool \
     -cpp11safe generated \
     -src schemas \
     myschema.zs
```

### Available Options

The C++11 Safe extension supports the following command line options:

#### Main Option
- **`-cpp11safe <outputDir>`** - Generate C++11-safe sources in the specified output directory

#### Allocator Configuration
- **`-setCppAllocator <allocator>`** - Set allocator type for generated code
  - `std` (default) - Use standard allocator
  - `polymorphic` - Use propagating polymorphic allocator. Enables use of custom memory management through polymorphic allocators inspired by C++17's `std::pmr::polymorphic_allocator`. See [Polymorphic Allocators Tutorial](https://github.com/ndsev/zserio-tutorial-cpp/tree/master/pmr) for details (note: tutorial uses standard C++11 extension, but the mechanism is the same for this C++11-safe extension).

#### Code Generation Options

##### Reflection and Type Information
- **`-withTypeInfoCode`** / **`-withoutTypeInfoCode`** - Enable/disable type information code (default: disabled)
  - Generates static method `typeInfo()` in all generated types
  - Returns static information like schema names, field types, optional status, etc.
  - Required prerequisite for reflection code

- **`-withReflectionCode`** / **`-withoutReflectionCode`** - Enable/disable reflection code generation (default: disabled)
  - Requires `-withTypeInfoCode` to be enabled
  - Generates `reflectable()` method for structures, choices, unions, and bitmasks
  - Generates `enumReflectable()` for enumerations
  - Enables generic access to Zserio objects (e.g., `getField()` by schema name)
  - Enables JSON export/import functionality via `zserio::toJsonString()` and `zserio::fromJsonString()`

##### Validation and Safety
- **`-withRangeCheckCode`** / **`-withoutRangeCheckCode`** - Enable/disable range checking code (default: disabled)
  - Generates explicit range checks for integer fields before writing
  - Useful for types like `bit:4` mapped to `uint8_t` to ensure values are in range `<0, 15>`

- **`-withValidationCode`** / **`-withoutValidationCode`** - Enable/disable validation code (default: disabled)
  - Generates `validate()` method for SQL databases
  - Validates SQL table schemas match Zserio definitions
  - Checks all column values for validity (ranges, enums, bitmasks)
  - Ensures blobs can be parsed successfully

##### Code Generation Control
- **`-withWriterCode`** / **`-withoutWriterCode`** - Enable/disable serialization code (default: enabled)
  - Controls generation of write/serialization methods
  - When enabled, automatically enables setter methods

- **`-withSettersCode`** / **`-withoutSettersCode`** - Enable/disable setter methods (default: enabled)
  - Controls generation of setter methods for fields
  - Automatically enabled when `-withWriterCode` is used

- **`-withCodeComments`** / **`-withoutCodeComments`** - Enable/disable code comments (default: disabled)
  - Generates Doxygen-style documentation comments
  - Incorporates comments from Zserio schema into generated code

- **`-withSourcesAmalgamation`** / **`-withoutSourcesAmalgamation`** - Enable/disable source amalgamation (default: enabled)
  - Controls whether generated sources are amalgamated into fewer files

- **`-withParsingInfoCode`** / **`-withoutParsingInfoCode`** - Enable/disable parsing info code [experimental] (default: disabled)
  - Generates additional parsing information (not part of stable API)

##### Service and Communication
- **`-withPubsubCode`** / **`-withoutPubsubCode`** - Enable/disable publish-subscribe code (default: disabled)
  - Generates code for publish-subscribe communication patterns

- **`-withServiceCode`** / **`-withoutServiceCode`** - Enable/disable service code generation (default: disabled)
  - Generates code for RPC-style services

- **`-withSqlCode`** / **`-withoutSqlCode`** - Enable/disable SQL database code (default: disabled)
  - Generates code for SQL database integration

#### Example Usage
```bash
java -jar zserio.jar \
    -cpp11safe ./generated \
    -setCppAllocator polymorphic \
    -withReflectionCode \
    -withTypeInfoCode \
    -withValidationCode \
    -withRangeCheckCode \
    -src ./schemas \
    schema.zs
```


## Content

[Repository Structure](#repository-structure)

&nbsp; &nbsp; &nbsp; &nbsp; [Key Components](#key-components)

[Building and Using This Extension](#building-and-using-this-extension)

&nbsp; &nbsp; &nbsp; &nbsp; [Prerequisites](#prerequisites)

&nbsp; &nbsp; &nbsp; &nbsp; [Building the Extension](#building-the-extension)

&nbsp; &nbsp; &nbsp; &nbsp; [Using the Extension](#using-the-extension)

&nbsp; &nbsp; &nbsp; &nbsp; [Available Options](#available-options)

[Serialization API](#serialization-api)

&nbsp; &nbsp; &nbsp; &nbsp; [Ordering Rules](#ordering-rules)

[Using Zserio CMake Helper](#using-zserio-cmake-helper)

[Functional Safety](#functional-safety)

[Compatibility Check](#compatibility-check)

[Optimizations](#optimizations)

## Serialization API

The serialization API provides the following features for all Zserio structures, choices and unions:

- Full [`std::allocator`](https://en.cppreference.com/w/cpp/memory/allocator) support in constructors.
- Serialization of all Zserio objects to the bit stream
  (method [`zserio::serialize()`](https://zserio.org/doc/runtime/latest/cpp/SerializeUtil_8h.html)).
- Deserialization of all Zserio objects from the bit stream
  (method [`zserio::deserialize()`](https://zserio.org/doc/runtime/latest/cpp/SerializeUtil_8h.html)).
- Getters and setters for all fields
- Method `bitSizeOf()` which calculates a number of bits needed for serialization of the Zserio object.
- Comparison operator which compares two Zserio objects field by field.
- Less than operator which compares two Zserio objects field by field using the
  [Ordering Rules](#ordering-rules).
- Method `hashCode()` which calculates a hash code of the Zserio object.

### Ordering Rules

Both C++ runtime and generator provide `operator<` (in addition to `operator==`) on all objects which can
occur in generated API. Thus it's possible to easily use generated objects in `std::set` or `std::map`.

In general, all compound objects are compared lexicographically (inspired by
[lexicographical_compare](https://en.cppreference.com/w/cpp/algorithm/lexicographical_compare)):

* Parameters are compared first in order of definition.
* Fields are compared:
   * In case of [structures](https://github.com/ndsev/zserio/blob/master/doc/ZserioLanguageOverview.md#structure-type),
     all fields are compared in order of definition.
   * In case of [unions](https://github.com/ndsev/zserio/blob/master/doc/ZserioLanguageOverview.md#union-type),
     the `choiceTag` is compared first and then the selected field is compared.
   * In case of [choices](https://github.com/ndsev/zserio/blob/master/doc/ZserioLanguageOverview.md#choice-type),
     only the selected field is compared (if any).

Comparison of [optional fields](https://github.com/ndsev/zserio/blob/master/doc/ZserioLanguageOverview.md#optional-members)
(kept in `InplaceOptionalHolder` or `HeapOptionalHolder`):

* When both fields are present, they are compared.
* Otherwise the missing field is less than the other field if and only if the other field is present.
* If both fields are missing, they are equal.

> Note that same rules applies for
  [extended fields](https://github.com/ndsev/zserio/blob/master/doc/ZserioLanguageOverview.md#extended-members) and for fields in unions
  and choices, which are internally kept in `AnyHolder`.

Comparison of [arrays](https://github.com/ndsev/zserio/blob/master/doc/ZserioLanguageOverview.md#array-types)
(`Array`) uses native comparison of the underlying `std::vector`.

Comparison of [`extern` fields](https://github.com/ndsev/zserio/blob/master/doc/ZserioLanguageOverview.md#extern-type) (kept in `BitBuffer`):

* Compares byte by byte and follows the rules of
  [lexicographical compare](https://en.cppreference.com/w/cpp/algorithm/lexicographical_compare).
* The last byte is properly masked to use only the proper number of bits.

Comparison of [`bytes` fields](https://github.com/ndsev/zserio/blob/master/doc/ZserioLanguageOverview.md#bytes-type) uses native comparison
on the underlying `std::vector`.

## Using Zserio CMake Helper

This extension provides its own CMake helper that defines the custom function `zserio_generate_cpp`.
This function can be used for automatic generation of C++ sources from zserio schemas using the C++11 Safe extension.

### Prerequisites

* CMake 3.15+
* Java must be available - the function calls `find_package(JAVA java)`
* `ZSERIO_JAR_FILE` must be defined either as an environment or CMake variable

### Usage

    zserio_generate_cpp(
        TARGET <target>
        [SRC_DIR <directory>]
        [MAIN_ZS <file>]
        [GEN_DIR <directory>]
        [EXTRA_ARGS <argument>...]
        [GENERATED_SOURCES_VAR <variable>]
        [OUTPUT_VAR <variable>]
        [ERROR_VAR <variable>]
        [RESULT_VAR <variable>]
        [FORCE_REGENERATION]
        [CLEAN_GEN_DIR])

### Arguments

`TARGET`

Target to which the generated sources will be assigned.

`SRC_DIR`

Source directory for zserio schemas. Optional, defaults to `CMAKE_CURRENT_SOURCE_DIR`.

`MAIN_ZS`

Main zserio schema. Optional if the MAIN_ZS file is specified as a source for the given `TARGET`.

`GEN_DIR`

Directory where the C++ sources will be generated.

`EXTRA_ARGS`

Extra arguments to be passed to the Zserio tool.

`GENERATED_SOURCES_VAR`

The variable will be set with a list of generated source files (full paths). Optional.

`OUTPUT_VAR`

The variable will be set with the contents of the standard output pipe. Optional.
If not set, the standard output pipe is printed.

`ERROR_VAR`

The variable will be set with the contents of the standard error pipe. Optional.
If not set, the standard error pipe is printed.

`RESULT_VAR`

The variable will be set to contain the result of the zserio generator. Optional.
If not set, a `FATAL_ERROR` is raised in case of the zserio generator error.

`FORCE_RECONFIGURE`

Forces regeneration every time the CMake configure is run.

`CLEAN_GEN_DIR`

Cleans `GEN_DIR` when generation in CMake configure-time is run.

> Note that `OUTPUT_VAR` and `ERROR_VAR` can be set to the same value and then both pipes will be merged.

> Note that `OUTPUT_VAR`, `ERROR_VAR` and `RESULT_VAR` are updated only when the generation is executed within
> the configure-time - i.e. for the first time or when zserio schema sources are changed, etc.
> See "[How if works](#how-it-works)" for more info.

### Example

    set(CMAKE_MODULE_PATH "${ZSERIO_RELEASE}/cmake")
    set(ZSERIO_JAR_FILE "${ZSERIO_RELEASE}/zserio.jar")
    include(zserio_compiler)

    add_library(sample_zs sample.zs)
    zserio_generate_cpp(
        TARGET sample_zs
        GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)

### How it works

First time the CMake configure is run, the sources are generated using `execute_process` directly in
configure-time and auxiliary information (timestamps, list of generated sources, etc.) is stored in the
CMake cache. The auxiliary info is used to define a custom command which uses the same zserio command line
as the original `execute_process` and thus allows to re-generate sources when it's needed - e.g. after the
clean step.

The custom command is sufficient as long as the generated sources remains unchanged. Otherwise the
`execute_process` must be re-run in configure-time to ensure that all generated sources are collected correctly.
This functionality is achieved using the auxiliary information mentioned above.

List of generated sources can change in following situations:

- ZSERIO_JAR_FILE is changed
- Zserio schema sources are changed
- EXTRA_ARGS are changed

## Functional Safety

Zserio's C++ support is designed with a strong focus on functional safety, aiming to ensure the reliability,
integrity, and robustness of the system while reducing the risk of software-induced hazards. This section
provides an overview of the functional safety measures implemented, highlighting development practices that
contribute to the framework's safety and trustworthiness.

### C++ Runtime Library

The following describes features which minimize the risk of Zserio C++ runtime library malfunctioning behavior:

- Supported compilers (minimum versions): gcc 7.5.0, Clang 11.0.0, MinGW 7.5.0, MSVC 2017
- Warnings are treated as errors for all supported compilers
- All features are properly tested by [unit tests](runtime/test/) for all supported compilers (>600 tests)
- Implemented automatic test coverage threshold check using [llvm-cov](https://llvm.org/docs/CommandGuide/llvm-cov.html) and Clang 14.0.6 (see
  [coverage report](https://zserio.org/doc/runtime/latest/cpp/coverage/clang/index.html) which fulfills a line coverage threshold of 99%)
- AddressSanitizer is run with no findings
- UndefinedBehaviourSanitizer is run with no findings
- C++ runtime library sources are checked by static analysis tool clang-tidy version 14.0.6
- C++ runtime library sources are checked by [SonarCloud](https://sonarcloud.io/summary/new_code?id=ndsev_zserio)

#### Clang-tidy Usage

Clang-tidy tool is run using [this configuration](runtime/ClangTidyConfig.txt).
The clang-tidy report from the latest C++ runtime library is available [here](https://zserio.org/doc/runtime/latest/cpp/clang-tidy/clang-tidy-report.txt).

Due to compatibility and functional safety considerations, zserio is constrained to utilize the C++11 standard.
Consequently, certain clang-tidy findings remain unresolved at present. This is mainly attributed to
zserio's C++ runtime library, which operates at a lower level and emulates standard abstractions like
std::span or std::string_view introduced in C++17.

Therefore all clang-tidy findings have been carefully checked and filtered out using definitions in clang-tidy
[suppression file](runtime/ClangTidySuppressions.txt).
This suppression file contains as well the brief reasoning why these findings were not fixed. This solution
with suppression file has been chosen not to pollute C++ runtime sources with `// NOLINT` comments and to
allow implementation of warnings-as-error feature. The clang-tidy suppression file is automatically used
during compilation using CMake (see [CMake runtime configuration](runtime/CMakeLists.txt)).

### C++ Generated Code

The following describes features which minimize the risk of Zserio C++ generated code malfunctioning behavior:

- Supported compilers (minimum versions): gcc 7.5.0, clang 11.0.0, MinGW 7.5.0, MSVC 2017
- Warnings are treated as errors for all supported compilers
- All zserio language features are properly tested by [unit tests](https://github.com/ndsev/zserio/tree/master/test) for all supported compilers
  (>1700 tests)
- Unit tests check C++ code generated from small zserio schemas (>70 schemas)
- Generated sources are checked by static analysis tool clang-tidy version 14.0.6 using
  [this configuration](runtime/ClangTidyConfig.txt)
- Generated sources are checked by [SonarCloud](https://sonarcloud.io/summary/new_code?id=ndsev_zserio)

### Exception-Free Design

Unlike the standard zserio C++ extension, the C++11 Safe extension is designed to operate completely without exceptions. This is a fundamental requirement for many safety-critical systems where:

- Exception handling may introduce unpredictable timing behavior
- Coding standards (such as MISRA C++ or AUTOSAR) prohibit exception usage
- Deterministic error handling is required for certification

#### Error Handling Approach

Instead of throwing exceptions, the C++11 Safe extension uses:

1. **Return Codes**: All operations that can fail return error codes
2. **Error State Objects**: Complex operations use error state objects to provide detailed error information
3. **Compile-Time Validation**: Maximum use of compile-time checks to catch errors early
4. **Bounded Operations**: All operations have predictable resource usage and bounds

#### Comparison with Standard Extension

The standard zserio C++ runtime library may throw `zserio::CppRuntimeException` in various circumstances. The C++11 Safe extension handles these same error conditions without exceptions:

| Error Condition | Standard Extension | C++11 Safe Extension |
| --------------- | ------------------ | -------------------- |
| Buffer size exceeded limit | Throws `CppRuntimeException` | Returns error code `BUFFER_TOO_LARGE` |
| Wrong buffer bit size | Throws `CppRuntimeException` | Returns error code `INVALID_BUFFER_SIZE` |
| Invalid number of bits to read | Throws `CppRuntimeException` | Returns error code `INVALID_BIT_COUNT` |
| Reached end of stream | Throws `CppRuntimeException` | Returns error code `END_OF_STREAM` |
| VarSize value out of range | Throws `CppRuntimeException` | Returns error code `VARSIZE_OUT_OF_RANGE` |
| Optional field not present | Throws `CppRuntimeException` | Returns null/empty optional with status |
| Wrong offset value | Throws `CppRuntimeException` | Returns error code `OFFSET_MISMATCH` |
| Constraint violation | Throws `CppRuntimeException` | Returns error code `CONSTRAINT_VIOLATION` |
| No match in choice/union | Throws `CppRuntimeException` | Returns error code `INVALID_CHOICE` |
| Bitmask value out of bounds | Throws `CppRuntimeException` | Returns error code `BITMASK_OUT_OF_BOUNDS` |
| Unknown enumeration value | Throws `CppRuntimeException` | Returns error code `UNKNOWN_ENUM_VALUE` |

#### Usage Example

```cpp
// Standard extension (with exceptions)
try {
    MyStruct data = zserio::deserialize<MyStruct>(reader);
} catch (const zserio::CppRuntimeException& e) {
    // Handle error
}

// C++11 Safe extension (exception-free)
zserio::Result<MyStruct> result = zserio::deserialize<MyStruct>(reader);
if (!result.isSuccess()) {
    // Handle error using result.error()
    switch (result.error()) {
        case zserio::ErrorCode::END_OF_STREAM:
            // Handle end of stream
            break;
        // ... other error cases
    }
}
```


## Compatibility Check

C++ generator honors the `zserio_compatibility_version` specified in the schema. However note that only
the version specified in the root package of the schema is taken into account. The generator checks that
language features used in the schema are still encoded in a binary compatible way with the specified
compatibility version and fires an error when it detects any problem.

> Note: Binary encoding of packed arrays has been changed in version `2.5.0` and thus versions `2.4.x` are
binary incompatible with later versions.

## Optimizations

The C++ generator provides the following optimizations of the generated code:

- If any Zserio structure, choice or union type is not used in the packed array, no packing interface methods
  will be generated for them
  (e.g. `write(ZserioPackingContext& context, ::zserio::BitStreamWriter& out)`).

Such optimizations can be done because Zserio relays on the fact that the entire schema is known during the
generation. Therefore, splitting schema into two parts and generating them independently cannot guarantee
correct functionality. This can lead to a problem especially for templates, if a template is defined
in one part and instantiated in the other.
