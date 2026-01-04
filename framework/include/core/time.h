/**
 * @file core/time.h
 * @brief Time source abstraction for framework timing.
 *
 * This header defines the function pointer type used by executors to query
 * the current monotonic time. The framework internally uses microseconds for
 * all timing calculations.
 */

#ifndef NIMBLE_CORE_TIME_H
#define NIMBLE_CORE_TIME_H

#include <cstdint>

namespace nimble {

/**
 * @brief Function pointer type for monotonic time source.
 *
 * The caller must provide a function that returns monotonic microseconds
 * since an arbitrary epoch. The time source must be non-decreasing to
 * ensure correct executor behavior. Example sources: hardware timer counter,
 * OS monotonic clock, or a test fake time counter.
 *
 * @return Current time in microseconds (monotonic).
 */
using TimeSourceFn = uint64_t (*)();


} // namespace nimble

#endif // NIMBLE_CORE_TIME_H
