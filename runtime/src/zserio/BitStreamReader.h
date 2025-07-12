#ifndef ZSERIO_BIT_STREAM_READER_H_INC
#define ZSERIO_BIT_STREAM_READER_H_INC

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>

#include "zserio/BitBuffer.h"
#include "zserio/ErrorCode.h"
#include "zserio/RebindAlloc.h"
#include "zserio/Result.h"
#include "zserio/Span.h"
#include "zserio/String.h"
#include "zserio/Types.h"
#include "zserio/Vector.h"

namespace zserio
{

/**
 * Reader class which allows to read various data from the bit stream.
 */
class BitStreamReader
{
public:
    /** Type for bit position. */
    using BitPosType = size_t;

    /**
     * Context of the reader defining its state.
     */
    struct ReaderContext
    {
        /**
         * Constructor.
         *
         * \param readBuffer Span to the buffer to read.
         * \param readBufferBitSize Size of the buffer in bits.
         */
        explicit ReaderContext(Span<const uint8_t> readBuffer, size_t readBufferBitSize);

        /**
         * Destructor.
         */
        ~ReaderContext() = default;

        /**
         * Copying and moving is disallowed!
         * \{
         */
        ReaderContext(const ReaderContext&) = delete;
        ReaderContext& operator=(const ReaderContext&) = delete;

        ReaderContext(const ReaderContext&&) = delete;
        ReaderContext& operator=(const ReaderContext&&) = delete;
        /**
         * \}
         */

        Span<const uint8_t> buffer; /**< Buffer to read from. */
        const BitPosType bufferBitSize; /**< Size of the buffer in bits. */

        uintptr_t cache; /**< Bit cache to optimize bit reading. */
        uint8_t cacheNumBits; /**< Num bits available in the bit cache. */

        BitPosType bitIndex; /**< Current bit index. */
    };

    /**
     * Constructor from raw buffer.
     *
     * \param buffer Pointer to the buffer to read.
     * \param bufferByteSize Size of the buffer in bytes.
     */
    explicit BitStreamReader(const uint8_t* buffer, size_t bufferByteSize);

    /**
     * Constructor from buffer passed as a Span.
     *
     * \param buffer Buffer to read.
     */
    explicit BitStreamReader(Span<const uint8_t> buffer);

    /**
     * Constructor from buffer passed as a Span with exact bit size.
     *
     * \param buffer Buffer to read.
     * \param bufferBitSize Size of the buffer in bits.
     */
    explicit BitStreamReader(Span<const uint8_t> buffer, size_t bufferBitSize);

    /**
     * Constructor from raw buffer with exact bit size.
     *
     * \param buffer Pointer to buffer to read.
     * \param bufferBitSize Size of the buffer in bits.
     */
    explicit BitStreamReader(const uint8_t* buffer, size_t bufferBitSize, BitsTag);

    /**
     * Constructor from bit buffer.
     *
     * \param bitBuffer Bit buffer to read from.
     */
    template <typename ALLOC>
    explicit BitStreamReader(const BasicBitBuffer<ALLOC>& bitBuffer) :
            BitStreamReader(bitBuffer.getData(), bitBuffer.getBitSize())
    {}

    /**
     * Destructor.
     */
    ~BitStreamReader() = default;

    /**
     * Reads unsigned bits up to 32-bits.
     *
     * \param numBits Number of bits to read.
     *
     * \return Result with read bits or error code.
     */
    Result<uint32_t> readBits(uint8_t numBits = 32) noexcept;

    /**
     * Reads unsigned bits up to 64-bits.
     *
     * \param numBits Number of bits to read.
     *
     * \return Result with read bits or error code.
     */
    Result<uint64_t> readBits64(uint8_t numBits = 64) noexcept;

    /**
     * Reads signed bits up to 32-bits.
     *
     * \param numBits Number of bits to read.
     *
     * \return Result with read bits or error code.
     */
    Result<int32_t> readSignedBits(uint8_t numBits = 32) noexcept;

    /**
     * Reads signed bits up to 64-bits.
     *
     * \param numBits Number of bits to read.
     *
     * \return Result with read bits or error code.
     */
    Result<int64_t> readSignedBits64(uint8_t numBits = 64) noexcept;

    /**
     * Reads signed variable integer up to 64 bits.
     *
     * \return Result with read varint64 or error code.
     */
    Result<int64_t> readVarInt64() noexcept;

    /**
     * Reads signed variable integer up to 32 bits.
     *
     * \return Result with read varint32 or error code.
     */
    Result<int32_t> readVarInt32() noexcept;

