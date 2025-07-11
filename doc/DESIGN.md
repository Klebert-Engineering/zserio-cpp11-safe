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

## Phase 1: Result<T> Pattern Implementation

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

### Error Propagation Macros

Convenience macros for error propagation:

```cpp
#define PROPAGATE_ERROR(result) \
    do { \
        auto&& _r = (result); \
        if (_r.isError()) return Result<T>::error(_r.getError()); \
    } while(0)

#define PROPAGATE_ERROR_WITH_CONTEXT(result, context) \
    do { \
        auto&& _r = (result); \
        if (_r.isError()) { \
            /* Context can be logged or stored */ \
            return Result<T>::error(_r.getError()); \
        } \
    } while(0)
```

## Phase 3: Runtime Library Conversion

### Core Runtime Classes

With unsafe features now separated, we can focus on converting the remaining runtime classes to use Result<T> pattern:

```cpp
class BitStreamReader {
public:
    // Constructor noexcept even with validation
    BitStreamReader(const uint8_t* buffer, size_t size) noexcept;

    // All read operations return Result<T> and are noexcept
    Result<uint32_t> readBits(uint8_t numBits) noexcept;
    Result<int32_t> readSignedBits(uint8_t numBits) noexcept;
    Result<uint64_t> readBits64(uint8_t numBits) noexcept;
    Result<int64_t> readSignedBits64(uint8_t numBits) noexcept;

    // State queries always noexcept
    size_t getBitPosition() const noexcept;
    size_t getBufferBitSize() const noexcept;

    // Compound type readers
    Result<float> readFloat16() noexcept;
    Result<float> readFloat32() noexcept;
    Result<double> readFloat64() noexcept;

    Result<uint64_t> readVarUInt() noexcept;
    Result<int64_t> readVarInt() noexcept;
    Result<uint32_t> readVarSize() noexcept;
};

class BitStreamWriter {
public:
    // Pre-allocated buffer passed in
    BitStreamWriter(uint8_t* buffer, size_t size) noexcept;

    // All write operations noexcept
    Result<void> writeBits(uint32_t value, uint8_t numBits) noexcept;
    Result<void> writeBits64(uint64_t value, uint8_t numBits) noexcept;
    Result<void> writeVarInt32(int32_t value) noexcept;
    Result<void> writeVarUInt64(uint64_t value) noexcept;

    // State queries always noexcept
    size_t getBitPosition() const noexcept;
    size_t getBufferBitSize() const noexcept;
};
```

### Noexcept Specification Strategy

1. **Core Runtime Functions**: All Result<T> returning functions must be noexcept
2. **State Queries**: Always noexcept
3. **Constructors**: noexcept when possible, documented when not
4. **Generated Code**: Proper noexcept specifications based on member types

## Phase 3: Unsafe Extensions Management

### Clear Separation of Functional Safety Ready and Development Features

All features that are not suitable for functional safety will be moved to a clearly marked "unsafe" directory:

```
runtime/src/zserio/
├── BitStreamReader.h        # Functional safety ready core
├── BitStreamWriter.h
├── Result.h
├── ErrorCode.h
├── ...
└── unsafe/                  # NOT FOR PRODUCTION USE
    ├── CppRuntimeException.h  # Exception-based compatibility
    ├── JsonEncoder.h          # Dynamic allocation, unbounded operations
    ├── Reflectable.h          # Runtime type information
    ├── ITypeInfo.h            # Introspection support
    ├── TypeInfo.h
    ├── Walker.h
    └── ...
```

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
// In unsafe/CppRuntimeException.h
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
class CppRuntimeException : public std::exception {
    // Exception-based error handling for development/debugging
};

} // namespace unsafe
} // namespace zserio
```

## Phase 4: Memory Management and PMR Integration

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
- Maintain backwards compatibility where possible

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

## Migration Support

### Backwards Compatibility

During transition, we can support both APIs:

```cpp
#ifdef ZSERIO_ENABLE_EXCEPTIONS
    // Traditional exception-based API
    int32_t readInt32() {
        auto result = readInt32Result();
        if (result.isError()) {
            throw CppRuntimeException("Read failed");
        }
        return result.getValue();
    }
#endif

// New Result-based API
Result<int32_t> readInt32Result() noexcept;
```

### Phased Adoption
1. Opt-in Result<T> API
2. Result<T> default, exceptions opt-in
3. Exception support deprecated
4. Exception support removed

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
