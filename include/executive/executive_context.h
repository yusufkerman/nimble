/**
 * @file executive/executive_context.h
 * @brief Runtime context for the cyclic executive.
 *
 * This header defines the `ExecContext` structure which holds all mutable
 * state needed by the executor at runtime. By keeping all state in a single
 * context struct, the design supports multiple executor instances and avoids
 * hidden global state.
 */

#ifndef NIMBLE_EXECUTIVE_CONTEXT_H
#define NIMBLE_EXECUTIVE_CONTEXT_H

#include <cstddef>
#include <cstdint>

#include "core/device.h"
#include "core/health.h"
#include "core/time.h"
#include "schedule/schedule_defs.h"
#include "policy/overrun_policy.h"

namespace nimble {

/**
 * @struct ExecContext
 * @brief Runtime state for a cyclic executive instance.
 *
 * This structure holds all configuration, buffers, and timing state needed
 * by the executor. The caller must provide and own all pointed-to memory
 * (devices, schedules, health, states) for the lifetime of the context.
 * No dynamic allocation occurs within the executor.
 */
struct ExecContext {
    const Device* devices = nullptr;    /**< Device table (caller-owned). */
    size_t device_count = 0;            /**< Number of devices in table. */

    const ScheduleDef* schedules = nullptr; /**< Schedule table (caller-owned). */
    size_t schedule_count = 0;              /**< Number of available schedules. */

    Health* health = nullptr;           /**< Health buffer (length == device_count). */
    DeviceState* states = nullptr;      /**< State buffer (length == device_count). */

    TimeSourceFn time_source = nullptr; /**< Monotonic time source function. */

    OverrunPolicy overrun_policy = OverrunPolicy::DropTask; /**< Overrun handling policy. */

    size_t active_schedule = 0;         /**< Index of currently active schedule. */
    size_t pending_schedule = SIZE_MAX; /**< Pending schedule (SIZE_MAX = none). */
    size_t current_minor = 0;           /**< Index of current minor within active schedule. */
    uint64_t minor_start_time_us = 0;   /**< Start timestamp of current minor (µs). */
    uint64_t minor_used_us = 0;         /**< Deterministic budget used in current minor (µs). */
    uint64_t schedule_major_us = 0;     /**< Total duration of active schedule's major frame (µs). */
};


} // namespace nimble

#endif // NIMBLE_EXECUTIVE_CONTEXT_H
