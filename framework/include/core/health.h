/**
 * @file core/health.h
 * @brief Health telemetry data structure for runtime monitoring.
 *
 * This header defines a simple POD structure to hold execution statistics
 * and timing measurements for devices. The structure is intentionally minimal
 * to minimize memory overhead in embedded systems.
 */

#ifndef NIMBLE_CORE_HEALTH_H
#define NIMBLE_CORE_HEALTH_H

#include <cstdint>

namespace nimble {

/**
 * @struct Health
 * @brief Runtime health and execution statistics for a device.
 *
 * Executors update this structure to track how many times a device has
 * executed, how many overruns occurred, and timing measurements.
 */
struct Health {
    uint32_t execution_count = 0;   /**< Number of times update() has been called. */
    uint32_t overrun_count = 0;     /**< Count of budget overruns or WCET violations. */
    uint64_t last_exec_time_us = 0; /**< Last measured execution time in microseconds. */
    uint64_t max_exec_time_us = 0;  /**< Maximum observed execution time in microseconds. */
};


} // namespace nimble

#endif // NIMBLE_CORE_HEALTH_H
