# Towards C++11 Safe: A Functional Safety Roadmap for Zserio C++

## Executive Summary

This document outlines a comprehensive strategy to transform the Zserio C++11 extension into a functional safety compliant implementation. Building on insights from a previous proof-of-concept (POC), we propose a cleaner, systematic approach that addresses the critical requirements for safety-critical systems while maintaining Zserio's core functionality.

### Key Objectives

1. **Exception-Free Error Handling**: Replace exceptions with error codes using a `Result<T>` API that enforces error checking through `[[nodiscard]]` annotations and provides `noexcept` guarantees, ensuring all errors propagate from the lowest level (BitStreamReader) to the application layer without silent failures
2. **Deterministic Behavior**: No hidden allocations, no unpredictable control flow, no `std::abort()` calls
3. **Safe Zserio Polymorphic Allocator Usage**: Ensure all dynamic memory allocations are performed via the Zserio-provided polymorphic allocator interface, with explicit detection and handling of allocation failures. This enables error propagation instead of silent process termination, even though allocation failures should be architecturally avoided in production systems.
4. **Pluggable Safe Container Implementations**: Allow users to substitute standard containers (such as `std::vector`, `std::string`) with custom, functionally safe alternatives. When exceptions are disabled (`-fno-exceptions`), the C++ standard specifies that operations which would normally throw exceptions have implementation-defined behavior - they may call `std::abort()`, exhibit undefined behavior, or handle the condition in other ways. This affects not only allocation failures but also bounds checking, invalid operations, and other error conditions. Custom safe containers provide explicit error signaling and bounded behavior as required for functional safety.

### Implementation Phases

The implementation follows a strategic phase ordering designed to maximize efficiency:

1. **Phase 1: Unsafe Extensions Separation** - First, move all inherently unsafe features (JSON, reflection, type info) to a clearly marked `unsafe/` directory. This clarifies what remains for safety refactoring.

2. **Phase 2: Result<T> Pattern Implementation** - Establish the exception-free error handling foundation that all subsequent work will build upon.

3. **Phase 3: Runtime Library Conversion** - Convert the core runtime library to use Result<T>, focusing only on the safe subset identified in Phase 1.

4. **Phase 4: Code Generator Updates** - Update the code generator to produce exception-free code that uses the new Result<T>-based runtime APIs.

5. **Phase 5: Memory Management Integration** - Implement defensive strategies for memory allocation failures and provide safety-aware memory resource patterns.

## Phase 1: Unsafe Extensions Separation

### Overview

The first phase establishes a clear architectural boundary between functional safety ready code and development-only features. All inherently unsafe features are moved to a clearly marked `unsafe/` directory, making it obvious what remains for safety refactoring.

### Unsafe Features Identification

The following features are inherently unsafe for functional safety and must be separated:

1. **JSON Support** - Dynamic allocation, unbounded operations, string parsing
2. **Reflection/Introspection** - Runtime type information, dynamic behavior
3. **Type Info** - RTTI-like features incompatible with safety standards
4. **Walker/Visitor** - Dynamic traversal with unbounded recursion
5. **Tree Creator** - Dynamic object construction
6. **Debug Utilities** - Development-only debugging aids

### Directory Structure

After Phase 1, the runtime library has a clear separation:

```
runtime/src/zserio/
├── BitStreamReader.h        # Core serialization (safe)
├── BitStreamWriter.h        # Core serialization (safe)
├── BitBuffer.h             # Core types (safe)
├── CppRuntimeException.h   # Remains here during Phase 1-2
├── ErrorCode.h             # For future Result<T> (safe)
├── Result.h                # For future Result<T> (safe)
├── ...                     # Other safe components
└── unsafe/                 # NOT FOR PRODUCTION USE
    ├── DebugStringUtil.h   # Debug utilities
    ├── IReflectable.h      # Reflection interfaces
    ├── ITypeInfo.h         # Type introspection
    ├── IWalkFilter.h       # Walker interfaces
    ├── IWalkObserver.h     
    ├── JsonDecoder.h       # JSON support
    ├── JsonEncoder.h       
    ├── JsonParser.h        
    ├── JsonReader.h        
    ├── JsonTokenizer.h     
    ├── JsonWriter.h        
    ├── Reflectable.h       # Reflection implementation
    ├── ReflectableUtil.h   
    ├── TypeInfo.h          # Type info implementation
    ├── TypeInfoUtil.h      
    ├── Walker.h            # Object traversal
    ├── WalkerConst.h       
    ├── ZserioTreeCreator.h # Dynamic construction
    └── pmr/                # PMR variants of unsafe features
        ├── IReflectable.h
        ├── ITypeInfo.h
        └── Reflectable.h
```

> **Implementation Note**: Exception classes (CppRuntimeException and related) remain in their current location during Phases 1 and 2. They will be relocated to the unsafe/ directory as part of Phase 3, after the safe runtime conversion is complete. At that point, only unsafe features will use exceptions, making the architectural boundary crystal clear.

### Build Configuration

```cmake
# CMakeLists.txt
option(ZSERIO_CPP11_UNSAFE
       "UNSAFE: Enable development-only extensions - NEVER USE IN PRODUCTION!" OFF)

if(ZSERIO_CPP11_UNSAFE)
    message(WARNING "")
    message(WARNING "================================================")
    message(WARNING "UNSAFE EXTENSIONS ENABLED - NOT FOR PRODUCTION!")
    message(WARNING "This build includes non-functional-safety-ready code!")
    message(WARNING "================================================")
    message(WARNING "")

    # Unsafe extensions CAN use exceptions
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

    add_subdirectory(src/zserio/unsafe)
    target_compile_definitions(ZserioCppRuntime
        PUBLIC ZSERIO_CPP11_UNSAFE_MODE)
else()
    # Production mode: absolutely no exceptions
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
endif()
```

