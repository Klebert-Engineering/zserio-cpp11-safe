#ifndef ZSERIO_RESULT_H_INC
#define ZSERIO_RESULT_H_INC

#include <type_traits>
#include <new>
#include <utility>

#include "zserio/ErrorCode.h"

namespace zserio
{

// For C++17 and later, use [[nodiscard]]
#if __cplusplus >= 201703L
#define ZSERIO_NODISCARD [[nodiscard]]
#else
// For earlier versions, we can't use warn_unused_result on classes
// It only works on functions in older compilers
#define ZSERIO_NODISCARD
#endif

/**
 * Result type for exception-free error handling.
 *
 * This class template provides a type-safe way to return either a value or an error code.
 * It is designed for functional safety compliance where exceptions cannot be used.
 *
 * The Result<T> class is marked to ensure callers check the result (when supported).
 * All operations are noexcept to guarantee no exceptions are thrown.
 *
 * @tparam T The type of the success value. Can be void.
 */
template <typename T = void>
class ZSERIO_NODISCARD Result
{
private:
    // Storage optimization using aligned storage with placement new
    union Storage
    {
        alignas(T) unsigned char valueStorage[sizeof(T)];
        ErrorCode error;
        
        Storage() : error(ErrorCode::Success) {}
        ~Storage() {} // Destructor handled by Result
    };
    
    Storage m_storage;
    bool m_hasValue;

    // Private constructors for factory methods
    explicit Result(const T& value) noexcept(std::is_nothrow_copy_constructible<T>::value)
        : m_hasValue(true)
    {
        new (m_storage.valueStorage) T(value);
    }

    explicit Result(T&& value) noexcept(std::is_nothrow_move_constructible<T>::value)
        : m_hasValue(true)
    {
        new (m_storage.valueStorage) T(std::move(value));
    }

    explicit Result(ErrorCode error) noexcept
        : m_hasValue(false)
    {
        m_storage.error = error;
    }

public:
    // Move-only semantics - no copy allowed for efficiency
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    Result(Result&& other) noexcept(std::is_nothrow_move_constructible<T>::value)
        : m_hasValue(other.m_hasValue)
    {
        if (m_hasValue)
        {
            new (m_storage.valueStorage) T(std::move(*other.getValuePtr()));
        }
        else
        {
            m_storage.error = other.m_storage.error;
        }
    }

    Result& operator=(Result&& other) noexcept(std::is_nothrow_move_assignable<T>::value)
    {
        if (this != &other)
        {
            destroy();
            m_hasValue = other.m_hasValue;
            if (m_hasValue)
            {
                new (m_storage.valueStorage) T(std::move(*other.getValuePtr()));
            }
            else
            {
                m_storage.error = other.m_storage.error;
            }
        }
        return *this;
    }

    ~Result() noexcept
    {
        destroy();
    }

    // Static factory methods for creating results
    static Result success(const T& value) noexcept(std::is_nothrow_copy_constructible<T>::value)
    {
        return Result(value);
    }

    static Result success(T&& value) noexcept(std::is_nothrow_move_constructible<T>::value)
    {
        return Result(std::forward<T>(value));
    }

    static Result error(ErrorCode errorCode) noexcept
    {
        return Result(errorCode);
    }

    // Query methods - all noexcept
    ZSERIO_NODISCARD bool isSuccess() const noexcept
    {
        return m_hasValue;
    }

    ZSERIO_NODISCARD bool isError() const noexcept
    {
        return !isSuccess();
    }

    ZSERIO_NODISCARD ErrorCode getError() const noexcept
    {
        return m_hasValue ? ErrorCode::Success : m_storage.error;
    }

    // Value access methods
    ZSERIO_NODISCARD const T& getValue() const & noexcept
    {
        // In production, accessing error Result is undefined behavior
        // Debug builds could assert here
        return *getValuePtr();
    }

    ZSERIO_NODISCARD T& getValue() & noexcept
    {
        return *getValuePtr();
    }

    ZSERIO_NODISCARD T&& getValue() && noexcept
    {
        return std::move(*getValuePtr());
    }

    T&& moveValue() noexcept
    {
        return std::move(*getValuePtr());
    }

private:
    T* getValuePtr() noexcept
    {
        return reinterpret_cast<T*>(m_storage.valueStorage);
    }

    const T* getValuePtr() const noexcept
    {
        return reinterpret_cast<const T*>(m_storage.valueStorage);
    }

    void destroy() noexcept
    {
        if (m_hasValue)
        {
            getValuePtr()->~T();
            m_hasValue = false;
        }
    }
};

// Specialization for void
template <>
class ZSERIO_NODISCARD Result<void>
{
private:
    ErrorCode m_error;

    explicit Result(ErrorCode error) noexcept : m_error(error) {}

public:
    Result() noexcept : m_error(ErrorCode::Success) {}

    // Move-only semantics
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
    Result(Result&&) = default;
    Result& operator=(Result&&) = default;

    static Result success() noexcept
    {
        return Result();
    }

    static Result error(ErrorCode errorCode) noexcept
    {
        return Result(errorCode);
    }

    ZSERIO_NODISCARD bool isSuccess() const noexcept
    {
        return m_error == ErrorCode::Success;
    }

    ZSERIO_NODISCARD bool isError() const noexcept
    {
        return m_error != ErrorCode::Success;
    }

    ZSERIO_NODISCARD ErrorCode getError() const noexcept
    {
        return m_error;
    }
};

// Type aliases for common Result types
using VoidResult = Result<void>;
using BoolResult = Result<bool>;
using SizeResult = Result<size_t>;
using Int32Result = Result<int32_t>;
using Uint32Result = Result<uint32_t>;
using Int64Result = Result<int64_t>;
using Uint64Result = Result<uint64_t>;
using FloatResult = Result<float>;
using DoubleResult = Result<double>;

} // namespace zserio

#endif // ZSERIO_RESULT_H_INC