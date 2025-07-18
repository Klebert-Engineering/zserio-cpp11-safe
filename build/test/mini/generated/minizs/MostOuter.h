/**
 * Automatically generated by Zserio C++11 Safe generator version 1.2.1 using Zserio core 2.16.1.
 * Generator setup: writerCode, settersCode, pubsubCode, serviceCode, sqlCode, polymorphicAllocator.
 */

#ifndef MINIZS_MOST_OUTER_H
#define MINIZS_MOST_OUTER_H

#include <zserio/CppRuntimeVersion.h>
#if CPP_EXTENSION_RUNTIME_VERSION_NUMBER != 1002001
    #error Version mismatch between Zserio runtime library and Zserio C++ generator!
    #error Please update your Zserio runtime library to the version 1.2.1.
#endif

#include <zserio/Traits.h>
#include <zserio/BitStreamReader.h>
#include <zserio/BitStreamWriter.h>
#include <zserio/AllocatorPropagatingCopy.h>
#include <zserio/pmr/PolymorphicAllocator.h>
#include <memory>
#include <zserio/ArrayTraits.h>
#include <zserio/Types.h>

#include <minizs/Outer.h>

namespace minizs
{

class MostOuter
{
public:
    using allocator_type = ::zserio::pmr::PropagatingPolymorphicAllocator<>;

    MostOuter() noexcept :
            MostOuter(allocator_type())
    {}

    explicit MostOuter(const allocator_type& allocator) noexcept;
    
    static ::zserio::Result<MostOuter> create(::zserio::BitStreamReader& in, const allocator_type& allocator = allocator_type());
    
    static ::zserio::Result<MostOuter> deserialize(::zserio::BitStreamReader& in, const allocator_type& allocator = allocator_type())
    {
        return create(in, allocator);
    }

    template <typename ZSERIO_T_outer = ::minizs::Outer>
    MostOuter(
            uint8_t numOfInner_,
            ZSERIO_T_outer&& outer_,
            const allocator_type& allocator = allocator_type()) :
            MostOuter(allocator)
    {
        m_numOfInner_ = numOfInner_;
        m_outer_ = ::std::forward<ZSERIO_T_outer>(outer_);
    }


    ~MostOuter() = default;

    MostOuter(const MostOuter& other);
    MostOuter& operator=(const MostOuter& other);

    MostOuter(MostOuter&& other);
    MostOuter& operator=(MostOuter&& other);

    MostOuter(::zserio::PropagateAllocatorT,
            const MostOuter& other, const allocator_type& allocator);

    ::zserio::Result<void> initializeChildren();

    uint8_t getNumOfInner() const;
    void setNumOfInner(uint8_t numOfInner_);

    const ::minizs::Outer& getOuter() const;
    ::minizs::Outer& getOuter();
    void setOuter(const ::minizs::Outer& outer_);
    void setOuter(::minizs::Outer&& outer_);

    ::zserio::Result<size_t> bitSizeOf(size_t bitPosition = 0) const;

    ::zserio::Result<size_t> initializeOffsets(size_t bitPosition = 0);

    bool operator==(const MostOuter& other) const;

    bool operator<(const MostOuter& other) const;

    uint32_t hashCode() const;

    ::zserio::Result<void> write(::zserio::BitStreamWriter& out) const;

private:
    uint8_t readNumOfInner(::zserio::BitStreamReader& in);
    ::zserio::Result<::minizs::Outer> readOuter(::zserio::BitStreamReader& in,
            const allocator_type& allocator);

    bool m_areChildrenInitialized;
    uint8_t m_numOfInner_;
    ::minizs::Outer m_outer_;
};

} // namespace minizs

#endif // MINIZS_MOST_OUTER_H