### Code Generation Control

```java
// In code generator
boolean unsafeMasterSwitch = commandLine.hasOption("-UNSAFE");

if (commandLine.hasOption("-withTypeInfo")) {
    if (!unsafeMasterSwitch) {
        throw new Exception(
            "ERROR: -withTypeInfo requires -UNSAFE flag.\n" +
            "Type info is not functional safety ready. To acknowledge this risk,\n" +
            "you must explicitly enable -UNSAFE mode.");
    }
    // Generate type info code with warnings
}

if (commandLine.hasOption("-withReflectionCode")) {
    if (!unsafeMasterSwitch) {
        throw new Exception(
            "ERROR: -withReflectionCode requires -UNSAFE flag.\n" +
            "Reflection is not functional safety ready.");
    }
}

if (commandLine.hasOption("-withJsonCode")) {
    if (!unsafeMasterSwitch) {
        throw new Exception(
            "ERROR: -withJsonCode requires -UNSAFE flag.\n" +
            "JSON serialization is not functional safety ready.");
    }
}
```

### Clear Warnings in Unsafe Code

```cpp
// In unsafe/JsonEncoder.h (and all unsafe headers)
#ifndef ZSERIO_CPP11_UNSAFE_MODE
#error "This file requires UNSAFE mode. Do not include in production code!"
#endif

namespace zserio {
namespace unsafe {

/**
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !!                                               !!
 * !!  WARNING: DEVELOPMENT USE ONLY                !!
 * !!  NOT FOR PRODUCTION OR SAFETY-CRITICAL USE    !!
 * !!                                               !!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */
class JsonEncoder {
    // Implementation
};

} // namespace unsafe
} // namespace zserio
```

### Benefits of Phase 1

1. **Clear Scope**: Identifies exactly what needs safety refactoring
2. **No Accidental Usage**: Unsafe features require explicit opt-in
3. **Simplified Verification**: Safety auditors can ignore unsafe/ directory
4. **Gradual Migration**: Users can still access familiar features during transition
5. **Architectural Clarity**: Physical separation enforces design boundaries

## Phase 2: Result<T> Pattern Implementation

### Design Choice: Result<T> vs std::expected

We considered using a backport of `std::expected` (C++23) but decided to implement our own `Result<T>` pattern for the following reasons:

1. **Simplicity**: Our `Result<T>` implementation is straightforward and minimal, containing only what we need for zserio error handling. This makes it easier to verify for functional safety certification.

2. **Verification Burden**: A full `std::expected` implementation includes monadic operations (`.and_then()`, `.or_else()`, `.transform()`, etc.) which add complexity to safety verification. Many safety-critical projects avoid monadic chaining as it can obscure control flow.

3. **No Additional Dependencies**: Using our own implementation avoids external dependencies or complex backports that would need separate certification.

4. **Focused API**: Our `Result<T>` provides exactly the operations needed for zserio: `isSuccess()`, `isError()`, `getError()`, and `getValue()`. No more, no less.

5. **Future Flexibility**: We can migrate to `std::expected` later if needed, once C++23 is widely adopted and if monadic operations prove valuable. The current API is a subset of `std::expected`, making migration straightforward.

For now, **simplicity wins** - a minimal, verifiable implementation is more valuable than advanced features that add certification complexity.

### The Result<T> Pattern

Based on the POC's proven approach, we will adopt a `Result<T>` pattern as the foundation for all error handling:

