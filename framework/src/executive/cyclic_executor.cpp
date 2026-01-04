// framework/src/executive/cyclic_executor.cpp
// Implementation of the core cyclic executive (init + tick).
// Responsibility: minor-frame driven deterministic execution and overrun handling.
// Notes:
// - No dynamic allocation, no globals; all state is in ExecContext.
// - Time units are microseconds.

#include "executive/cyclic_executor.h"

#include <cstdint>
#include <cstddef>

namespace nimble {

namespace {
// Saturating add to avoid overflow; kNever semantics not required here but keep safe.
static inline uint64_t sat_add_u64(uint64_t a, uint64_t b) noexcept {
    if (a > UINT64_MAX - b) return UINT64_MAX;
    return a + b;
}
} // anonymous

bool cyclic_init(ExecContext* ctx,
                 const Device* devices,
                 size_t device_count,
                 const ScheduleDef* schedules,
                 size_t schedule_count,
                 size_t initial_schedule,
                 Health* health_buffer,
                 DeviceState* state_buffer,
                 TimeSourceFn time_source,
                 OverrunPolicy overrun_policy) noexcept {
    if (!ctx || !devices || device_count == 0 || !schedules || schedule_count == 0 || !state_buffer || !time_source) return false;
    if (initial_schedule >= schedule_count) return false;

    // Copy configuration into context (caller owns the pointed-to buffers)
    ctx->devices = devices;
    ctx->device_count = device_count;
    ctx->schedules = schedules;
    ctx->schedule_count = schedule_count;
    ctx->active_schedule = initial_schedule;
    ctx->pending_schedule = SIZE_MAX;
    ctx->health = health_buffer; // optional: executor does not modify health here
    ctx->states = state_buffer;
    ctx->time_source = time_source;
    ctx->overrun_policy = overrun_policy;

    // Compute major-frame duration for the active schedule
    const ScheduleDef& s = ctx->schedules[ctx->active_schedule];
    uint64_t major = 0;
    for (size_t i = 0; i < s.minor_count; ++i) {
        major = sat_add_u64(major, s.minors[i].duration_us);
    }
    ctx->schedule_major_us = major;

    // Initialize minor timing to now and set minor index to zero
    ctx->minor_start_time_us = ctx->time_source();
    ctx->current_minor = 0;
    ctx->minor_used_us = 0;

    // Initialize device states: call device init if present and mark Running
    for (size_t i = 0; i < ctx->device_count; ++i) {
        if (ctx->devices[i].init) ctx->devices[i].init(const_cast<Device*>(&ctx->devices[i]));
        ctx->states[i] = DeviceState::Running;
    }

    return true;
}

static void execute_minor(ExecContext* ctx) noexcept {
    const ScheduleDef& s = ctx->schedules[ctx->active_schedule];
    if (ctx->current_minor >= s.minor_count) return;
    const MinorFrameDef& minor = s.minors[ctx->current_minor];

    uint64_t budget = minor.duration_us;
    ctx->minor_used_us = 0;

    for (size_t i = 0; i < minor.device_count; ++i) {
        size_t dev_idx = minor.device_indices[i];
        if (dev_idx >= ctx->device_count) continue;

        // Only execute devices in Running state
        if (ctx->states[dev_idx] != DeviceState::Running) continue;

        uint64_t declared = ctx->devices[dev_idx].wcet_us;
        uint64_t would = sat_add_u64(ctx->minor_used_us, declared);
        if (would > budget) {
            // budget exceeded — apply overrun policy deterministically
            switch (ctx->overrun_policy) {
                case OverrunPolicy::DropTask:
                    // skip this task, continue with next
                    continue;
                case OverrunPolicy::SkipFrame:
                    // stop executing further tasks in this minor
                    ctx->minor_used_us = budget;
                    return;
                case OverrunPolicy::SignalFault:
                case OverrunPolicy::ResetSystem:
                default:
                    // For policies that require system/fault hooks (not in scope here),
                    // conservatively stop executing remaining tasks in this minor.
                    ctx->minor_used_us = budget;
                    return;
            }
        }

        // Execute the device update (executor does not measure runtime here)
        if (ctx->devices[dev_idx].update) ctx->devices[dev_idx].update(const_cast<Device*>(&ctx->devices[dev_idx]));

        // Deterministic accounting uses declared WCET (not measured)
        ctx->minor_used_us = would;
        if (ctx->minor_used_us >= budget) return;
    }
}

static void advance_minor(ExecContext* ctx) noexcept {
    const ScheduleDef& s = ctx->schedules[ctx->active_schedule];
    ctx->current_minor++;
    if (ctx->current_minor >= s.minor_count) {
        ctx->current_minor = 0;
        // schedule switching at major-frame boundaries could be applied here
        // (pending_schedule is part of ExecContext) — not implemented in this step.
    }
}

void cyclic_tick_us(ExecContext* ctx, uint64_t current_time_us) noexcept {
    if (!ctx || !ctx->schedules) return;

    const ScheduleDef& s = ctx->schedules[ctx->active_schedule];
    if (s.minor_count == 0) return;

    // If current time hasn't reached the current minor end, do nothing.
    uint64_t minor_end = sat_add_u64(ctx->minor_start_time_us, s.minors[ctx->current_minor].duration_us);
    if (current_time_us < minor_end) return;

    // Execute the minor whose deadline has passed.
    execute_minor(ctx);

    // Advance the minor start time deterministically by the minor duration we just executed.
    ctx->minor_start_time_us = sat_add_u64(ctx->minor_start_time_us, s.minors[ctx->current_minor].duration_us);

    // Move to next minor (wraps at major-frame boundary)
    advance_minor(ctx);
}

void cyclic_poll(ExecContext* ctx) noexcept {
    if (!ctx || !ctx->time_source) return;
    cyclic_tick_us(ctx, ctx->time_source());
}

} // namespace nimble
