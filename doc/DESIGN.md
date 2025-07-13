# Towards C++11 Safe: A Functional Safety Roadmap for Zserio C++

## Executive Summary

This document outlines a comprehensive strategy to transform the Zserio C++11 extension into a functional safety compliant implementation. Building on insights from a previous proof-of-concept (POC), we propose a cleaner, systematic approach that addresses the critical requirements for safety-critical systems.

### Key Objectives

1. **Exception-Free Error Handling**: Replace exceptions with error codes using a `Result<T>` API that enforces error checking through `ZSERIO_NODISCARD` annotations and provides `noexcept` guarantees
2. **Deterministic Behavior**: No hidden allocations, no unpredictable control flow, no `std::abort()` calls
3. **Safe Memory Management**: Enforce usage of Zserio's PMR (polymorphic memory resource) interface for all dynamic allocations. Provide practical default implementations for development (with memory statistics and detected/controlled allocation failures) while requiring production systems to supply their own safety-certified memory resources
4. **Pluggable Container Support**: Allow users to substitute STL containers with functionally safe alternatives, as STL containers may call `std::abort()` when exceptions are disabled

### Implementation Status

- **Phase 1**: ✅ Complete - Unsafe extensions separated into `unsafe/` directory
- **Phase 2**: ✅ Complete - `Result<T>` pattern implemented and tested
- **Phase 3**: 🚧 In Progress - Runtime library conversion (need to propagate results for all cases and re-enable tests)
- **Phase 4**: 🔧 Code Generator Updates - Transform generator to produce functional safety compliant code
- **Phase 5**: 🔌 Planned - Pluggable container interface for functional safety replacements
- **Phase 6**: 📋 Planned - Memory management integration

## Phase 1: Unsafe Extensions Separation ✅

### Overview

All inherently unsafe features have been moved to a clearly marked `unsafe/` directory. This phase established a clear architectural boundary between functional safety ready code and development-only features.

### Unsafe Features Identified

- **JSON Support** - Dynamic allocation, unbounded operations
- **Reflection/Introspection** - Runtime type information, dynamic behavior
- **Type Info** - RTTI-like features incompatible with safety standards
- **Walker/Visitor** - Dynamic traversal with unbounded recursion
- **Tree Creator** - Dynamic object construction
- **Debug Utilities** - Development-only debugging aids

### Directory Structure

```
runtime/src/zserio/
├── BitStreamReader.h        # Core serialization (safe)
├── BitStreamWriter.h        # Core serialization (safe)
├── ErrorCode.h             # Error codes (safe)
├── Result.h                # Result<T> pattern (safe)
├── CppRuntimeException.h   # Remains here during Phase 1-2
└── unsafe/                 # NOT FOR PRODUCTION USE
    ├── JsonDecoder.h       # JSON support
    ├── IReflectable.h      # Reflection interfaces
    ├── Walker.h            # Object traversal
    └── ...                 # Other unsafe features
```

> **Note**: Exception classes remain in their current location during Phases 1-2. They will be relocated to `unsafe/` after Phase 3 runtime conversion.

### Build Configuration

```cmake
option(ZSERIO_CPP11_UNSAFE
       "UNSAFE: Enable development-only extensions - NEVER USE IN PRODUCTION!" OFF)

if(ZSERIO_CPP11_UNSAFE)
    message(WARNING "UNSAFE EXTENSIONS ENABLED - NOT FOR PRODUCTION!")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
endif()
```

## Phase 2: `Result<T>` Pattern Implementation ✅

### Design Choice: Simple Result<T> vs std::expected

We implemented our own minimal `Result<T>` pattern instead of backporting `std::expected` for:
- **Simplicity**: Easier to verify for functional safety certification
- **No monadic operations**: Avoids complex control flow that can obscure safety analysis
- **Focused API**: Only `isSuccess()`, `isError()`, `getError()`, and `getValue()`

### C++11 Compatible [[nodiscard]]

```cpp
#if __cplusplus >= 201703L
#define ZSERIO_NODISCARD [[nodiscard]]
#elif defined(__GNUC__) || defined(__clang__)
#define ZSERIO_NODISCARD __attribute__((warn_unused_result))
#else
#define ZSERIO_NODISCARD
#endif
```

**Important**: In C++11, we cannot reliably annotate the `Result<T>` class itself. Each function returning `Result<T>` must be individually annotated with `ZSERIO_NODISCARD`.

### Result<T> Implementation (Simplified)