```cpp
template <typename T = void>
class [[nodiscard]] Result {
private:
    // Storage optimization using aligned storage with placement new
    union Storage {
        alignas(T) unsigned char valueStorage[sizeof(T)];
        ErrorCode error;
        bool hasValue;

        Storage() : hasValue(false), error(ErrorCode::Success) {}
        ~Storage() {} // Destructor handled by Result
    } m_storage;

    // Private constructors for factory methods
    explicit Result(const T& value) noexcept(std::is_nothrow_copy_constructible<T>::value) {
        m_storage.hasValue = true;
        new (m_storage.valueStorage) T(value);
    }

    explicit Result(T&& value) noexcept(std::is_nothrow_move_constructible<T>::value) {
        m_storage.hasValue = true;
        new (m_storage.valueStorage) T(std::move(value));
    }

    explicit Result(ErrorCode error) noexcept : m_storage() {
        m_storage.hasValue = false;
        m_storage.error = error;
    }

public:
    // Move-only semantics - no copy allowed for efficiency
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    Result(Result&& other) noexcept(std::is_nothrow_move_constructible<T>::value) {
        if (other.m_storage.hasValue) {
            m_storage.hasValue = true;
            new (m_storage.valueStorage) T(std::move(*other.getValuePtr()));
        } else {
            m_storage.hasValue = false;
            m_storage.error = other.m_storage.error;
        }
    }

    Result& operator=(Result&& other) noexcept(std::is_nothrow_move_assignable<T>::value) {
        if (this != &other) {
            destroy();
            if (other.m_storage.hasValue) {
                m_storage.hasValue = true;
                new (m_storage.valueStorage) T(std::move(*other.getValuePtr()));
            } else {
                m_storage.hasValue = false;
                m_storage.error = other.m_storage.error;
            }
        }
        return *this;
    }

    ~Result() noexcept {
        destroy();
    }

    // Static factory methods for creating results
    static Result success(const T& value) noexcept(std::is_nothrow_copy_constructible<T>::value) {
        return Result(value);
    }

    static Result success(T&& value) noexcept(std::is_nothrow_move_constructible<T>::value) {
        return Result(std::forward<T>(value));
    }

    static Result error(ErrorCode errorCode) noexcept {
        return Result(errorCode);
    }

    // Query methods - all noexcept
    [[nodiscard]] bool isSuccess() const noexcept { return m_storage.hasValue; }
    [[nodiscard]] bool isError() const noexcept { return !isSuccess(); }
    [[nodiscard]] ErrorCode getError() const noexcept {
        return m_storage.hasValue ? ErrorCode::Success : m_storage.error;
    }

    // Value access methods
    [[nodiscard]] const T& getValue() const & noexcept {
        // In production, accessing error Result is undefined behavior
        // Debug builds could assert here
        return *getValuePtr();
    }

    [[nodiscard]] T& getValue() & noexcept {
        return *getValuePtr();
    }

    [[nodiscard]] T&& getValue() && noexcept {
        return std::move(*getValuePtr());
    }

    T&& moveValue() noexcept {
        return std::move(*getValuePtr());
    }

private:
    T* getValuePtr() noexcept {
        return reinterpret_cast<T*>(m_storage.valueStorage);
    }

    const T* getValuePtr() const noexcept {
        return reinterpret_cast<const T*>(m_storage.valueStorage);
    }

    void destroy() noexcept {
        if (m_storage.hasValue) {
            getValuePtr()->~T();
            m_storage.hasValue = false;
        }
    }
};

// Specialization for void
template <>
class [[nodiscard]] Result<void> {
private:
    ErrorCode m_error;

    explicit Result(ErrorCode error) noexcept : m_error(error) {}

public:
    Result() noexcept : m_error(ErrorCode::Success) {}

    static Result success() noexcept {
        return Result();
    }

    static Result error(ErrorCode errorCode) noexcept {
        return Result(errorCode);
    }

    [[nodiscard]] bool isSuccess() const noexcept { return m_error == ErrorCode::Success; }
    [[nodiscard]] bool isError() const noexcept { return m_error != ErrorCode::Success; }
    [[nodiscard]] ErrorCode getError() const noexcept { return m_error; }
};
```

### Comprehensive Error Codes

The POC defined 84 comprehensive error codes covering all aspects of zserio operations. We will adopt and enhance this system:

```cpp
enum class ErrorCode : uint32_t {
    // General
    Success = 0,
    UnknownError = 1,

    // Memory/Allocation (2-9)
    AllocationFailed = 2,
    InsufficientCapacity = 3,
    BufferSizeExceeded = 4,
    MemoryLimitExceeded = 5,
    InvalidAlignment = 6,
    NullPointer = 7,
    InvalidPointer = 8,
    MemoryPoolExhausted = 9,

    // I/O Operations (10-19)
    EndOfStream = 10,
    InvalidBitPosition = 11,
    InvalidNumBits = 12,
    BufferOverflow = 13,
    WrongBufferBitSize = 14,
    InvalidOffset = 15,
    StreamClosed = 16,
    ReadError = 17,
    WriteError = 18,
    SeekError = 19,

    // Serialization/Deserialization (20-29)
    SerializationFailed = 20,
    DeserializationFailed = 21,
    InvalidWireFormat = 22,
    VersionMismatch = 23,
    InvalidMagicNumber = 24,
    ChecksumMismatch = 25,
    CompressionError = 26,
    DecompressionError = 27,
    InvalidEncoding = 28,
    ProtocolError = 29,

    // Type/Value Errors (30-44)
    InvalidParameter = 30,
    InvalidValue = 31,
    OutOfRange = 32,
    InvalidEnumValue = 33,
    InvalidStringFormat = 34,
    ConversionError = 35,
    InvalidBitmask = 36,
    InvalidChoice = 37,
    InvalidUnion = 38,
    TypeMismatch = 39,
    InvalidCast = 40,
    NumericOverflow = 41,
    NumericUnderflow = 42,
    DivisionByZero = 43,
    InvalidFloatingPoint = 44,

    // Structural Errors (45-54)
    ArrayLengthMismatch = 45,
    ParameterMismatch = 46,
    UninitializedParameter = 47,
    UninitializedField = 48,
    MissingRequiredField = 49,
    UnexpectedField = 50,
    InvalidFieldOrder = 51,
    RecursionLimitExceeded = 52,
    CircularReference = 53,
    InvalidStructure = 54,

    // Validation (55-59)
    ValidationFailed = 55,
    ConstraintViolation = 56,
    InvalidConstraint = 57,
    RangeCheckFailed = 58,
    InvalidCondition = 59,

    // Optional/Container Access (60-64)
    EmptyOptional = 60,
    InvalidIndex = 61,
    EmptyContainer = 62,
    ContainerFull = 63,
    InvalidIterator = 64,

    // File Operations (65-69)
    FileOpenFailed = 65,
    FileReadFailed = 66,
    FileWriteFailed = 67,
    FileSeekFailed = 68,
    FileCloseFailed = 69,

    // Database Operations (70-74)
    SqliteError = 70,
    DatabaseConnectionFailed = 71,
    QueryFailed = 72,
    TransactionFailed = 73,
    DatabaseLocked = 74,

    // Service/RPC (75-79)
    ServiceError = 75,
    MethodNotFound = 76,
    InvalidRequest = 77,
    InvalidResponse = 78,
    ServiceTimeout = 79,

    // Pubsub (80-84)
    PubsubError = 80,
    TopicNotFound = 81,
    SubscriptionFailed = 82,
    PublishFailed = 83,
    InvalidMessage = 84
};
```

