#ifndef ZSERIO_PMR_REFLECTABLE_H_INC
#define ZSERIO_PMR_REFLECTABLE_H_INC

#include "zserio/unsafe/Reflectable.h"
#include "zserio/unsafe/pmr/PolymorphicAllocator.h"

namespace zserio
{
namespace pmr
{

/**
 * Typedef to the reflectable factroy provided for convenience - using PropagatingPolymorphicAllocator<uint8_t>.
 */
using ReflectableFactory = BasicReflectableFactory<PropagatingPolymorphicAllocator<uint8_t>>;

} // namespace pmr
} // namespace zserio

#endif // ZSERIO_PMR_REFLECTABLE_H_INC
