#ifndef ZSERIO_SIZE_CONVERT_UTIL_H_INC
#define ZSERIO_SIZE_CONVERT_UTIL_H_INC

#include <cstddef>

#include "zserio/Result.h"
#include "zserio/Types.h"

namespace zserio
{

/**
 * Converts size (array size, string size or bit buffer size) of type size_t to uint32_t value.
 *
 * \param value Size of type size_t to convert.
 *
 * \return Result containing uint32_t value converted from size or error code on failure.
 */
Result<uint32_t> convertSizeToUInt32(size_t value) noexcept;

/**
 * Converts uint64_t value to size (array size, string size of bit buffer size).
 *
 * \param value uint64_t value to convert.
 *
 * \return Result containing size_t value converted from uint64_t value or error code on failure.
 */
Result<size_t> convertUInt64ToSize(uint64_t value) noexcept;

} // namespace zserio

#endif // ZSERIO_SIZE_CONVERT_UTIL_H_INC
