/**
 * @file schedule/schedule_defs.h
 * @brief Static schedule table definitions for cyclic executive.
 *
 * This header provides POD structures that describe offline-computed schedules.
 * A schedule consists of minor frames, each containing a deterministic ordered
 * list of device indices to execute within a fixed time budget. These structures
 * are designed to be statically allocated and immutable at runtime.
 */

#ifndef NIMBLE_SCHEDULE_DEFS_H
#define NIMBLE_SCHEDULE_DEFS_H

#include <cstdint>
#include <cstddef>

#include "core/device.h"

namespace nimble {

/**
 * @struct MinorFrameDef
 * @brief Definition of a minor frame within a cyclic schedule.
 *
 * A minor frame specifies which devices to execute in what order, along with
 * the time budget allocated to this frame. The executor processes devices in
 * the order specified by `device_indices` and enforces the `duration_us` budget
 * using declared WCET values.
 */
struct MinorFrameDef {
    const uint16_t* device_indices = nullptr; /**< Array of device table indices. */
    size_t device_count = 0;                  /**< Number of devices in this minor. */
    uint64_t duration_us = 0;                 /**< Minor frame duration/budget in µs. */
};

/**
 * @struct ScheduleDef
 * @brief Definition of a complete major-frame schedule.
 *
 * A schedule is a sequence of minor frames that repeats cyclically. The executor
 * wraps back to the first minor after completing the last one. Multiple schedules
 * can be defined to support mode switching.
 */
struct ScheduleDef {
    const MinorFrameDef* minors = nullptr; /**< Array of minor frame definitions. */
    size_t minor_count = 0;                /**< Number of minors in this schedule. */
};


} // namespace nimble

#endif // NIMBLE_SCHEDULE_DEFS_H
