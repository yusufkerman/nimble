// framework/include/core/time.h
// Time abstraction for the framework.
// Responsibility: provide a typedef for the platform-provided monotonic time source.

#ifndef DFW_CORE_TIME_H
#define DFW_CORE_TIME_H

#include <cstdint>

namespace dfw {

// Monotonic time source returning microseconds since an epoch (implementation-defined).
// The scheduler requires a monotonic, non-decreasing time source provided by the caller.
using TimeSourceFn = uint64_t (*)();

} // namespace dfw

#endif // DFW_CORE_TIME_H