### Error Propagation Strategy

While error propagation macros could enhance code readability by reducing boilerplate, we have decided **against** using them for functional safety compliance. 

**Considered but rejected approach:**
```cpp
// REJECTED: Macros hide control flow
#define PROPAGATE_ERROR(result) \
    do { \
        auto&& _r = (result); \
        if (_r.isError()) return Result<T>::error(_r.getError()); \
    } while(0)
```

**Reasons for rejection:**
1. **Hidden Control Flow**: The `return` statement inside macros violates safety guidelines (e.g., MISRA C++ Rule 16-0-1). Safety standards require explicit, traceable control flow.
2. **Static Analysis Limitations**: Certification tools often struggle with macro expansion, potentially missing critical checks or producing false positives.
3. **Debugging Complexity**: Macros make it harder to trace execution paths during debugging and safety analysis.
4. **Type Safety Concerns**: The macro needs to infer the return type `T`, which could lead to subtle errors.

**Recommended approach for functional safety:**
```cpp
// Explicit and traceable error handling
auto result = someOperation();
if (result.isError()) {
    return Result<ReturnType>::error(result.getError());
}
auto value = result.getValue();
```

This explicit approach ensures:
- Clear control flow visible to static analyzers
- Better traceability for safety certification
- Easier debugging and code review
- Type safety guaranteed by the compiler

## Phase 3: Runtime Library Conversion

### Overview

The runtime library conversion phase systematically transforms all core runtime
components to use the Result<T> pattern for exception-free error handling. This
phase builds upon the infrastructure established in Phase 2 and focuses on the
safe subset of runtime functionality identified in Phase 1.

### Key Design Decisions

#### Single API Approach - No Dual Support

The safe runtime provides **only** Result<T>-based APIs, with no exception-throwing variants. This critical decision:
- Eliminates code duplication and conditional compilation complexity
- Reduces verification burden for safety certification
- Ensures predictable behavior in all configurations
- Forces explicit error handling throughout the codebase

#### Factory Pattern for Object Construction

Since C++ constructors cannot return error codes, we adopt a factory pattern
for objects that may fail during construction. This pattern, proven in the POC,
ensures all errors are explicitly handled:

```cpp
// Pattern for generated structures
class MyStruct {
public:
    // Simple constructor - no validation, cannot fail
    explicit MyStruct(const allocator_type& allocator = allocator_type()) noexcept;
    
    // Factory method for deserialization that can fail
    static Result<MyStruct> deserialize(
        BitStreamReader& reader, 
        const allocator_type& allocator = allocator_type()) noexcept;
    
    // Instance methods that can fail
    Result<void> initializeChildren() noexcept;
    Result<void> write(BitStreamWriter& writer) const noexcept;
};
```

#### Explicit Error Propagation

For functional safety compliance, we explicitly handle all error propagation without macros:

```cpp
// Clear, traceable error handling
auto bitsResult = reader.readBits(8);
if (bitsResult.isError()) {
    return Result<MyStruct>::error(bitsResult.getError());
}
uint8_t value = bitsResult.getValue();
```

### Core Runtime Classes Conversion

#### BitStreamReader

The BitStreamReader is the foundation of deserialization and must be converted first:

```cpp
class BitStreamReader {
public:
    // Constructor remains simple - validation deferred
    BitStreamReader(const uint8_t* buffer, size_t size) noexcept;
    
    // Factory method for construction with validation
    static Result<BitStreamReader> create(
        const uint8_t* buffer, 
        size_t bufferBitSize) noexcept;

    // All read operations return Result<T> and are noexcept
    Result<uint32_t> readBits(uint8_t numBits) noexcept;
    Result<int32_t> readSignedBits(uint8_t numBits) noexcept;
    Result<uint64_t> readBits64(uint8_t numBits) noexcept;
    Result<int64_t> readSignedBits64(uint8_t numBits) noexcept;

    // Variable-length encodings
    Result<uint64_t> readVarUInt() noexcept;
    Result<int64_t> readVarInt() noexcept;
    Result<uint32_t> readVarSize() noexcept;
    
    // Floating-point types
    Result<float> readFloat16() noexcept;
    Result<float> readFloat32() noexcept;
    Result<double> readFloat64() noexcept;
    
    // Complex types with allocator support
    template <typename ALLOC = std::allocator<uint8_t>>
    Result<vector<uint8_t, ALLOC>> readBytes(const ALLOC& alloc = ALLOC()) noexcept;
    
    template <typename ALLOC = std::allocator<char>>
    Result<string<ALLOC>> readString(const ALLOC& alloc = ALLOC()) noexcept;

    // State queries always noexcept
    size_t getBitPosition() const noexcept;
    size_t getBufferBitSize() const noexcept;
    Result<void> setBitPosition(size_t position) noexcept;
};
```

#### BitStreamWriter

The BitStreamWriter handles serialization with explicit error handling:

