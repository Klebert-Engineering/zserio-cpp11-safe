#ifndef ZSERIO_BITSIZEOF_CALCULATOR_H_INC
#define ZSERIO_BITSIZEOF_CALCULATOR_H_INC

#include <cstddef>
#include <string>

#include "zserio/BitBuffer.h"
#include "zserio/BitPositionUtil.h"
#include "zserio/Result.h"
#include "zserio/SizeConvertUtil.h"
#include "zserio/Span.h"
#include "zserio/StringView.h"
#include "zserio/Types.h"

namespace zserio
{

/**
 * Calculates bit size of Zserio varint16 type.
 *
 * \param value Varint16 value.
 *
 * \return Result containing bit size of the current varint16 value or error code on failure.
 */
Result<size_t> bitSizeOfVarInt16(int16_t value) noexcept;

/**
 * Calculates bit size of Zserio varint32 type.
 *
 * \param value Varint32 value.
 *
 * \return Result containing bit size of the current varint32 value or error code on failure.
 */
Result<size_t> bitSizeOfVarInt32(int32_t value) noexcept;

/**
 * Calculates bit size of Zserio varint64 type.
 *
 * \param value Varint64 value.
 *
 * \return Result containing bit size of the current varint64 value or error code on failure.
 */
Result<size_t> bitSizeOfVarInt64(int64_t value) noexcept;

/**
 * Calculates bit size of Zserio varuint16 type.
 *
 * \param value Varuint16 value.
 *
 * \return Result containing bit size of the current varuint16 value or error code on failure.
 */
Result<size_t> bitSizeOfVarUInt16(uint16_t value) noexcept;

/**
 * Calculates bit size of Zserio varuint32 type.
 *
 * \param value Varuint32 value.
 *
 * \return Result containing bit size of the current varuint32 value or error code on failure.
 */
Result<size_t> bitSizeOfVarUInt32(uint32_t value) noexcept;

/**
 * Calculates bit size of Zserio varuint64 type.
 *
 * \param value Varuint64 value.
 *
 * \return Result containing bit size of the current varuint64 value or error code on failure.
 */
Result<size_t> bitSizeOfVarUInt64(uint64_t value) noexcept;

/**
 * Calculates bit size of Zserio varint type.
 *
 * \param value Varint value.
 *
 * \return Result containing bit size of the current varint value or error code on failure.
 */
Result<size_t> bitSizeOfVarInt(int64_t value) noexcept;

/**
 * Calculates bit size of Zserio varuint type.
 *
 * \param value Varuint value.
 *
 * \return Result containing bit size of the current varuint value or error code on failure.
 */
Result<size_t> bitSizeOfVarUInt(uint64_t value) noexcept;

/**
 * Calculates bit size of Zserio varsize type.
 *
 * \param value Varsize value.
 *
 * \return Result containing bit size of the current varsize value or error code on failure.
 */
Result<size_t> bitSizeOfVarSize(uint32_t value) noexcept;

/**
 * Calculates bit size of bytes.
 *
 * \param bytesValue Span representing the bytes value.
 *
 * \return Result containing bit size of the given bytes value or error code on failure.
 */
Result<size_t> bitSizeOfBytes(Span<const uint8_t> bytesValue) noexcept;

/**
 * Calculates bit size of the string.
 *
 * \param stringValue String view for which to calculate bit size.
 *
 * \return Result containing bit size of the given string or error code on failure.
 */
Result<size_t> bitSizeOfString(StringView stringValue) noexcept;

/**
 * Calculates bit size of the bit buffer.
 *
 * \param bitBuffer Bit buffer for which to calculate bit size.
 *
 * \return Result containing bit size of the given bit buffer or error code on failure.
 */
template <typename ALLOC>
Result<size_t> bitSizeOfBitBuffer(const BasicBitBuffer<ALLOC>& bitBuffer) noexcept
{
    const size_t bitBufferSize = bitBuffer.getBitSize();

    // bit buffer consists of varsize for bit size followed by the bits
    auto convertResult = convertSizeToUInt32(bitBufferSize);
    if (convertResult.isError())
    {
        return Result<size_t>::error(convertResult.getError());
    }
    
    auto sizeResult = bitSizeOfVarSize(convertResult.getValue());
    if (sizeResult.isError())
    {
        return Result<size_t>::error(sizeResult.getError());
    }
    
    return Result<size_t>::success(sizeResult.getValue() + bitBufferSize);
}

} // namespace zserio

#endif // ifndef ZSERIO_BITSIZEOF_CALCULATOR_H_INC
