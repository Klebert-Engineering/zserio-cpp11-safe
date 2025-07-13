#include "zserio/BitFieldUtil.h"

namespace zserio
{

static Result<void> checkBitFieldLength(size_t length) noexcept
{
    if (length == 0 || length > 64)
    {
        return Result<void>::error(ErrorCode::InvalidParameter);
    }
    return Result<void>::success();
}

Result<int64_t> getBitFieldLowerBound(size_t length, bool isSigned) noexcept
{
    auto checkResult = checkBitFieldLength(length);
    if (checkResult.isError())
    {
        return Result<int64_t>::error(checkResult.getError());
    }

    if (isSigned)
    {
        return Result<int64_t>::success(-static_cast<int64_t>((UINT64_C(1) << (length - 1)) - 1) - 1);
    }
    else
    {
        return Result<int64_t>::success(0);
    }
}

Result<uint64_t> getBitFieldUpperBound(size_t length, bool isSigned) noexcept
{
    auto checkResult = checkBitFieldLength(length);
    if (checkResult.isError())
    {
        return Result<uint64_t>::error(checkResult.getError());
    }

    if (isSigned)
    {
        return Result<uint64_t>::success((UINT64_C(1) << (length - 1)) - 1);
    }
    else
    {
        return Result<uint64_t>::success(length == 64 ? UINT64_MAX : ((UINT64_C(1) << length) - 1));
    }
}

} // namespace zserio