```cpp
class BitStreamWriter {
public:
    // Pre-allocated buffer passed in
    BitStreamWriter(uint8_t* buffer, size_t size) noexcept;
    
    // Factory method with validation
    static Result<BitStreamWriter> create(
        uint8_t* buffer, 
        size_t bufferBitSize) noexcept;

    // All write operations return Result<void>
    Result<void> writeBits(uint32_t value, uint8_t numBits) noexcept;
    Result<void> writeBits64(uint64_t value, uint8_t numBits) noexcept;
    Result<void> writeSignedBits(int32_t value, uint8_t numBits) noexcept;
    Result<void> writeSignedBits64(int64_t value, uint8_t numBits) noexcept;
    
    // Variable-length encodings
    Result<void> writeVarUInt(uint64_t value) noexcept;
    Result<void> writeVarInt(int64_t value) noexcept;
    Result<void> writeVarSize(uint32_t value) noexcept;
    
    // Complex types
    template <typename ALLOC>
    Result<void> writeBytes(const vector<uint8_t, ALLOC>& data) noexcept;
    
    template <typename ALLOC>
    Result<void> writeString(const string<ALLOC>& str) noexcept;

    // State management
    size_t getBitPosition() const noexcept;
    size_t getBufferBitSize() const noexcept;
};
```

### Container and Type Support Classes

#### BitBuffer with Factory Pattern

```cpp
class BitBuffer {
public:
    // Simple constructor for empty buffer
    explicit BitBuffer(const ALLOC& allocator = ALLOC()) noexcept;
    
    // Factory methods for construction that can fail
    static Result<BitBuffer> create(
        size_t bitSize, 
        const ALLOC& allocator = ALLOC()) noexcept;
    
    static Result<BitBuffer> fromReader(
        BitStreamReader& reader,
        size_t bitSize,
        const ALLOC& allocator = ALLOC()) noexcept;
    
    // Safe operations
    Result<void> resize(size_t newBitSize) noexcept;
    Result<uint8_t> getByteAt(size_t byteIndex) const noexcept;
};
```

#### Critical Container Limitation

**TODO: Custom Container Implementation Required**

When building with `-fno-exceptions`, STL containers have implementation-defined behavior on allocation failure or bounds violations. Most implementations call `std::abort()`, which is unacceptable for functional safety.

```cpp
// CRITICAL TODO: STL containers may abort when exceptions are disabled
class Array {
    vector<T, ALLOC> m_data;  // TODO: Will abort on allocation failure!
    
public:
    Result<void> read(BitStreamReader& reader, size_t size) noexcept {
        // TODO: This reserve() may abort if allocation fails
        // Future phases MUST provide custom containers
        m_data.reserve(size);  // DANGER: May call std::abort()!
        
        for (size_t i = 0; i < size; ++i) {
            auto result = T::deserialize(reader);
            if (result.isError()) {
                return Result<void>::error(result.getError());
            }
            // TODO: push_back may also abort on allocation failure
            m_data.push_back(result.moveValue());
        }
        return Result<void>::success();
    }
};
```

This limitation is acknowledged but deferred to future phases because:
1. Phase 3 focuses on establishing the Result<T> pattern
2. Custom containers require significant design effort
3. Pre-allocated memory pools can mitigate the risk temporarily
4. Full solution requires the pluggable container system described in Phase 5

### Build System Integration

#### CMake Configuration

```cmake
# Runtime library configuration
# The safe runtime ALWAYS builds without exceptions
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

# Optional unsafe extensions (from Phase 1)
option(ZSERIO_CPP11_UNSAFE
       "UNSAFE: Enable development-only extensions - NEVER USE IN PRODUCTION!" OFF)

if(ZSERIO_CPP11_UNSAFE)
    # Only unsafe extensions can use exceptions
    # Safe runtime remains exception-free
    message(WARNING "Unsafe extensions enabled - NOT FOR SAFETY-CRITICAL USE!")
endif()
```

#### No Dual API - Safe Code is Always Exception-Free

The safe runtime components use Result<T> exclusively. This design decision:
- Simplifies implementation and verification
- Reduces code paths to test
- Makes safety certification easier
- Ensures consistent behavior regardless of build configuration

```cpp
class BitStreamReader {
public:
    // Only Result-based API - no exceptions ever
    Result<uint32_t> readBits(uint8_t numBits) noexcept;
    Result<int32_t> readSignedBits(uint8_t numBits) noexcept;
    
    // No throwing variants - keeps implementation clean
};
```

### Implementation Order

The conversion follows a careful dependency order to minimize disruption:

1. **Core Infrastructure**
   - Result<T> and ErrorCode (from Phase 2)
   - Build system configuration
   - Test infrastructure updates

2. **BitStream Foundation**
   - BitStreamReader conversion
   - BitStreamWriter conversion
   - Comprehensive error propagation tests

3. **Type Support Classes**
   - BitFieldUtil (bounds checking)
   - BitSizeOfCalculator (overflow prevention)
   - FloatUtil (NaN/Inf handling)
   - SizeConvertUtil (safe conversions)

4. **Container Classes**
   - BitBuffer with factory methods
   - Array/Vector safety wrappers
   - String handling with bounds
   - Span safety improvements

5. **Complex Types**
   - OptionalHolder (access validation)
   - UniquePtr (allocation handling)
   - Variant type safety

### Testing Strategy

Each converted component requires comprehensive testing:

