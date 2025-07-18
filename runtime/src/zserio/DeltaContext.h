#ifndef ZSERIO_DELTA_CONTEXT_H_INC
#define ZSERIO_DELTA_CONTEXT_H_INC

#include <type_traits>

#include "zserio/BitStreamReader.h"
#include "zserio/BitStreamWriter.h"
#include "zserio/RebindAlloc.h"
#include "zserio/Traits.h"
#include "zserio/Types.h"
#include "zserio/UniquePtr.h"
#include "zserio/Vector.h"

namespace zserio
{

namespace detail
{

// calculates bit length on delta provided as an absolute number
inline uint8_t absDeltaBitLength(uint64_t absDelta)
{
    uint8_t result = 0;
    while (absDelta > 0)
    {
        result++;
        absDelta >>= 1U;
    }

    return result;
}

// calculates bit length, emulates Python bit_length to keep same logic
template <typename T>
uint8_t calcBitLength(T lhs, T rhs)
{
    const uint64_t absDelta = lhs > rhs
            ? static_cast<uint64_t>(lhs) - static_cast<uint64_t>(rhs)
            : static_cast<uint64_t>(rhs) - static_cast<uint64_t>(lhs);

    return absDeltaBitLength(absDelta);
}

// calculates delta, doesn't check for possible int64_t overflow since it's used only in cases where it's
// already known that overflow cannot occur
template <typename T>
int64_t calcUncheckedDelta(T lhs, uint64_t rhs)
{
    return static_cast<int64_t>(static_cast<uint64_t>(lhs) - rhs);
}

} // namespace detail

/**
 * Context for delta packing created for each packable field.
 *
 * Contexts are always newly created for each array operation (bitSizeOfPacked, initializeOffsetsPacked,
 * readPacked, writePacked). They must be initialized at first via calling the init method for each packable
 * element present in the array. After the full initialization, only a single method (bitSizeOf, read, write)
 * can be repeatedly called for exactly the same sequence of packable elements.
 */
class DeltaContext
{
public:
    /**
     * Method generated by default.
     * \{
     */
    DeltaContext() = default;
    ~DeltaContext() = default;

    DeltaContext(DeltaContext&& other) = default;
    DeltaContext& operator=(DeltaContext&& other) = default;
    DeltaContext(const DeltaContext& other) = default;
    DeltaContext& operator=(const DeltaContext& other) = default;
    /**
     * \}
     */

    /**
     * Calls the initialization step for a single element.
     *
     * \param owner Owner of the packed element.
     * \param element Current element.
     */
    template <typename ARRAY_TRAITS, typename OWNER_TYPE>
    void init(const OWNER_TYPE& owner, typename ARRAY_TRAITS::ElementType element)
    {
        m_numElements++;
        m_unpackedBitSize += bitSizeOfUnpacked<ARRAY_TRAITS>(owner, element);

        if (!isFlagSet(INIT_STARTED_FLAG))
        {
            setFlag(INIT_STARTED_FLAG);
            m_previousElement = static_cast<uint64_t>(element);
            m_firstElementBitSize = static_cast<uint8_t>(m_unpackedBitSize);
        }
        else
        {
            if (m_maxBitNumber <= MAX_BIT_NUMBER_LIMIT)
            {
                setFlag(IS_PACKED_FLAG);
                const auto previousElement = static_cast<typename ARRAY_TRAITS::ElementType>(m_previousElement);
                const uint8_t maxBitNumber = detail::calcBitLength(element, previousElement);
                if (maxBitNumber > m_maxBitNumber)
                {
                    m_maxBitNumber = maxBitNumber;
                    if (m_maxBitNumber > MAX_BIT_NUMBER_LIMIT)
                    {
                        resetFlag(IS_PACKED_FLAG);
                    }
                }
                m_previousElement = static_cast<uint64_t>(element);
            }
        }
    }

