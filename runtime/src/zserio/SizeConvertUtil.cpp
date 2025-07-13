#include <cstddef>
#include <limits>

#include "zserio/RuntimeArch.h"
#include "zserio/SizeConvertUtil.h"

namespace zserio
{

Result<uint32_t> convertSizeToUInt32(size_t value) noexcept
{
#ifdef ZSERIO_RUNTIME_64BIT
    if (value > static_cast<size_t>(std::numeric_limits<uint32_t>::max()))
    {
        return Result<uint32_t>::error(ErrorCode::NumericOverflow);
    }
#endif

    return Result<uint32_t>::success(static_cast<uint32_t>(value));
}

Result<size_t> convertUInt64ToSize(uint64_t value) noexcept
{
#ifndef ZSERIO_RUNTIME_64BIT
    if (value > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
    {
        return Result<size_t>::error(ErrorCode::NumericOverflow);
    }
#endif

    return Result<size_t>::success(static_cast<size_t>(value));
}

} // namespace zserio
