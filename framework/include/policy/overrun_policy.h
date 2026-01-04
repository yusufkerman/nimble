// framework/include/policy/overrun_policy.h
// Overrun policy vocabulary and deterministic decision enums.
// Responsibility: declare how an executor should interpret an overrun event.
// WHY: Executors call policy decisions; policy header contains only the decision space.

#ifndef DFW_POLICY_OVERRUN_H
#define DFW_POLICY_OVERRUN_H

#include <cstdint>

namespace dfw {

// High-level actions an executor may take when detecting an overrun or budget breach.
enum class OverrunPolicy : uint8_t {
    // Skip the offending task and continue with next tasks deterministically.
    DropTask = 0,
    // Stop executing remaining tasks in the current minor frame.
    SkipFrame,
    // Mark the device as faulted; executor/platform may run fault hooks.
    SignalFault,
    // Trigger a system reset hook (executor calls provided reset callback).
    ResetSystem,
};

} // namespace dfw

#endif // DFW_POLICY_OVERRUN_H