    /**
     * Returns length of the packed element stored in the bit stream in bits.
     *
     * \param owner Owner of the packed element.
     * \param element Value of the current element.
     *
     * \return Length of the packed element stored in the bit stream in bits.
     */
    template <typename ARRAY_TRAITS, typename OWNER_TYPE>
    size_t bitSizeOf(const OWNER_TYPE& owner, typename ARRAY_TRAITS::ElementType element)
    {
        if (!isFlagSet(PROCESSING_STARTED_FLAG))
        {
            setFlag(PROCESSING_STARTED_FLAG);
            finishInit();

            return bitSizeOfDescriptor() + bitSizeOfUnpacked<ARRAY_TRAITS>(owner, element);
        }
        else if (!isFlagSet(IS_PACKED_FLAG))
        {
            return bitSizeOfUnpacked<ARRAY_TRAITS>(owner, element);
        }
        else
        {
            return static_cast<size_t>(m_maxBitNumber) + (m_maxBitNumber > 0 ? 1 : 0);
        }
    }

    /**
     * Reads a packed element from the bit stream.
     *
     * \param owner Owner of the packed element.
     * \param in Bit stream reader.
     *
     * \return Value of the packed element.
     */
    template <typename ARRAY_TRAITS, typename OWNER_TYPE>
    Result<typename ARRAY_TRAITS::ElementType> read(const OWNER_TYPE& owner, BitStreamReader& in)
    {
        if (!isFlagSet(PROCESSING_STARTED_FLAG))
        {
            setFlag(PROCESSING_STARTED_FLAG);
            auto descResult = readDescriptor(in);
            if (!descResult.isSuccess())
            {
                return Result<typename ARRAY_TRAITS::ElementType>::error(descResult.getError());
            }

            return readUnpacked<ARRAY_TRAITS>(owner, in);
        }
        else if (!isFlagSet(IS_PACKED_FLAG))
        {
            return readUnpacked<ARRAY_TRAITS>(owner, in);
        }
        else
        {
            if (m_maxBitNumber > 0)
            {
                auto deltaResult = in.readSignedBits64(static_cast<uint8_t>(m_maxBitNumber + 1));
                if (!deltaResult.isSuccess())
                {
                    return Result<typename ARRAY_TRAITS::ElementType>::error(deltaResult.getError());
                }
                const int64_t delta = deltaResult.getValue();
                const typename ARRAY_TRAITS::ElementType element =
                        static_cast<typename ARRAY_TRAITS::ElementType>(
                                m_previousElement + static_cast<uint64_t>(delta));
                m_previousElement = static_cast<uint64_t>(element);
            }

            return Result<typename ARRAY_TRAITS::ElementType>::success(
                    static_cast<typename ARRAY_TRAITS::ElementType>(m_previousElement));
        }
    }

    /**
     * Writes the packed element to the bit stream.
     *
     * \param owner Owner of the packed element.
     * \param out Bit stream writer.
     * \param element Value of the current element.
     */
    template <typename ARRAY_TRAITS, typename OWNER_TYPE>
    Result<void> write(const OWNER_TYPE& owner, BitStreamWriter& out, typename ARRAY_TRAITS::ElementType element)
    {
        if (!isFlagSet(PROCESSING_STARTED_FLAG))
        {
            setFlag(PROCESSING_STARTED_FLAG);
            finishInit();
            auto descResult = writeDescriptor(out);
            if (!descResult.isSuccess())
            {
                return descResult;
            }

            return writeUnpacked<ARRAY_TRAITS>(owner, out, element);
        }
        else if (!isFlagSet(IS_PACKED_FLAG))
        {
            return writeUnpacked<ARRAY_TRAITS>(owner, out, element);
        }
        else
        {
            if (m_maxBitNumber > 0)
            {
                // it's already checked in the init phase that the delta will fit into int64_t
                const int64_t delta = detail::calcUncheckedDelta(element, m_previousElement);
                auto writeResult = out.writeSignedBits64(delta, static_cast<uint8_t>(m_maxBitNumber + 1));
                if (!writeResult.isSuccess())
                {
                    return writeResult;
                }
                m_previousElement = static_cast<uint64_t>(element);
            }
            return Result<void>::success();
        }
    }