1. **Unit Tests**
   - Convert existing tests to Result<T> API
   - Add error path coverage
   - Verify noexcept compliance

2. **Integration Tests**
   - End-to-end serialization/deserialization
   - Error propagation chains
   - Memory allocation failure scenarios

3. **Performance Tests**
   - Benchmark Result<T> overhead
   - Compare with exception-based code
   - Optimize critical paths

### Migration Strategy

Since there is no dual API support, migration requires a clean break:

1. **Clear Separation**
   - Safe runtime uses Result<T> exclusively
   - No backward compatibility with exception-based code
   - Users must choose: safe or unsafe variant

2. **Documentation**
   - Migration guide showing before/after examples
   - Error handling patterns cookbook
   - Memory pool sizing guidelines

3. **Tooling Support**
   - Static analysis to find exception usage
   - Code generation updates for Result<T> pattern
   - Build configuration validation

### Exception Class Relocation

As part of Phase 3, all exception classes move to the `unsafe/` directory:
- Safe runtime no longer uses any exceptions after Result<T> conversion
- Only unsafe features (JSON, reflection) need CppRuntimeException
- Moving exceptions to `unsafe/` clarifies the architectural boundary
- Include paths in unsafe features updated to `#include "zserio/unsafe/CppRuntimeException.h"`

This relocation happens after the safe runtime conversion is complete, ensuring:
- Clear separation between safe and unsafe code
- No accidental exception usage in safe code (would fail to compile)
- Explicit acknowledgment when using exception-based features

### Noexcept Specification Strategy

1. **Core Runtime Functions**: All Result<T> returning functions must be noexcept
2. **State Queries**: Always noexcept
3. **Constructors**: noexcept when possible, documented when not
4. **Generated Code**: Proper noexcept specifications based on member types
5. **Template Functions**: Conditional noexcept based on template parameters

## Phase 4: Code Generator Updates

### Overview

The code generator must be updated to produce exception-free code that uses the Result<T>-based runtime APIs. This phase transforms how zserio generates C++ code, ensuring all generated code follows functional safety principles.

### Key Changes

#### Deserialization Factory Pattern

Generated structures use static factory methods for construction that can fail:

```cpp
// Generated code pattern
class MyMessage {
public:
    // Simple constructor - cannot fail
    explicit MyMessage(const allocator_type& allocator = allocator_type()) noexcept;
    
    // Factory method for deserialization
    static Result<MyMessage> deserialize(
        BitStreamReader& reader,
        const allocator_type& allocator = allocator_type()) noexcept {
        
        MyMessage obj(allocator);
        
        // Read field1
        auto field1Result = reader.readBits(16);
        if (field1Result.isError()) {
            return Result<MyMessage>::error(field1Result.getError());
        }
        obj.m_field1 = field1Result.getValue();
        
        // Read array size
        auto sizeResult = reader.readVarSize();
        if (sizeResult.isError()) {
            return Result<MyMessage>::error(sizeResult.getError());
        }
        
        // TODO: STL reserve may abort!
        obj.m_array.reserve(sizeResult.getValue());
        
        // Read array elements
        for (size_t i = 0; i < sizeResult.getValue(); ++i) {
            auto elemResult = Element::deserialize(reader, allocator);
            if (elemResult.isError()) {
                return Result<MyMessage>::error(elemResult.getError());
            }
            obj.m_array.push_back(elemResult.moveValue());
        }
        
        // Initialize parameterized children
        auto initResult = obj.initializeChildren();
        if (initResult.isError()) {
            return Result<MyMessage>::error(initResult.getError());
        }
        
        return Result<MyMessage>::success(std::move(obj));
    }
};
```

#### Write Methods with Error Handling

```cpp
Result<void> write(BitStreamWriter& writer) const noexcept {
    // Write field1
    auto result = writer.writeBits(m_field1, 16);
    if (result.isError()) {
        return result;
    }
    
    // Write array size
    result = writer.writeVarSize(static_cast<uint32_t>(m_array.size()));
    if (result.isError()) {
        return result;
    }
    
    // Write array elements
    for (const auto& elem : m_array) {
        result = elem.write(writer);
        if (result.isError()) {
            return result;
        }
    }
    
    return Result<void>::success();
}
```

#### Size Calculation with Overflow Protection

```cpp
Result<size_t> bitSizeOf(size_t bitPosition = 0) const noexcept {
    size_t endBitPosition = bitPosition;
    
    // Add field1 size
    endBitPosition += 16;
    
    // Add array size field
    auto varSizeBits = zserio::bitSizeOfVarSize(static_cast<uint32_t>(m_array.size()));
    if (endBitPosition > SIZE_MAX - varSizeBits) {
        return Result<size_t>::error(ErrorCode::NumericOverflow);
    }
    endBitPosition += varSizeBits;
    
    // Add array elements
    for (const auto& elem : m_array) {
        auto elemSizeResult = elem.bitSizeOf(endBitPosition);
        if (elemSizeResult.isError()) {
            return elemSizeResult;
        }
        if (endBitPosition > SIZE_MAX - elemSizeResult.getValue()) {
            return Result<size_t>::error(ErrorCode::NumericOverflow);
        }
        endBitPosition += elemSizeResult.getValue();
    }
    
    return Result<size_t>::success(endBitPosition - bitPosition);
}
```

### Template Updates

All FreeMarker templates must be updated:

1. **Structure Templates** (*.h.ftl, *.cpp.ftl)
   - Add static deserialize factory method
   - Convert write() to return Result<void>
   - Update bitSizeOf() for overflow checking
   - Add noexcept specifications

