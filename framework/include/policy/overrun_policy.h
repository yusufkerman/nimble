/**
 * @file policy/overrun_policy.h
 * @brief Overrun handling policy enumeration.
 *
 * This header defines the deterministic policy choices for handling budget
 * overruns in the cyclic executive. When a device's declared WCET would exceed
 * the remaining minor frame budget, the executor consults this policy to decide
 * what action to take.
 */

#ifndef NIMBLE_POLICY_OVERRUN_H
#define NIMBLE_POLICY_OVERRUN_H

#include <cstdint>

namespace nimble {

/**
 * @enum OverrunPolicy
 * @brief Policy for handling minor-frame budget overruns.
 *
 * The executor applies this policy when a device's declared WCET would cause
 * the cumulative minor-frame budget to be exceeded. The policy choice determines
 * whether the offending task is skipped, the frame is aborted, or a fault is raised.
 */
enum class OverrunPolicy : uint8_t {
    DropTask = 0,   /**< Skip the offending task; continue with remaining tasks. */
    SkipFrame,      /**< Stop executing remaining tasks in this minor frame. */
    SignalFault,    /**< Mark the device as faulted and invoke fault hooks. */
    ResetSystem,    /**< Trigger system reset hook (platform-defined). */
};


} // namespace nimble

#endif // NIMBLE_POLICY_OVERRUN_H