    /**
     * Reads signed variable integer up to 16 bits.
     *
     * \return Result with read varint16 or error code.
     */
    Result<int16_t> readVarInt16() noexcept;

    /**
     * Read unsigned variable integer up to 64 bits.
     *
     * \return Result with read varuint64 or error code.
     */
    Result<uint64_t> readVarUInt64() noexcept;

    /**
     * Read unsigned variable integer up to 32 bits.
     *
     * \return Result with read varuint32 or error code.
     */
    Result<uint32_t> readVarUInt32() noexcept;

    /**
     * Read unsigned variable integer up to 16 bits.
     *
     * \return Result with read varuint16 or error code.
     */
    Result<uint16_t> readVarUInt16() noexcept;

    /**
     * Reads signed variable integer up to 72 bits.
     *
     * \return Result with read varint or error code.
     */
    Result<int64_t> readVarInt() noexcept;

    /**
     * Read unsigned variable integer up to 72 bits.
     *
     * \return Result with read varuint or error code.
     */
    Result<uint64_t> readVarUInt() noexcept;

    /**
     * Read variable size integer up to 40 bits.
     *
     * \return Result with read varsize or error code.
     */
    Result<uint32_t> readVarSize() noexcept;

    /**
     * Reads 16-bit float.
     *
     * \return Result with read float16 or error code.
     */
    Result<float> readFloat16() noexcept;

    /**
     * Reads 32-bit float.
     *
     * \return Result with read float32 or error code.
     */
    Result<float> readFloat32() noexcept;

    /**
     * Reads 64-bit float double.
     *
     * \return Result with read float64 or error code.
     */
    Result<double> readFloat64() noexcept;

    /**
     * Reads bytes.
     *
     * \param alloc Allocator to use.
     *
     * \return Result with read bytes as a vector or error code.
     */
    template <typename ALLOC = std::allocator<uint8_t>>
    Result<vector<uint8_t, ALLOC>> readBytes(const ALLOC& alloc = ALLOC()) noexcept
    {
        auto lenResult = readVarSize();
        if (lenResult.isError())
        {
            return Result<vector<uint8_t, ALLOC>>::error(lenResult.getError());
        }
        const size_t len = static_cast<size_t>(lenResult.getValue());
        
        const BitPosType beginBitPosition = getBitPosition();
        if ((beginBitPosition & 0x07U) != 0)
        {
            // we are not aligned to byte
            vector<uint8_t, ALLOC> value{alloc};
            // TODO: This reserve() may abort if allocation fails with -fno-exceptions!
            value.reserve(len);
            for (size_t i = 0; i < len; ++i)
            {
                auto byteResult = readByte();
                if (byteResult.isError())
                {
                    return Result<vector<uint8_t, ALLOC>>::error(byteResult.getError());
                }
                value.push_back(byteResult.getValue());
            }
            return Result<vector<uint8_t, ALLOC>>::success(std::move(value));
        }
        else
        {
            // we are aligned to byte
            auto setPosResult = setBitPosition(beginBitPosition + len * 8);
            if (setPosResult.isError())
            {
                return Result<vector<uint8_t, ALLOC>>::error(setPosResult.getError());
            }
            Span<const uint8_t>::iterator beginIt = m_context.buffer.begin() + beginBitPosition / 8;
            return Result<vector<uint8_t, ALLOC>>::success(
                vector<uint8_t, ALLOC>(beginIt, beginIt + len, alloc));
        }
    }

    /**
     * Reads an UTF-8 string.
     *
     * \param alloc Allocator to use.
     *
     * \return Result with read string or error code.
     */
    template <typename ALLOC = std::allocator<char>>
    Result<string<ALLOC>> readString(const ALLOC& alloc = ALLOC()) noexcept
    {
        auto lenResult = readVarSize();
        if (lenResult.isError())
        {
            return Result<string<ALLOC>>::error(lenResult.getError());
        }
        const size_t len = static_cast<size_t>(lenResult.getValue());
        
        const BitPosType beginBitPosition = getBitPosition();
        if ((beginBitPosition & 0x07U) != 0)
        {
            // we are not aligned to byte
            string<ALLOC> value{alloc};
            // TODO: This reserve() may abort if allocation fails with -fno-exceptions!
            value.reserve(len);
            for (size_t i = 0; i < len; ++i)
            {
                using char_traits = std::char_traits<char>;
                auto byteResult = readByte();
                if (byteResult.isError())
                {
                    return Result<string<ALLOC>>::error(byteResult.getError());
                }
                const char readCharacter =
                        char_traits::to_char_type(static_cast<char_traits::int_type>(byteResult.getValue()));
                value.push_back(readCharacter);
            }
            return Result<string<ALLOC>>::success(std::move(value));
        }
        else
        {
            // we are aligned to byte
            auto setPosResult = setBitPosition(beginBitPosition + len * 8);
            if (setPosResult.isError())
            {
                return Result<string<ALLOC>>::error(setPosResult.getError());
            }
            Span<const uint8_t>::iterator beginIt = m_context.buffer.begin() + beginBitPosition / 8;
            return Result<string<ALLOC>>::success(string<ALLOC>(beginIt, beginIt + len, alloc));
        }
    }

