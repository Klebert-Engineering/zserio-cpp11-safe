#ifndef ZSERIO_ERROR_CODE_H_INC
#define ZSERIO_ERROR_CODE_H_INC

#include <cstdint>

namespace zserio
{

/**
 * Comprehensive error codes for exception-free error handling.
 *
 * These error codes cover all aspects of zserio operations and are designed
 * for functional safety compliance where exceptions cannot be used.
 */
enum class ErrorCode : uint32_t
{
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

/**
 * Get human-readable error message for an error code.
 *
 * \param code Error code to get message for.
 * \return String view with error message.
 */
inline const char* getErrorMessage(ErrorCode code) noexcept
{
    switch (code)
    {
    case ErrorCode::Success:
        return "Success";
    case ErrorCode::UnknownError:
        return "Unknown error";

    // Memory/Allocation
    case ErrorCode::AllocationFailed:
        return "Memory allocation failed";
    case ErrorCode::InsufficientCapacity:
        return "Insufficient capacity";
    case ErrorCode::BufferSizeExceeded:
        return "Buffer size exceeded";
    case ErrorCode::MemoryLimitExceeded:
        return "Memory limit exceeded";
    case ErrorCode::InvalidAlignment:
        return "Invalid memory alignment";
    case ErrorCode::NullPointer:
        return "Null pointer";
    case ErrorCode::InvalidPointer:
        return "Invalid pointer";
    case ErrorCode::MemoryPoolExhausted:
        return "Memory pool exhausted";

    // I/O Operations
    case ErrorCode::EndOfStream:
        return "End of stream";
    case ErrorCode::InvalidBitPosition:
        return "Invalid bit position";
    case ErrorCode::InvalidNumBits:
        return "Invalid number of bits";
    case ErrorCode::BufferOverflow:
        return "Buffer overflow";
    case ErrorCode::WrongBufferBitSize:
        return "Wrong buffer bit size";
    case ErrorCode::InvalidOffset:
        return "Invalid offset";
    case ErrorCode::StreamClosed:
        return "Stream closed";
    case ErrorCode::ReadError:
        return "Read error";
    case ErrorCode::WriteError:
        return "Write error";
    case ErrorCode::SeekError:
        return "Seek error";

    // Serialization/Deserialization
    case ErrorCode::SerializationFailed:
        return "Serialization failed";
    case ErrorCode::DeserializationFailed:
        return "Deserialization failed";
    case ErrorCode::InvalidWireFormat:
        return "Invalid wire format";
    case ErrorCode::VersionMismatch:
        return "Version mismatch";
    case ErrorCode::InvalidMagicNumber:
        return "Invalid magic number";
    case ErrorCode::ChecksumMismatch:
        return "Checksum mismatch";
    case ErrorCode::CompressionError:
        return "Compression error";
    case ErrorCode::DecompressionError:
        return "Decompression error";
    case ErrorCode::InvalidEncoding:
        return "Invalid encoding";
    case ErrorCode::ProtocolError:
        return "Protocol error";

    // Type/Value Errors
    case ErrorCode::InvalidParameter:
        return "Invalid parameter";
    case ErrorCode::InvalidValue:
        return "Invalid value";
    case ErrorCode::OutOfRange:
        return "Value out of range";
    case ErrorCode::InvalidEnumValue:
        return "Invalid enum value";
    case ErrorCode::InvalidStringFormat:
        return "Invalid string format";
    case ErrorCode::ConversionError:
        return "Conversion error";
    case ErrorCode::InvalidBitmask:
        return "Invalid bitmask";
    case ErrorCode::InvalidChoice:
        return "Invalid choice";
    case ErrorCode::InvalidUnion:
        return "Invalid union";
    case ErrorCode::TypeMismatch:
        return "Type mismatch";
    case ErrorCode::InvalidCast:
        return "Invalid cast";
    case ErrorCode::NumericOverflow:
        return "Numeric overflow";
    case ErrorCode::NumericUnderflow:
        return "Numeric underflow";
    case ErrorCode::DivisionByZero:
        return "Division by zero";
    case ErrorCode::InvalidFloatingPoint:
        return "Invalid floating point";

    // Structural Errors
    case ErrorCode::ArrayLengthMismatch:
        return "Array length mismatch";
    case ErrorCode::ParameterMismatch:
        return "Parameter mismatch";
    case ErrorCode::UninitializedParameter:
        return "Uninitialized parameter";
    case ErrorCode::UninitializedField:
        return "Uninitialized field";
    case ErrorCode::MissingRequiredField:
        return "Missing required field";
    case ErrorCode::UnexpectedField:
        return "Unexpected field";
    case ErrorCode::InvalidFieldOrder:
        return "Invalid field order";
    case ErrorCode::RecursionLimitExceeded:
        return "Recursion limit exceeded";
    case ErrorCode::CircularReference:
        return "Circular reference";
    case ErrorCode::InvalidStructure:
        return "Invalid structure";

    // Validation
    case ErrorCode::ValidationFailed:
        return "Validation failed";
    case ErrorCode::ConstraintViolation:
        return "Constraint violation";
    case ErrorCode::InvalidConstraint:
        return "Invalid constraint";
    case ErrorCode::RangeCheckFailed:
        return "Range check failed";
    case ErrorCode::InvalidCondition:
        return "Invalid condition";

    // Optional/Container Access
    case ErrorCode::EmptyOptional:
        return "Empty optional";
    case ErrorCode::InvalidIndex:
        return "Invalid index";
    case ErrorCode::EmptyContainer:
        return "Empty container";
    case ErrorCode::ContainerFull:
        return "Container full";
    case ErrorCode::InvalidIterator:
        return "Invalid iterator";

    // File Operations
    case ErrorCode::FileOpenFailed:
        return "File open failed";
    case ErrorCode::FileReadFailed:
        return "File read failed";
    case ErrorCode::FileWriteFailed:
        return "File write failed";
    case ErrorCode::FileSeekFailed:
        return "File seek failed";
    case ErrorCode::FileCloseFailed:
        return "File close failed";

    // Database Operations
    case ErrorCode::SqliteError:
        return "SQLite error";
    case ErrorCode::DatabaseConnectionFailed:
        return "Database connection failed";
    case ErrorCode::QueryFailed:
        return "Query failed";
    case ErrorCode::TransactionFailed:
        return "Transaction failed";
    case ErrorCode::DatabaseLocked:
        return "Database locked";

    // Service/RPC
    case ErrorCode::ServiceError:
        return "Service error";
    case ErrorCode::MethodNotFound:
        return "Method not found";
    case ErrorCode::InvalidRequest:
        return "Invalid request";
    case ErrorCode::InvalidResponse:
        return "Invalid response";
    case ErrorCode::ServiceTimeout:
        return "Service timeout";

    // Pubsub
    case ErrorCode::PubsubError:
        return "Pub/sub error";
    case ErrorCode::TopicNotFound:
        return "Topic not found";
    case ErrorCode::SubscriptionFailed:
        return "Subscription failed";
    case ErrorCode::PublishFailed:
        return "Publish failed";
    case ErrorCode::InvalidMessage:
        return "Invalid message";

    default:
        return "Unknown error code";
    }
}

} // namespace zserio

#endif // ZSERIO_ERROR_CODE_H_INC