    // overloads with dummy owner

    /**
     * Calls the initialization step for a single element.
     *
     * \param element Current element.
     */
    template <typename ARRAY_TRAITS>
    void init(typename ARRAY_TRAITS::ElementType element)
    {
        init<ARRAY_TRAITS>(DummyOwner(), element);
    }

    /**
     * Returns length of the packed element stored in the bit stream in bits.
     *
     * \param element Value of the current element.
     *
     * \return Length of the packed element stored in the bit stream in bits.
     */
    template <typename ARRAY_TRAITS>
    size_t bitSizeOf(typename ARRAY_TRAITS::ElementType element)
    {
        return bitSizeOf<ARRAY_TRAITS>(DummyOwner(), element);
    }

    /**
     * Reads a packed element from the bit stream.
     *
     * \param in Bit stream reader.
     *
     * \return Value of the packed element.
     */
    template <typename ARRAY_TRAITS>
    Result<typename ARRAY_TRAITS::ElementType> read(BitStreamReader& in)
    {
        return read<ARRAY_TRAITS>(DummyOwner(), in);
    }

    /**
     * Writes the packed element to the bit stream.
     *
     * \param out Bit stream writer.
     * \param element Value of the current element.
     */
    template <typename ARRAY_TRAITS>
    Result<void> write(BitStreamWriter& out, typename ARRAY_TRAITS::ElementType element)
    {
        return write<ARRAY_TRAITS>(DummyOwner(), out, element);
    }

private:
    struct DummyOwner
    {};

    void finishInit()
    {
        if (isFlagSet(IS_PACKED_FLAG))
        {
            const size_t deltaBitSize = static_cast<size_t>(m_maxBitNumber) + (m_maxBitNumber > 0 ? 1 : 0);
            const size_t packedBitSizeWithDescriptor = 1U + MAX_BIT_NUMBER_BITS + // descriptor
                    m_firstElementBitSize + (m_numElements - 1) * deltaBitSize;
            const size_t unpackedBitSizeWithDescriptor = 1 + m_unpackedBitSize;
            if (packedBitSizeWithDescriptor >= unpackedBitSizeWithDescriptor)
            {
                resetFlag(IS_PACKED_FLAG);
            }
        }
    }

    size_t bitSizeOfDescriptor() const
    {
        if (isFlagSet(IS_PACKED_FLAG))
        {
            return 1 + MAX_BIT_NUMBER_BITS;
        }
        else
        {
            return 1;
        }
    }

    template <typename ARRAY_TRAITS,
            typename std::enable_if<has_owner_type<ARRAY_TRAITS>::value, int>::type = 0>
    static size_t bitSizeOfUnpacked(
            const typename ARRAY_TRAITS::OwnerType& owner, typename ARRAY_TRAITS::ElementType element)
    {
        return ARRAY_TRAITS::bitSizeOf(owner, element);
    }

    template <typename ARRAY_TRAITS,
            typename std::enable_if<!has_owner_type<ARRAY_TRAITS>::value, int>::type = 0>
    static size_t bitSizeOfUnpacked(const DummyOwner&, typename ARRAY_TRAITS::ElementType element)
    {
        return ARRAY_TRAITS::bitSizeOf(element);
    }

    Result<void> readDescriptor(BitStreamReader& in)
    {
        auto boolResult = in.readBool();
        if (!boolResult.isSuccess())
        {
            return Result<void>::error(boolResult.getError());
        }
        
        if (boolResult.getValue())
        {
            setFlag(IS_PACKED_FLAG);
            auto bitsResult = in.readBits(MAX_BIT_NUMBER_BITS);
            if (!bitsResult.isSuccess())
            {
                return Result<void>::error(bitsResult.getError());
            }
            m_maxBitNumber = static_cast<uint8_t>(bitsResult.getValue());
        }
        else
        {
            resetFlag(IS_PACKED_FLAG);
        }
        return Result<void>::success();
    }

