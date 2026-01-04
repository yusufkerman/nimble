/**
 * @file executive/cyclic_executor.h
 * @brief Public API for the cyclic executive scheduler.
 *
 * This header declares the initialization and tick functions for the cyclic
 * executive. The executor operates on an `ExecContext` provided by the caller
 * and processes minor frames deterministically based on a monotonic time source.
 */

#ifndef NIMBLE_CYCLIC_EXECUTOR_H
#define NIMBLE_CYCLIC_EXECUTOR_H

#include <cstddef>
#include <cstdint>

#include "executive/executive_context.h"
#include "schedule/schedule_defs.h"
#include "policy/catchup_policy.h"
#include "policy/overrun_policy.h"
#include "core/time.h"

namespace nimble {

/**
 * @brief Initialize a cyclic executive context.
 *
 * This function sets up the provided `ExecContext` for cyclic execution.
 * It stores pointers to caller-owned buffers and schedules, computes the
 * major-frame duration, initializes device states, and calls device init()
 * functions.
 *
 * @param ctx Pointer to caller-allocated ExecContext.
 * @param devices Device table (caller-owned, must remain valid).
 * @param device_count Number of devices in the table.
 * @param schedules Schedule table (caller-owned, must remain valid).
 * @param schedule_count Number of available schedules.
 * @param initial_schedule Index of the schedule to activate initially.
 * @param health_buffer Health buffer (length must be >= device_count).
 * @param state_buffer State buffer (length must be >= device_count).
 * @param time_source Monotonic time source function.
 * @param overrun_policy Policy for handling budget overruns.
 * @return true on success, false if arguments are invalid.
 */
bool cyclic_init(ExecContext* ctx,
                 const Device* devices,
                 size_t device_count,
                 const ScheduleDef* schedules,
                 size_t schedule_count,
                 size_t initial_schedule,
                 Health* health_buffer,
                 DeviceState* state_buffer,
                 TimeSourceFn time_source,
                 OverrunPolicy overrun_policy) noexcept;

/**
 * @brief Drive the executor with an explicit time value.
 *
 * This function checks if the current minor frame's deadline has passed.
 * If so, it executes the minor and advances to the next minor. Multiple
 * minors may be executed if time has advanced significantly.
 *
 * @param ctx Pointer to initialized ExecContext.
 * @param current_time_us Current monotonic time in microseconds.
 */
void cyclic_tick_us(ExecContext* ctx, uint64_t current_time_us) noexcept;

/**
 * @brief Drive the executor using the context's time source.
 *
 * This is a convenience wrapper that calls `cyclic_tick_us()` with the
 * current time obtained from `ctx->time_source()`.
 *
 * @param ctx Pointer to initialized ExecContext.
 */
void cyclic_poll(ExecContext* ctx) noexcept;

// Query helpers (trivial inline)
static inline size_t cyclic_active_schedule(const ExecContext* ctx) noexcept { return ctx->active_schedule; }
static inline size_t cyclic_current_minor(const ExecContext* ctx) noexcept { return ctx->current_minor; }
static inline uint64_t cyclic_current_minor_deadline_us(const ExecContext* ctx) noexcept {
    if (!ctx || !ctx->schedules) return 0;
    const ScheduleDef& s = ctx->schedules[ctx->active_schedule];
    if (ctx->current_minor >= s.minor_count) return 0;
    return ctx->minor_start_time_us + s.minors[ctx->current_minor].duration_us;
}


} // namespace nimble

#endif // NIMBLE_CYCLIC_EXECUTOR_H