2. **Choice/Union Templates**
   - Handle variant selection with error codes
   - Validate choice tags

3. **Service Templates**
   - Generate Result-based method signatures
   - Remove exception-based error handling

### Code Generation Options

```java
// New safety-oriented options
if (commandLine.hasOption("-cpp11safe")) {
    // Generate Result<T> based code
    context.setExceptionFree(true);
    context.setUseFactoryPattern(true);
}

// Validate incompatible options
if (context.isExceptionFree() && commandLine.hasOption("-withValidation")) {
    // Validation code uses exceptions
    throw new Exception(
        "ERROR: -withValidation is incompatible with -cpp11safe.\n" +
        "Validation code uses exceptions and is not safety-ready.");
}
```

### Generated Code Warnings

```cpp
// Generated file header
/**
 * Generated by Zserio C++11 Safe Extension
 * 
 * WARNING: This code is designed for functional safety.
 * - No exceptions are thrown
 * - All errors must be explicitly checked
 * - STL containers may abort on allocation failure
 * - See safety manual for proper usage
 */
```

## Phase 5: Memory Management and PMR Integration

### Working with Existing PMR Infrastructure

Since we must work with existing PMR and STL containers that may abort, we need a pragmatic approach:

#### Safety-Aware Memory Resource

```cpp
class SafetyMemoryResource : public pmr::MemoryResource {
private:
    uint8_t* m_buffer;
    size_t m_bufferSize;
    size_t m_offset = 0;

    // Safety tracking
    mutable std::atomic<bool> m_allocationFailed{false};
    mutable ErrorCode m_lastError{ErrorCode::Success};

public:
    SafetyMemoryResource(uint8_t* buffer, size_t size) noexcept
        : m_buffer(buffer), m_bufferSize(size) {}

    // Safety extension: Query if last allocation failed
    bool hasAllocationFailed() const noexcept {
        return m_allocationFailed.load();
    }

    ErrorCode getLastError() const noexcept {
        return m_lastError;
    }

    void clearError() noexcept {
        m_allocationFailed.store(false);
        m_lastError = ErrorCode::Success;
    }

protected:
    void* doAllocate(size_t bytes, size_t alignment) override {
        size_t alignedOffset = align(m_offset, alignment);

        if (alignedOffset + bytes > m_bufferSize) {
            // Record failure for later checking
            m_allocationFailed.store(true);
            m_lastError = ErrorCode::InsufficientMemory;

            // Return nullptr - STL will abort, but we can check first!
            return nullptr;
        }

        void* ptr = m_buffer + alignedOffset;
        m_offset = alignedOffset + bytes;
        return ptr;
    }

    void doDeallocate(void* p, size_t bytes, size_t alignment) override {
        // Simple bump allocator - no deallocation
    }
};
```

#### Defensive Deserialization Pattern

```cpp
class SafeDeserializer {
    pmr::MemoryResource* m_resource;

    // Try to get safety-aware resource if available
    SafetyMemoryResource* getSafetyResource() noexcept {
        return dynamic_cast<SafetyMemoryResource*>(m_resource);
    }

public:
    // Deserialize with defensive checks
    Result<MyStruct> deserialize(BitStreamReader& reader) noexcept {
        // Read array size first
        auto sizeResult = reader.readVarSize();
        PROPAGATE_ERROR(sizeResult);

        size_t arraySize = sizeResult.getValue();

        // Best effort: Check if obviously too large
        constexpr size_t SANITY_LIMIT = 1'000'000; // 1M elements
        if (arraySize > SANITY_LIMIT) {
            return Result<MyStruct>::error(ErrorCode::ArrayTooLarge);
        }

        // Create structure with PMR
        MyStruct result(m_resource);

        // If we have a safety resource, clear error state
        if (auto* safeResource = getSafetyResource()) {
            safeResource->clearError();
        }

        // DANGER ZONE: STL operations that may abort
        try {
            // In builds with exceptions, we can catch
            result.m_items.reserve(arraySize);
        } catch (...) {
            return Result<MyStruct>::error(ErrorCode::AllocationFailed);
        }

        // In no-exception builds, check if allocation failed
        if (auto* safeResource = getSafetyResource()) {
            if (safeResource->hasAllocationFailed()) {
                return Result<MyStruct>::error(
                    safeResource->getLastError());
            }
        }

        // Continue deserialization...
        return Result<MyStruct>::success(std::move(result));
    }
};
```

### Practical Memory Patterns

#### Pattern 1: Memory Pools with Reset

```cpp
class MessageProcessor {
    // Large pool for typical operations
    static constexpr size_t POOL_SIZE = 10 * 1024 * 1024; // 10MB
    alignas(64) uint8_t m_pool[POOL_SIZE];
    SafetyMemoryResource m_resource{m_pool, POOL_SIZE};

public:
    Result<void> processMessage(const uint8_t* data, size_t size) noexcept {
        // Reset pool for each message
        m_resource.reset();

        BitStreamReader reader(data, size);
        auto result = Message::deserialize(reader, &m_resource);

        if (result.isError()) {
            // Log memory usage for sizing analysis
            logMemoryUsage(m_resource.getPeakUsage());
            return Result<void>::error(result.getError());
        }

        // Process message...
        return Result<void>::success();
    }
};
```

#### Pattern 2: Domain-Specific Pools

