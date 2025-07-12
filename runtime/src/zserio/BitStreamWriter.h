#ifndef ZSERIO_BIT_STREAM_WRITER_H_INC
#define ZSERIO_BIT_STREAM_WRITER_H_INC

#include <algorithm>
#include <cstddef>
#include <cstring>

#include "zserio/BitBuffer.h"
#include "zserio/ErrorCode.h"
#include "zserio/Result.h"
#include "zserio/SizeConvertUtil.h"
#include "zserio/Span.h"
#include "zserio/StringView.h"
#include "zserio/Types.h"

namespace zserio
{

/**
 * Writer class which allows to write various data to the bit stream.
 */
class BitStreamWriter
{
public:

    /** Type for bit position. */
    using BitPosType = size_t;

    /**
     * Constructor from externally allocated byte buffer.
     *
     * \param buffer External byte buffer to create from.
     * \param bufferBitSize Size of the buffer in bits.
     */
    explicit BitStreamWriter(uint8_t* buffer, size_t bufferBitSize, BitsTag) noexcept;

    /**
     * Constructor from externally allocated byte buffer.
     *
     * \param buffer External byte buffer to create from.
     * \param bufferByteSize Size of the buffer in bytes.
     */
    explicit BitStreamWriter(uint8_t* buffer, size_t bufferByteSize) noexcept;

    /**
     * Constructor from externally allocated byte buffer.
     *
     * \param buffer External buffer to create from as a Span.
     */
    explicit BitStreamWriter(Span<uint8_t> buffer) noexcept;

    /**
     * Constructor from externally allocated byte buffer with exact bit size.
     * Note: This constructor does not validate buffer size compatibility.
     * Use create() factory method for validated construction.
     *
     * \param buffer External buffer to create from as a Span.
     * \param bufferBitSize Size of the buffer in bits.
     */
    explicit BitStreamWriter(Span<uint8_t> buffer, size_t bufferBitSize) noexcept;

    /**
     * Constructor from externally allocated bit buffer.
     *
     * \param bitBuffer External bit buffer to create from.
     */
    template <typename ALLOC>
    explicit BitStreamWriter(BasicBitBuffer<ALLOC>& bitBuffer) noexcept :
            BitStreamWriter(bitBuffer.getData(), bitBuffer.getBitSize())
    {}

    /**
     * Factory method for construction with validation.
     *
     * \param buffer External buffer to create from as a Span.
     * \param bufferBitSize Size of the buffer in bits.
     *
     * \return Result with BitStreamWriter on success, error code on failure.
     */
    static Result<BitStreamWriter> create(Span<uint8_t> buffer, size_t bufferBitSize) noexcept;

    /**
     * Destructor.
     */
    ~BitStreamWriter() = default;

    /**
     * Copying is disallowed, moving is allowed!
     * \{
     */
    BitStreamWriter(const BitStreamWriter&) = delete;
    BitStreamWriter& operator=(const BitStreamWriter&) = delete;

    BitStreamWriter(BitStreamWriter&&) = default;
    BitStreamWriter& operator=(BitStreamWriter&&) = default;
    /**
     * \}
     */

    /**
     * Writes unsigned bits up to 32 bits.
     *
     * \param data Data to write.
     * \param numBits Number of bits to write.
     *
     * \return Success or error code.
     */
    Result<void> writeBits(uint32_t data, uint8_t numBits = 32) noexcept;

    /**
     * Writes unsigned bits up to 64 bits.
     *
     * \param data Data to write.
     * \param numBits Number of bits to write.
     *
     * \return Success or error code.
     */
    Result<void> writeBits64(uint64_t data, uint8_t numBits = 64) noexcept;

    /**
     * Writes signed bits up to 32 bits.
     *
     * \param data Data to write.
     * \param numBits Number of bits to write.
     *
     * \return Success or error code.
     */
    Result<void> writeSignedBits(int32_t data, uint8_t numBits = 32) noexcept;

    /**
     * Writes signed bits up to 64 bits.
     *
     * \param data Data to write.
     * \param numBits Number of bits to write.
     *
     * \return Success or error code.
     */
    Result<void> writeSignedBits64(int64_t data, uint8_t numBits = 64) noexcept;

    /**
     * Writes signed variable integer up to 64 bits.
     *
     * \param data Varint64 to write.
     *
     * \return Success or error code.
     */
    Result<void> writeVarInt64(int64_t data) noexcept;

    /**
     * Writes signed variable integer up to 32 bits.
     *
     * \param data Varint32 to write.
     *
     * \return Success or error code.
     */
    Result<void> writeVarInt32(int32_t data) noexcept;

    /**
     * Writes signed variable integer up to 16 bits.
     *
     * \param data Varint16 to write.
     *
     * \return Success or error code.
     */
    Result<void> writeVarInt16(int16_t data) noexcept;

    /**
     * Writes unsigned variable integer up to 64 bits.
     *
     * \param data Varuint64 to write.
     *
     * \return Success or error code.
     */
    Result<void> writeVarUInt64(uint64_t data) noexcept;

    /**
     * Writes unsigned variable integer up to 32 bits.
     *
     * \param data Varuint32 to write.
     *
     * \return Success or error code.
     */
    Result<void> writeVarUInt32(uint32_t data) noexcept;

    /**
     * Writes unsigned variable integer up to 16 bits.
     *
     * \param data Varuint16 to write.
     *
     * \return Success or error code.
     */
    Result<void> writeVarUInt16(uint16_t data) noexcept;

    /**
     * Writes signed variable integer up to 72 bits.
     *
     * \param data Varuint64 to write.
     *
     * \return Success or error code.
     */
    Result<void> writeVarInt(int64_t data) noexcept;

