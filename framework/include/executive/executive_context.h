// framework/include/executive/executive_context.h
// Execution context struct holding all runtime state for the cyclic executive.
// Responsibility: own opaque runtime pointers and configuration; no behavior implemented here.
// WHY: Moving state into a context enables instance-based executors and removes header statics.

#ifndef DFW_EXECUTIVE_CONTEXT_H
#define DFW_EXECUTIVE_CONTEXT_H

#include <cstddef>
#include <cstdint>

#include "core/device.h"
#include "core/health.h"
#include "core/time.h"
#include "schedule/schedule_defs.h"
#include "policy/overrun_policy.h"

namespace dfw {

struct ExecContext {
    // Immutable configuration pointers (provided by caller).
    const Device* devices = nullptr;
    size_t device_count = 0;

    const ScheduleDef* schedules = nullptr;
    size_t schedule_count = 0;

    // Runtime buffers (caller-provided storage)
    Health* health = nullptr;           // len == device_count
    DeviceState* states = nullptr;      // len == device_count

    // Time source for runtime measurement (caller-provided)
    TimeSourceFn time_source = nullptr;

    // Policy choices
    OverrunPolicy overrun_policy = OverrunPolicy::DropTask;

    // Active schedule/minor indices and timing
    size_t active_schedule = 0;
    size_t pending_schedule = SIZE_MAX; // SIZE_MAX means none pending
    size_t current_minor = 0;
    uint64_t minor_start_time_us = 0; // start timestamp of the active minor
    uint64_t minor_used_us = 0;       // deterministic accounting for the active minor
    uint64_t schedule_major_us = 0;   // total major-frame duration for active schedule
};

} // namespace dfw

#endif // DFW_EXECUTIVE_CONTEXT_H