    template <typename ARRAY_TRAITS,
            typename std::enable_if<has_owner_type<ARRAY_TRAITS>::value, int>::type = 0>
    Result<typename ARRAY_TRAITS::ElementType> readUnpacked(
            const typename ARRAY_TRAITS::OwnerType& owner, BitStreamReader& in)
    {
        auto elementResult = ARRAY_TRAITS::read(owner, in);
        if (!elementResult.isSuccess())
        {
            return elementResult;
        }
        const auto element = elementResult.getValue();
        m_previousElement = static_cast<uint64_t>(element);
        return Result<typename ARRAY_TRAITS::ElementType>::success(element);
    }

    template <typename ARRAY_TRAITS,
            typename std::enable_if<!has_owner_type<ARRAY_TRAITS>::value, int>::type = 0>
    Result<typename ARRAY_TRAITS::ElementType> readUnpacked(const DummyOwner&, BitStreamReader& in)
    {
        auto elementResult = ARRAY_TRAITS::read(in);
        if (!elementResult.isSuccess())
        {
            return elementResult;
        }
        const auto element = elementResult.getValue();
        m_previousElement = static_cast<uint64_t>(element);
        return Result<typename ARRAY_TRAITS::ElementType>::success(element);
    }

    Result<void> writeDescriptor(BitStreamWriter& out) const
    {
        const bool isPacked = isFlagSet(IS_PACKED_FLAG);
        auto boolResult = out.writeBool(isPacked);
        if (!boolResult.isSuccess())
        {
            return boolResult;
        }
        if (isPacked)
        {
            auto bitsResult = out.writeBits(m_maxBitNumber, MAX_BIT_NUMBER_BITS);
            if (!bitsResult.isSuccess())
            {
                return bitsResult;
            }
        }
        return Result<void>::success();
    }

    template <typename ARRAY_TRAITS,
            typename std::enable_if<has_owner_type<ARRAY_TRAITS>::value, int>::type = 0>
    Result<void> writeUnpacked(const typename ARRAY_TRAITS::OwnerType& owner, BitStreamWriter& out,
            typename ARRAY_TRAITS::ElementType element)
    {
        m_previousElement = static_cast<uint64_t>(element);
        return ARRAY_TRAITS::write(owner, out, element);
    }

    template <typename ARRAY_TRAITS,
            typename std::enable_if<!has_owner_type<ARRAY_TRAITS>::value, int>::type = 0>
    Result<void> writeUnpacked(const DummyOwner&, BitStreamWriter& out, typename ARRAY_TRAITS::ElementType element)
    {
        m_previousElement = static_cast<uint64_t>(element);
        return ARRAY_TRAITS::write(out, element);
    }

    void setFlag(uint8_t flagMask)
    {
        m_flags |= flagMask;
    }

    void resetFlag(uint8_t flagMask)
    {
        m_flags &= static_cast<uint8_t>(~flagMask);
    }

    bool isFlagSet(uint8_t flagMask) const
    {
        return ((m_flags & flagMask) != 0);
    }

    static const uint8_t MAX_BIT_NUMBER_BITS = 6;
    static const uint8_t MAX_BIT_NUMBER_LIMIT = 62;

    static const uint8_t INIT_STARTED_FLAG = 0x01;
    static const uint8_t IS_PACKED_FLAG = 0x02;
    static const uint8_t PROCESSING_STARTED_FLAG = 0x04;

    uint64_t m_previousElement = 0;
    uint8_t m_maxBitNumber = 0;
    uint8_t m_flags = 0x00;

    uint8_t m_firstElementBitSize = 0;
    uint32_t m_numElements = 0;
    size_t m_unpackedBitSize = 0;
};

} // namespace zserio

#endif // ZSERIO_DELTA_CONTEXT_H_INC