    /**
     * Reads bool as a single bit.
     *
     * \return Result with read bool value or error code.
     */
    Result<bool> readBool() noexcept;

    /**
     * Reads a bit buffer.
     *
     * \param alloc Allocator to use.
     *
     * \return Result with read bit buffer or error code.
     */
    template <typename ALLOC = std::allocator<uint8_t>>
    Result<BasicBitBuffer<RebindAlloc<ALLOC, uint8_t>>> readBitBuffer(const ALLOC& allocator = ALLOC()) noexcept
    {
        auto sizeResult = readVarSize();
        if (sizeResult.isError())
        {
            return Result<BasicBitBuffer<RebindAlloc<ALLOC, uint8_t>>>::error(sizeResult.getError());
        }
        const size_t bitSize = static_cast<size_t>(sizeResult.getValue());
        const size_t numBytesToRead = bitSize / 8;
        const uint8_t numRestBits = static_cast<uint8_t>(bitSize - numBytesToRead * 8);
        
        // TODO: BitBuffer constructor may abort if allocation fails with -fno-exceptions!
        BasicBitBuffer<RebindAlloc<ALLOC, uint8_t>> bitBuffer(bitSize, allocator);
        Span<uint8_t> buffer = bitBuffer.getData();
        const BitPosType beginBitPosition = getBitPosition();
        const Span<uint8_t>::iterator itEnd = buffer.begin() + numBytesToRead;
        if ((beginBitPosition & 0x07U) != 0)
        {
            // we are not aligned to byte
            for (Span<uint8_t>::iterator it = buffer.begin(); it != itEnd; ++it)
            {
                auto byteResult = readBits(8);
                if (byteResult.isError())
                {
                    return Result<BasicBitBuffer<RebindAlloc<ALLOC, uint8_t>>>::error(byteResult.getError());
                }
                *it = static_cast<uint8_t>(byteResult.getValue());
            }
        }
        else
        {
            // we are aligned to byte
            auto setPosResult = setBitPosition(beginBitPosition + numBytesToRead * 8);
            if (setPosResult.isError())
            {
                return Result<BasicBitBuffer<RebindAlloc<ALLOC, uint8_t>>>::error(setPosResult.getError());
            }
            Span<const uint8_t>::const_iterator sourceIt = m_context.buffer.begin() + beginBitPosition / 8;
            (void)std::copy(sourceIt, sourceIt + numBytesToRead, buffer.begin());
        }

        if (numRestBits > 0)
        {
            auto bitsResult = readBits(numRestBits);
            if (bitsResult.isError())
            {
                return Result<BasicBitBuffer<RebindAlloc<ALLOC, uint8_t>>>::error(bitsResult.getError());
            }
            *itEnd = static_cast<uint8_t>(bitsResult.getValue() << (8U - numRestBits));
        }

        return Result<BasicBitBuffer<RebindAlloc<ALLOC, uint8_t>>>::success(std::move(bitBuffer));
    }

    /**
     * Gets current bit position.
     *
     * \return Current bit position.
     */
    BitPosType getBitPosition() const
    {
        return m_context.bitIndex;
    }

    /**
     * Sets current bit position. Use with caution!
     *
     * \param position New bit position.
     *
     * \return Result indicating success or error code.
     */
    Result<void> setBitPosition(BitPosType position) noexcept;

    /**
     * Moves current bit position to perform the requested bit alignment.
     *
     * \param alignment Size of the alignment in bits.
     *
     * \return Result indicating success or error code.
     */
    Result<void> alignTo(size_t alignment) noexcept;

    /**
     * Gets size of the underlying buffer in bits.
     *
     * \return Buffer bit size.
     */
    size_t getBufferBitSize() const
    {
        return m_context.bufferBitSize;
    }

private:
    Result<uint8_t> readByte() noexcept;

    ReaderContext m_context;
};

} // namespace zserio

#endif // ifndef ZSERIO_BIT_STREAM_READER_H_INC