```cpp
template <typename T = void>
class Result {
private:
    union {
        alignas(T) unsigned char valueStorage[sizeof(T)];
        ErrorCode error;
    } m_storage;
    bool m_hasValue;

public:
    // Move-only semantics
    Result(Result&&) = default;
    Result& operator=(Result&&) = default;
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    // Factory methods
    static Result success(T&& value) noexcept;
    static Result error(ErrorCode errorCode) noexcept;

    // Query methods
    ZSERIO_NODISCARD bool isSuccess() const noexcept;
    ZSERIO_NODISCARD bool isError() const noexcept;
    ZSERIO_NODISCARD ErrorCode getError() const noexcept;
    ZSERIO_NODISCARD const T& getValue() const & noexcept;
    ZSERIO_NODISCARD T&& getValue() && noexcept;
};
```

### Error Codes

The POC defined 84 comprehensive error codes. Key categories include:

```cpp
enum class ErrorCode : uint32_t {
    Success = 0,
    // Memory/Allocation (2-9)
    AllocationFailed = 2,
    BufferSizeExceeded = 4,
    // I/O Operations (10-19)
    EndOfStream = 10,
    InvalidBitPosition = 11,
    // Serialization (20-29)
    SerializationFailed = 20,
    VersionMismatch = 23,
    // Type/Value Errors (30-44)
    InvalidValue = 31,
    OutOfRange = 32,
    // ... total of 84 error codes
};
```

### Error Propagation

For functional safety compliance, we use explicit error handling without macros:

```cpp
// Explicit and traceable
auto result = someOperation();
if (result.isError()) {
    return Result<ReturnType>::error(result.getError());
}
auto value = result.getValue();
```

## Phase 3: Runtime Library Conversion 🚧

### Current Status

- Core `Result<T>` infrastructure complete
- BitStreamReader/Writer partially converted
- **TODO**: Complete error propagation for all code paths
- **TODO**: Re-enable and update runtime tests for Result<T> API

### Key Design Decisions

#### Move Semantics Requirement

Any type `T` used with `Result<T>` must support move semantics:

```cpp
// Enable move operations for Result<T> compatibility
BitStreamWriter(BitStreamWriter&&) = default;
BitStreamWriter& operator=(BitStreamWriter&&) = default;
```

#### Single API - No Dual Support

The safe runtime provides **only** Result<T>-based APIs with no exception-throwing variants.

#### Factory Pattern for Construction

```cpp
class MyStruct {
public:
    // Simple constructor - cannot fail
    explicit MyStruct(const allocator_type& allocator) noexcept;

    // Factory method for deserialization
    ZSERIO_NODISCARD static Result<MyStruct> deserialize(
        BitStreamReader& reader,
        const allocator_type& allocator = allocator_type()) noexcept;

    // Instance methods that can fail
    ZSERIO_NODISCARD Result<void> write(BitStreamWriter& writer) const noexcept;
};
```

### Core Classes Example

```cpp
class BitStreamReader {
public:
    // All operations return Result<T> and are noexcept
    ZSERIO_NODISCARD Result<uint32_t> readBits(uint8_t numBits) noexcept;
    ZSERIO_NODISCARD Result<int64_t> readVarInt() noexcept;

    template <typename ALLOC = std::allocator<char>>
    ZSERIO_NODISCARD Result<string<ALLOC>> readString(const ALLOC& alloc = ALLOC()) noexcept;
};
```

### Container Limitation

**Critical Issue**: STL containers may call `std::abort()` when exceptions are disabled:

```cpp
// WARNING: May abort on allocation failure with -fno-exceptions
m_data.reserve(size);  // May call std::abort()!
```

This is acknowledged but addressed in Phase 5 (pluggable containers).

## Phase 4: Code Generator Updates 🔧

### Overview

This phase transforms the Zserio code generator to produce functional safety compliant C++ code that uses the Result<T>-based runtime from Phase 3.

### Investigation Approach

As a first step, we are directly refactoring existing generated code to determine optimal patterns for:

- Error propagation strategies
- Factory method signatures
- Memory management integration
- noexcept specifications

This hands-on investigation informs the generator implementation.

### Code Generation Control

When the -UNSAFE master switch is not set, safety-incompatible options must be rejected:

```java
boolean unsafeMasterSwitch = commandLine.hasOption("-UNSAFE");

if (commandLine.hasOption("-withTypeInfo") && !unsafeMasterSwitch) {
    throw new Exception(
        "ERROR: -withTypeInfo requires -UNSAFE flag.\n" +
        "Type info is not functional safety ready.");
}
```