```cpp
class VehicleDataProcessor {
    // Separate pools for different data types
    SafetyMemoryResource m_roadSegmentPool;    // Known typical size
    SafetyMemoryResource m_sensorDataPool;     // Bounded by sensor rate
    SafetyMemoryResource m_databaseResultPool; // Sized by query limits

    Result<RoadData> getRoadData(Position pos) noexcept {
        // Use domain knowledge to select pool
        m_roadSegmentPool.reset();

        // Database query with result limit
        auto queryResult = database.queryRoadSegments(
            pos,
            MAX_SEGMENTS_PER_QUERY,
            &m_roadSegmentPool);

        return queryResult;
    }
};
```

## Implementation Strategy

### Phase 1: Unsafe Feature Separation
- Move all exception-using code to unsafe/
- Move reflection/introspection to unsafe/
- Move JSON support to unsafe/
- Add clear build-time controls
- **Benefit**: Clearly identifies what needs refactoring

### Phase 2: Core Infrastructure
- Implement Result<T> and ErrorCode system
- Create comprehensive error propagation tests
- Establish noexcept guidelines
- **Benefit**: Foundation for all subsequent work

### Phase 3: Runtime Library Conversion
- Convert BitStreamReader/Writer to Result<T>
- Update all type support classes
- Focus only on safe subset identified in Phase 1
- Move exception classes to unsafe/ after conversion

### Phase 4: Code Generator Updates
- Generate Result<T>-based APIs
- Produce exception-free deserialization/serialization code
- Add proper noexcept specifications
- Generate meaningful error contexts
- Include safety warnings for unsafe features

### Phase 5: Memory Management Integration
- Implement SafetyMemoryResource pattern
- Provide defensive allocation strategies
- Document best practices for bounded memory usage
- Create domain-specific memory pool examples

## Migration Strategy

### Clean Break Approach

The safe C++11 extension is a separate implementation with no backwards compatibility:

```cpp
// Safe runtime - ONLY Result<T> API
namespace zserio {
    class BitStreamReader {
        Result<uint32_t> readBits(uint8_t numBits) noexcept;
        // No exception-throwing variants
    };
}

// Original runtime still available separately
// Users must choose which to use
```

### Migration Path
1. Evaluate safety requirements
2. Choose safe or standard C++ extension
3. Update build configuration
4. Port code to Result<T> pattern
5. Implement proper error handling

## Future Improvements: Custom Container Strategy

**Note**: This section describes potential future enhancements after the initial exception-free implementation is complete.

### Container Abstraction Layer

Once we have proven the exception-free approach works, we can consider custom container implementations:

```cpp
// Abstract container interface for future use
template <typename T>
class IContainer {
public:
    virtual ~IContainer() = default;
    virtual Result<void> reserve(size_t n) = 0;
    virtual Result<void> push_back(const T& value) = 0;
    virtual size_t size() const noexcept = 0;
    virtual Result<T&> at(size_t index) = 0;
};
```

### Safe String Implementation

```cpp
// Future: Bounded string type
template<size_t MaxSize>
class BoundedString {
    char m_data[MaxSize + 1] = {0};
    size_t m_length = 0;

public:
    Result<void> assign(const char* str, size_t len) noexcept {
        if (len > MaxSize) {
            return Result<void>::error(ErrorCode::StringTooLong);
        }
        std::memcpy(m_data, str, len);
        m_data[len] = '\0';
        m_length = len;
        return Result<void>::success();
    }
};
```

### Container Plugin System

Allow users to provide their own container implementations:

```cpp
// Configuration structure
struct ZserioSafeConfig {
    // Container factories
    std::function<std::unique_ptr<IContainer<uint8_t>>()> byteArrayFactory;
    std::function<std::unique_ptr<IContainer<char>>()> stringFactory;

    // Memory management
    IMemoryResource* defaultMemoryResource = nullptr;
};
```

## Certification Considerations

### Design Principles
1. All error paths must be testable
2. No dynamic memory allocation in core paths
3. Bounded execution time
4. No recursion
5. All loops must have proven bounds

### Target Standards
- IEC 61508 SIL 3 as baseline
- ISO 26262 ASIL-D ready
- DO-178C considerations documented

### Documentation Requirements
1. Safety manual
2. Failure mode analysis
3. Test coverage reports
4. Static analysis results

## Open Questions

1. **Container Transition Timeline**: How long to support STL containers with abort risk?
2. **Memory Patterns**: Which patterns to recommend as standard?
3. **Performance Targets**: Acceptable overhead for safety checks?
4. **Certification Scope**: Which standards to formally pursue?
5. **Service Architecture**: The current IService.h design tightly couples RPC services with reflection support. In Phase 1, we've used conditional compilation to make reflection optional, but this creates two different interfaces depending on build configuration. Should we:
   - Keep the conditional compilation approach (current solution)?
   - Create separate interfaces for reflection-based and byte-based services?
   - Redesign the service architecture to cleanly separate concerns?
   - Consider services as inherently unsafe for functional safety?

## Risk Mitigation

### Technical Risks
1. **STL Abort Risk**: Mitigated by SafetyMemoryResource pattern
2. **API Breaking Changes**: Mitigated by transition period
3. **Performance Impact**: Mitigated by efficient Result<T> implementation

### Process Risks

1. **User Adoption**: Clear migration path and tools
2. **Certification Cost**: Modular approach, certify core first
3. **Maintenance Burden**: Clear separation of safe/unsafe code