    /**
     * Writes unsigned variable integer up to 72 bits.
     *
     * \param data Varuint64 to write.
     *
     * \return Success or error code.
     */
    Result<void> writeVarUInt(uint64_t data) noexcept;

    /**
     * Writes variable size integer up to 40 bits.
     *
     * \param data Varsize to write.
     *
     * \return Success or error code.
     */
    Result<void> writeVarSize(uint32_t data) noexcept;

    /**
     * Writes 16-bit float.
     *
     * \param data Float16 to write.
     *
     * \return Success or error code.
     */
    Result<void> writeFloat16(float data) noexcept;

    /**
     * Writes 32-bit float.
     *
     * \param data Float32 to write.
     *
     * \return Success or error code.
     */
    Result<void> writeFloat32(float data) noexcept;

    /**
     * Writes 64-bit float.
     *
     * \param data Float64 to write.
     *
     * \return Success or error code.
     */
    Result<void> writeFloat64(double data) noexcept;

    /**
     * Writes bytes.
     *
     * \param data Bytes to write.
     *
     * \return Success or error code.
     */
    Result<void> writeBytes(Span<const uint8_t> data) noexcept;

    /**
     * Writes UTF-8 string.
     *
     * \param data String view to write.
     *
     * \return Success or error code.
     */
    Result<void> writeString(StringView data) noexcept;

    /**
     * Writes bool as a single bit.
     *
     * \param data Bool to write.
     *
     * \return Success or error code.
     */
    Result<void> writeBool(bool data) noexcept;

    /**
     * Writes bit buffer.
     *
     * \param bitBuffer Bit buffer to write.
     *
     * \return Success or error code.
     */
    template <typename ALLOC>
    Result<void> writeBitBuffer(const BasicBitBuffer<ALLOC>& bitBuffer) noexcept
    {
        const size_t bitSize = bitBuffer.getBitSize();
        auto sizeResult = writeVarSize(convertSizeToUInt32(bitSize));
        if (sizeResult.isError())
        {
            return sizeResult;
        }

        Span<const uint8_t> buffer = bitBuffer.getData();
        size_t numBytesToWrite = bitSize / 8;
        const uint8_t numRestBits = static_cast<uint8_t>(bitSize - numBytesToWrite * 8);
        const BitPosType beginBitPosition = getBitPosition();
        const Span<const uint8_t>::iterator itEnd = buffer.begin() + numBytesToWrite;
        if ((beginBitPosition & 0x07U) != 0)
        {
            // we are not aligned to byte
            for (Span<const uint8_t>::iterator it = buffer.begin(); it != itEnd; ++it)
            {
                auto result = writeUnsignedBits(*it, 8);
                if (result.isError())
                {
                    return result;
                }
            }
        }
        else
        {
            // we are aligned to byte
            auto posResult = setBitPosition(beginBitPosition + numBytesToWrite * 8);
            if (posResult.isError())
            {
                return posResult;
            }
            if (hasWriteBuffer())
            {
                (void)std::copy(buffer.begin(), buffer.begin() + numBytesToWrite,
                        m_buffer.data() + beginBitPosition / 8);
            }
        }

        if (numRestBits > 0)
        {
            auto result = writeUnsignedBits(static_cast<uint32_t>(*itEnd) >> (8U - numRestBits), numRestBits);
            if (result.isError())
            {
                return result;
            }
        }
        
        return Result<void>::success();
    }

    /**
     * Gets current bit position.
     *
     * \return Current bit position.
     */
    BitPosType getBitPosition() const
    {
        return m_bitIndex;
    }

    /**
     * Sets current bit position. Use with caution!
     *
     * \param position New bit position.
     *
     * \return Success or error code.
     */
    Result<void> setBitPosition(BitPosType position) noexcept;

    /**
     * Moves current bit position to perform the requested bit alignment.
     *
     * \param alignment Size of the alignment in bits.
     *
     * \return Success or error code.
     */
    Result<void> alignTo(size_t alignment) noexcept;

    /**
     * Gets whether the writer has assigned a write buffer.
     *
     * \return True when a buffer is assigned. False otherwise.
     */
    bool hasWriteBuffer() const
    {
        return m_buffer.data() != nullptr;
    }

    /**
     * Gets the write buffer.
     *
     * \return Pointer to the beginning of write buffer.
     */
    const uint8_t* getWriteBuffer() const noexcept;

    /**
     * Gets the write buffer as span.
     *
     * \return Span which represents the write buffer.
     */
    Span<const uint8_t> getBuffer() const noexcept;

    /**
     * Gets size of the underlying buffer in bits.
     *
     * \return Buffer bit size.
     */
    size_t getBufferBitSize() const
    {
        return m_bufferBitSize;
    }

private:
    Result<void> writeUnsignedBits(uint32_t data, uint8_t numBits) noexcept;
    Result<void> writeUnsignedBits64(uint64_t data, uint8_t numBits) noexcept;
    Result<void> writeSignedVarNum(int64_t value, size_t maxVarBytes, size_t numVarBytes) noexcept;
    Result<void> writeUnsignedVarNum(uint64_t value, size_t maxVarBytes, size_t numVarBytes) noexcept;
    Result<void> writeVarNum(uint64_t value, bool hasSign, bool isNegative, size_t maxVarBytes, size_t numVarBytes) noexcept;

    Result<void> checkCapacity(size_t bitSize) const noexcept;

    Span<uint8_t> m_buffer;
    size_t m_bitIndex;
    size_t m_bufferBitSize;
};

} // namespace zserio

#endif // ifndef ZSERIO_BIT_STREAM_WRITER_H_INC