### Generated Code Patterns (Under Investigation)

```cpp
// Factory pattern for deserialization
ZSERIO_NODISCARD static Result<MyMessage> deserialize(
    BitStreamReader& reader,
    const allocator_type& allocator = allocator_type()) noexcept {

    MyMessage obj(allocator);

    // Read field with error propagation
    auto fieldResult = reader.readBits(16);
    if (fieldResult.isError()) {
        return Result<MyMessage>::error(fieldResult.getError());
    }
    obj.m_field1 = fieldResult.getValue();

    return Result<MyMessage>::success(std::move(obj));
}
```

## Phase 5: Functional Safety Container Replacements 🔌

### The Problem

STL containers (`std::vector`, `std::string`, etc.) have a fundamental incompatibility with functional safety when exceptions are disabled:

- With `-fno-exceptions`, allocation failures and bounds violations have implementation-defined behavior
- Most implementations call `std::abort()` - unacceptable for safety-critical systems
- No mechanism to return error codes from operations like `push_back()`, `reserve()`, or `at()`
- Even basic operations like copy construction can fail silently or abort

### Our Approach: Pluggable Container Interface

Rather than providing our own safe container implementations, we will:

1. **Define container interfaces** that generated code and runtime will use
2. **Allow users to plug in their own implementations** giving them full control
3. **Support both safe and unsafe modes**:
   - `ZSERIO_UNSAFE=ON`: Can use STL containers or custom implementations
   - `ZSERIO_UNSAFE=OFF`: Must use custom safe implementations

### Interface Design (Under Investigation)

The exact interface is still being determined through our Phase 4 investigation. Key requirements:

```cpp
// Possible approach - trait-based container selection
template<typename T>
struct ZserioContainerTraits {
    using vector_type = /* user-defined or std::vector */;
    using string_type = /* user-defined or std::string */;
};

// Or policy-based design
template<typename T, typename ContainerPolicy = DefaultSafePolicy>
class Array {
    typename ContainerPolicy::template vector_t<T> m_data;
    // ...
};
```

### Design Considerations

1. **Error Propagation**: All operations that can fail must return `Result<T>`
2. **Allocation Control**: Containers must work with PMR allocators
3. **Bounded Operations**: Support for compile-time or runtime bounds
4. **Zero Overhead**: Interface should not add runtime cost in safe mode
5. **Easy Integration**: Simple for users to provide their implementations

### Why Not Provide Safe Implementations?

- Different safety standards have different requirements
- Users often have existing certified container libraries
- Reduces our certification burden
- Allows domain-specific optimizations

We may reconsider and provide reference implementations in the future, but initially focus on the interface design.

## Phase 6: Memory Management Integration 📋

### Safety-Aware Memory Resource

Working with existing PMR infrastructure while tracking allocation failures:

```cpp
class SafetyMemoryResource : public pmr::MemoryResource {
    mutable std::atomic<bool> m_allocationFailed{false};

public:
    bool hasAllocationFailed() const noexcept {
        return m_allocationFailed.load();
    }

protected:
    void* doAllocate(size_t bytes, size_t alignment) override {
        // Track allocation failures for later checking
        if (insufficient_memory) {
            m_allocationFailed.store(true);
            return nullptr;  // STL will abort, but we can check first!
        }
        // ... allocation logic
    }
};
```

### Practical Patterns

```cpp
// Domain-specific memory pools
class MessageProcessor {
    static constexpr size_t POOL_SIZE = 10 * 1024 * 1024; // 10MB
    alignas(64) uint8_t m_pool[POOL_SIZE];
    SafetyMemoryResource m_resource{m_pool, POOL_SIZE};

    Result<void> processMessage(const uint8_t* data, size_t size) noexcept {
        m_resource.reset();  // Reset for each message
        // ... process with bounded memory
    }
};
```

## Migration Strategy

### Clean Break Approach

The safe C++11 extension is a separate implementation:
- No backwards compatibility with exception-based code
- Users must choose: safe or standard C++ extension
- Clear migration path with examples and tooling

## Open Questions

1. **Container Timeline**: How long to support STL containers with abort risk?
2. **Memory Patterns**: Which patterns to recommend as standard?
3. **Performance Targets**: Acceptable overhead for safety checks?
4. **Service Architecture**: Should services be considered inherently unsafe for functional safety?