// framework/include/executive/cyclic_executor.h
// Public API for the cyclic executive. Header contains declarations only.
// Responsibility: declare init and tick entry points operating on an ExecContext.

#ifndef DFW_CYCLIC_EXECUTOR_H
#define DFW_CYCLIC_EXECUTOR_H

#include <cstddef>
#include <cstdint>

#include "executive/executive_context.h"
#include "schedule/schedule_defs.h"
#include "policy/catchup_policy.h"
#include "policy/overrun_policy.h"
#include "core/time.h"

namespace dfw {

// Initialize the provided ExecContext for cyclic execution.
// - `ctx` must point to caller-provided ExecContext storage.
// - `devices`, `schedules`, `health_buffer`, `state_buffer` must remain valid
//    for the lifetime of the context (caller-owned storage).
// - `initial_schedule` picks the active schedule index.
// - `time_source` is required for correct timing behavior.
// Returns true on success, false for invalid arguments.
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

// Drive the executor using an absolute monotonic time (microseconds).
// This call may execute one or more minor frames if `current_time_us` has advanced.
void cyclic_tick_us(ExecContext* ctx, uint64_t current_time_us) noexcept;

// Convenience: call tick using the context's TimeSourceFn.
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

} // namespace dfw

#endif // DFW_CYCLIC_EXECUTOR_H
