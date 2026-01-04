// framework/include/schedule/schedule_defs.h
// Static schedule descriptions: minor frames and schedules.
// Responsibility: provide plain POD definitions describing static schedule tables.
// WHY: Schedulers must operate on immutable, statically-declared tables for determinism.

#ifndef DFW_SCHEDULE_DEFS_H
#define DFW_SCHEDULE_DEFS_H

#include <cstdint>
#include <cstddef>

#include "core/device.h"

namespace dfw {

// A minor frame groups a deterministic, ordered list of device indices.
// The scheduler reads these indices in order and executes the referenced devices.
struct MinorFrameDef {
    // Pointer to an array of device indices (indices into the device table).
    const uint16_t* device_indices = nullptr;
    // Number of devices in this minor (determines iteration count).
    size_t device_count = 0;
    // Duration (budget) of this minor frame in microseconds.
    uint64_t duration_us = 0;
};

// A schedule is a collection of minor frames that form a major frame.
// Schedulers must not modify these structures at runtime.
struct ScheduleDef {
    const MinorFrameDef* minors = nullptr;
    size_t minor_count = 0;
};

} // namespace dfw

#endif // DFW_SCHEDULE_DEFS_H
