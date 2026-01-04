// framework/include/core/health.h
// Health telemetry data for devices. Plain POD data only.
// Responsibility: hold execution counters and runtime measurement fields.

#ifndef DFW_CORE_HEALTH_H
#define DFW_CORE_HEALTH_H

#include <cstdint>

namespace dfw {

struct Health {
    uint32_t execution_count = 0;   // how many times update() has run
    uint32_t overrun_count = 0;     // detected overruns or WCET violations
    uint64_t last_exec_time_us = 0; // last measured execution time in microseconds
    uint64_t max_exec_time_us = 0;  // maximum observed execution time
};

} // namespace dfw

#endif // DFW_CORE_HEALTH_H
