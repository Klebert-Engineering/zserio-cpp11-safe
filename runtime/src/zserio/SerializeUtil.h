/**
 * \file
 * It provides help methods for serialization and deserialization of generated objects.
 *
 * These utilities are not used by generated code and they are provided only for user convenience.
 *
 * \note Please note that file operations allocate memory as needed and are not designed to use allocators.
 */

#ifndef ZSERIO_SERIALIZE_UTIL_H_INC
#define ZSERIO_SERIALIZE_UTIL_H_INC

#include "zserio/BitStreamReader.h"
#include "zserio/BitStreamWriter.h"
#include "zserio/FileUtil.h"
#include "zserio/Result.h"
#include "zserio/ErrorCode.h"
#include "zserio/Traits.h"
#include "zserio/Vector.h"

namespace zserio
{

namespace detail
{

template <typename T>
Result<void> initializeChildrenImpl(std::true_type, T& object) noexcept
{
    return object.initializeChildren();
}

template <typename T>
Result<void> initializeChildrenImpl(std::false_type, T&) noexcept
{
    return Result<void>::success();
}

template <typename T>
Result<void> initializeChildren(T& object) noexcept
{
    return initializeChildrenImpl(has_initialize_children<T>(), object);
}

template <typename T, typename... ARGS>
Result<void> initializeImpl(std::true_type, T& object, ARGS&&... arguments) noexcept
{
    return object.initialize(std::forward<ARGS>(arguments)...);
}

template <typename T>
Result<void> initializeImpl(std::false_type, T& object) noexcept
{
    return initializeChildren(object);
}

template <typename T, typename... ARGS>
Result<void> initialize(T& object, ARGS&&... arguments) noexcept
{
    return initializeImpl(has_initialize<T>(), object, std::forward<ARGS>(arguments)...);
}

template <typename T, typename = void>
struct allocator_chooser
{
    using type = std::allocator<uint8_t>;
};

template <typename T>
struct allocator_chooser<T, detail::void_t<typename T::allocator_type>>
{
    using type = typename T::allocator_type;
};

// This implementation needs to be in detail because old MSVC compiler 2015 has problems with calling overload.
template <typename T, typename ALLOC, typename... ARGS>
Result<BasicBitBuffer<ALLOC>> serialize(T& object, const ALLOC& allocator, ARGS&&... arguments) noexcept
{
    // Initialize the object
    auto initResult = detail::initialize(object, std::forward<ARGS>(arguments)...);
    if (initResult.isError())
    {
        return Result<BasicBitBuffer<ALLOC>>::error(initResult.getError());
    }
    
    // Get bit size - assuming initializeOffsets returns size_t
    const size_t bitSize = object.initializeOffsets();
    
    // Create bit buffer
    BasicBitBuffer<ALLOC> bitBuffer(bitSize, allocator);
    BitStreamWriter writer(bitBuffer);
    
    // Write object
    auto writeResult = object.write(writer);
    if (writeResult.isError())
    {
        return Result<BasicBitBuffer<ALLOC>>::error(writeResult.getError());
    }
    
    return Result<BasicBitBuffer<ALLOC>>::success(std::move(bitBuffer));
}

} // namespace detail

/**
 * Serializes given generated object to bit buffer using given allocator.
 *
 * Before serialization, the method properly calls on the given zserio object methods `initialize()`
 * (if exits), `initializeChildren()` (if exists) and `initializeOffsets()`.
 *
 * Example:
 * \code{.cpp}
 *     #include <zserio/SerializeUtil.h>
 *     #include <zserio/pmr/PolymorphicAllocator.h>
 *
 *     const zserio::pmr::PolymorphicAllocator<> allocator;
 *     SomeZserioObject object(allocator);
 *     auto bufferResult = zserio::serialize(object, allocator);
 *     if (bufferResult.isError()) {
 *         // handle error
 *     }
 *     const auto& bitBuffer = bufferResult.getValue();
 * \endcode
 *
 * \param object Generated object to serialize.
 * \param allocator Allocator to use to allocate bit buffer.
 * \param arguments Object's actual parameters for initialize() method (optional).
 *
 * \return Result containing bit buffer with the serialized object, or error code.
 */
template <typename T, typename ALLOC, typename... ARGS,
        typename std::enable_if<!std::is_enum<T>::value && is_allocator<ALLOC>::value, int>::type = 0>
Result<BasicBitBuffer<ALLOC>> serialize(T& object, const ALLOC& allocator, ARGS&&... arguments) noexcept
{
    return detail::serialize(object, allocator, std::forward<ARGS>(arguments)...);
}

/**
 * Serializes given generated object to bit buffer using default allocator 'std::allocator<uint8_t>'.
 *
 * Before serialization, the method properly calls on the given zserio object methods `initialize()`
 * (if exits), `initializeChildren()` (if exists) and `initializeOffsets()`.
 *
 * Example:
 * \code{.cpp}
 *     #include <zserio/SerializeUtil.h>
 *
 *     SomeZserioObject object;
 *     auto bufferResult = zserio::serialize(object);
 *     if (bufferResult.isError()) {
 *         // handle error
 *     }
 *     const auto& bitBuffer = bufferResult.getValue();
 * \endcode
 *
 * \param object Generated object to serialize.
 * \param arguments Object's actual parameters for initialize() method (optional).
 *
 * \return Result containing bit buffer with the serialized object, or error code.
 */
template <typename T, typename ALLOC = typename detail::allocator_chooser<T>::type, typename... ARGS,
        typename std::enable_if<!std::is_enum<T>::value &&
                        !is_first_allocator<typename std::decay<ARGS>::type...>::value,
                int>::type = 0>
Result<BasicBitBuffer<ALLOC>> serialize(T& object, ARGS&&... arguments) noexcept
{
    return detail::serialize(object, ALLOC(), std::forward<ARGS>(arguments)...);
}

/**
 * Serializes given generated enum to bit buffer.
 *
 * Example:
 * \code{.cpp}
 *     #include <zserio/SerializeUtil.h>
 *
 *     const SomeZserioEnum enumValue = SomeZserioEnum::SomeEnumValue;
 *     auto bufferResult = zserio::serialize(enumValue);
 *     if (bufferResult.isError()) {
 *         // handle error
 *     }
 *     const auto& bitBuffer = bufferResult.getValue();
 * \endcode
 *
 * \param enumValue Generated enum to serialize.
 * \param allocator Allocator to use to allocate bit buffer.
 *
 * \return Result containing bit buffer with the serialized enum, or error code.
 */
template <typename T, typename ALLOC = std::allocator<uint8_t>,
        typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
Result<BasicBitBuffer<ALLOC>> serialize(T enumValue, const ALLOC& allocator = ALLOC()) noexcept
{
    const size_t bitSize = zserio::bitSizeOf(enumValue);
    BasicBitBuffer<ALLOC> bitBuffer(bitSize, allocator);
    BitStreamWriter writer(bitBuffer);
    auto writeResult = zserio::write(writer, enumValue);
    if (writeResult.isError())
    {
        return Result<BasicBitBuffer<ALLOC>>::error(writeResult.getError());
    }
    return Result<BasicBitBuffer<ALLOC>>::success(std::move(bitBuffer));
}

/**
 * Deserializes given bit buffer to instance of generated object.
 *
 * Example:
 * \code{.cpp}
 *     #include <zserio/SerializeUtil.h>
 *
 *     SomeZserioObject object;
 *     auto bufferResult = zserio::serialize(object);
 *     if (bufferResult.isError()) {
 *         // handle error
 *     }
 *     auto readObjectResult = zserio::deserialize<SomeZserioObject>(bufferResult.getValue());
 *     if (readObjectResult.isError()) {
 *         // handle error
 *     }
 *     SomeZserioObject readObject = readObjectResult.moveValue();
 * \endcode
 *
 * \param bitBuffer Bit buffer to use.
 * \param arguments Object's actual parameters together with allocator for object's read constructor (optional).
 *
 * \return Result containing generated object created from the given bit buffer, or error code.
 */
template <typename T, typename ALLOC, typename... ARGS>
typename std::enable_if<!std::is_enum<T>::value, Result<T>>::type deserialize(
        const BasicBitBuffer<ALLOC>& bitBuffer, ARGS&&... arguments) noexcept
{
    BitStreamReader reader(bitBuffer);
    return T::deserialize(reader, std::forward<ARGS>(arguments)...);
}

/**
 * Deserializes given bit buffer to instance of generated enum.
 *
 * Example:
 * \code{.cpp}
 *     #include <zserio/SerializeUtil.h>
 *
 *     const SomeZserioEnum enumValue = SomeZserioEnum::SomeEnumValue;
 *     auto bufferResult = zserio::serialize(enumValue);
 *     if (bufferResult.isError()) {
 *         // handle error
 *     }
 *     auto readEnumResult = zserio::deserialize<DummyEnum>(bufferResult.getValue());
 *     if (readEnumResult.isError()) {
 *         // handle error
 *     }
 *     const SomeZserioEnum readEnumValue = readEnumResult.getValue();
 * \endcode
 *
 * \param bitBuffer Bit buffer to use.
 *
 * \return Result containing generated enum created from the given bit buffer, or error code.
 */
template <typename T, typename ALLOC>
typename std::enable_if<std::is_enum<T>::value, Result<T>>::type deserialize(
        const BasicBitBuffer<ALLOC>& bitBuffer) noexcept
{
    BitStreamReader reader(bitBuffer);
    return zserio::read<T>(reader);
}

/**
 * Serializes given generated object to vector of bytes using given allocator.
 *
 * Before serialization, the method properly calls on the given zserio object methods `initialize()`
 * (if exits), `initializeChildren()` (if exists) and `initializeOffsets()`.
 *
 * Example:
 * \code{.cpp}
 *     #include <zserio/SerializeUtil.h>
 *     #include <zserio/pmr/PolymorphicAllocator.h>
 *
 *     const zserio::pmr::PolymorphicAllocator<> allocator;
 *     SomeZserioObject object(allocator);
 *     auto bufferResult = zserio::serializeToBytes(object, allocator);
 *     if (bufferResult.isError()) {
 *         // handle error
 *     }
 *     const auto& buffer = bufferResult.getValue();
 * \endcode
 *
 * \param object Generated object to serialize.
 * \param allocator Allocator to use to allocate vector.
 * \param arguments Object's actual parameters for initialize() method (optional).
 *
 * \return Result containing vector of bytes with the serialized object, or error code.
 */
template <typename T, typename ALLOC, typename... ARGS,
        typename std::enable_if<!std::is_enum<T>::value && is_allocator<ALLOC>::value, int>::type = 0>
Result<vector<uint8_t, ALLOC>> serializeToBytes(T& object, const ALLOC& allocator, ARGS&&... arguments) noexcept
{
    auto bitBufferResult = detail::serialize(object, allocator, std::forward<ARGS>(arguments)...);
    if (bitBufferResult.isError())
    {
        return Result<vector<uint8_t, ALLOC>>::error(bitBufferResult.getError());
    }

    return Result<vector<uint8_t, ALLOC>>::success(bitBufferResult.getValue().getBytes());
}

/**
 * Serializes given generated object to vector of bytes using default allocator 'std::allocator<uint8_t>'.
 *
 * Before serialization, the method properly calls on the given zserio object methods `initialize()`
 * (if exits), `initializeChildren()` (if exists) and `initializeOffsets()`.
 *
 * However, it's still possible that not all bits of the last byte are used. In this case, only most
 * significant bits of the corresponding size are used.
 *
 * Example:
 * \code{.cpp}
 *     #include <zserio/SerializeUtil.h>
 *
 *     SomeZserioObject object;
 *     auto bufferResult = zserio::serializeToBytes(object);
 *     if (bufferResult.isError()) {
 *         // handle error
 *     }
 *     const auto& buffer = bufferResult.getValue();
 * \endcode
 *
 * \param object Generated object to serialize.
 * \param arguments Object's actual parameters for initialize() method (optional).
 *
 * \return Result containing vector of bytes with the serialized object, or error code.
 */
template <typename T, typename ALLOC = typename detail::allocator_chooser<T>::type, typename... ARGS,
        typename std::enable_if<!std::is_enum<T>::value &&
                        !is_first_allocator<typename std::decay<ARGS>::type...>::value,
                int>::type = 0>
Result<vector<uint8_t, ALLOC>> serializeToBytes(T& object, ARGS&&... arguments) noexcept
{
    auto bitBufferResult = detail::serialize(object, ALLOC(), std::forward<ARGS>(arguments)...);
    if (bitBufferResult.isError())
    {
        return Result<vector<uint8_t, ALLOC>>::error(bitBufferResult.getError());
    }

    return Result<vector<uint8_t, ALLOC>>::success(bitBufferResult.getValue().getBytes());
}

/**
 * Serializes given generated enum to vector of bytes.
 *
 * Example:
 * \code{.cpp}
 *     #include <zserio/SerializeUtil.h>
 *
 *     const SomeZserioEnum enumValue = SomeZserioEnum::SomeEnumValue;
 *     auto bufferResult = zserio::serializeToBytes(enumValue);
 *     if (bufferResult.isError()) {
 *         // handle error
 *     }
 *     const auto& buffer = bufferResult.getValue();
 * \endcode
 *
 * \param enumValue Generated enum to serialize.
 * \param allocator Allocator to use to allocate vector.
 *
 * \return Result containing vector of bytes with the serialized enum, or error code.
 */
template <typename T, typename ALLOC = std::allocator<uint8_t>,
        typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
Result<vector<uint8_t, ALLOC>> serializeToBytes(T enumValue, const ALLOC& allocator = ALLOC()) noexcept
{
    auto bitBufferResult = serialize(enumValue, allocator);
    if (bitBufferResult.isError())
    {
        return Result<vector<uint8_t, ALLOC>>::error(bitBufferResult.getError());
    }

    return Result<vector<uint8_t, ALLOC>>::success(bitBufferResult.getValue().getBytes());
}

/**
 * Deserializes given vector of bytes to instance of generated object.
 *
 * This method can potentially use all bits of the last byte even if not all of them were written during
 * serialization (because there is no way how to specify exact number of bits). Thus, it could allow reading
 * behind stream (possibly in case of damaged data).
 *
 * Example:
 * \code{.cpp}
 *     #include <zserio/SerializeUtil.h>
 *
 *     SomeZserioObject object;
 *     auto bufferResult = zserio::serializeToBytes(object);
 *     if (bufferResult.isError()) {
 *         // handle error
 *     }
 *     auto readObjectResult = zserio::deserializeFromBytes<SomeZserioObject>(bufferResult.getValue());
 *     if (readObjectResult.isError()) {
 *         // handle error
 *     }
 *     SomeZserioObject readObject = readObjectResult.moveValue();
 * \endcode
 *
 * \param buffer Vector of bytes to use.
 * \param arguments Object's actual parameters together with allocator for object's read constructor (optional).
 *
 * \return Result containing generated object created from the given vector of bytes, or error code.
 */
template <typename T, typename... ARGS>
typename std::enable_if<!std::is_enum<T>::value, Result<T>>::type deserializeFromBytes(
        Span<const uint8_t> buffer, ARGS&&... arguments) noexcept
{
    BitStreamReader reader(buffer);
    return T::deserialize(reader, std::forward<ARGS>(arguments)...);
}

/**
 * Deserializes given vector of bytes to instance of generated enum.
 *
 * Example:
 * \code{.cpp}
 *     #include <zserio/SerializeUtil.h>
 *
 *     const SomeZserioEnum enumValue = SomeZserioEnum::SomeEnumValue;
 *     auto bufferResult = zserio::serializeToBytes(enumValue);
 *     if (bufferResult.isError()) {
 *         // handle error
 *     }
 *     auto readEnumResult = zserio::deserializeFromBytes<DummyEnum>(bufferResult.getValue());
 *     if (readEnumResult.isError()) {
 *         // handle error
 *     }
 *     const SomeZserioEnum readEnumValue = readEnumResult.getValue();
 * \endcode
 *
 * \param buffer Vector of bytes to use.
 *
 * \return Result containing generated enum created from the given vector of bytes, or error code.
 */
template <typename T>
typename std::enable_if<std::is_enum<T>::value, Result<T>>::type deserializeFromBytes(
        Span<const uint8_t> buffer) noexcept
{
    BitStreamReader reader(buffer);
    return zserio::read<T>(reader);
}

/**
 * Serializes given generated object to file.
 *
 * Example:
 * \code{.cpp}
 *     #include <zserio/SerializeUtil.h>
 *
 *     SomeZserioObject object;
 *     auto result = zserio::serializeToFile(object, "FileName.bin");
 *     if (result.isError()) {
 *         // handle error
 *     }
 * \endcode
 *
 * \param object Generated object to serialize.
 * \param fileName File name to write.
 * \param arguments Object's actual parameters for initialize() method (optional).
 *
 * \return Result<void> indicating success or error code.
 */
template <typename T, typename... ARGS>
Result<void> serializeToFile(T& object, const std::string& fileName, ARGS&&... arguments) noexcept
{
    auto bitBufferResult = serialize(object, std::forward<ARGS>(arguments)...);
    if (bitBufferResult.isError())
    {
        return Result<void>::error(bitBufferResult.getError());
    }
    return writeBufferToFile(bitBufferResult.getValue(), fileName);
}

/**
 * Deserializes given file contents to instance of generated object.
 *
 * Example:
 * \code{.cpp}
 *     #include <zserio/SerializeUtil.h>
 *
 *     const std::string fileName = "FileName.bin";
 *     SomeZserioObject object;
 *     auto writeResult = zserio::serializeToFile(object, fileName);
 *     if (writeResult.isError()) {
 *         // handle error
 *     }
 *     auto readObjectResult = zserio::deserializeFromFile<SomeZserioObject>(fileName);
 *     if (readObjectResult.isError()) {
 *         // handle error
 *     }
 *     SomeZserioObject readObject = readObjectResult.moveValue();
 * \endcode
 *
 * \note Please note that BitBuffer is always allocated using 'std::allocator<uint8_t>'.
 *
 * \param fileName File to use.
 * \param arguments Object's arguments (optional).
 *
 * \return Result containing generated object created from the given file contents, or error code.
 */
template <typename T, typename... ARGS>
Result<T> deserializeFromFile(const std::string& fileName, ARGS&&... arguments) noexcept
{
    auto bitBufferResult = readBufferFromFile(fileName);
    if (bitBufferResult.isError())
    {
        return Result<T>::error(bitBufferResult.getError());
    }
    return deserialize<T>(bitBufferResult.getValue(), std::forward<ARGS>(arguments)...);
}

} // namespace zserio

#endif // ZSERIO_SERIALIZE_UTIL_H_INC