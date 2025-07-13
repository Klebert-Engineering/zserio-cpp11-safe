#ifndef ZSERIO_BITFIELD_UTIL_H_INC
#define ZSERIO_BITFIELD_UTIL_H_INC

#include <cstddef>

#include "zserio/Result.h"
#include "zserio/Types.h"

namespace zserio
{

/**
 * Calculates lower bound for the given bit field.
 *
 * \param length Length of the bit field.
 * \param isSigned Whether the bit field is signed.
 *
 * \return Result containing lower bound for the bit field or error code on failure.
 */
Result<int64_t> getBitFieldLowerBound(size_t length, bool isSigned) noexcept;

/**
 * Calculates upper bound for the given bit field.
 *
 * \param length Length of the bit field.
 * \param isSigned Whether the bit field is signed.
 *
 * \return Result containing upper bound for the bit field or error code on failure.
 */
Result<uint64_t> getBitFieldUpperBound(size_t length, bool isSigned) noexcept;

} // namespace zserio

#endif // ifndef ZSERIO_BITFIELD_UTIL_H_INC
