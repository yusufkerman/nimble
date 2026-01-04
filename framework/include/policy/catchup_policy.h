// framework/include/policy/catchup_policy.h
// Catch-up policy definitions for legacy periodic execution.
// Responsibility: enumerate deterministic catch-up strategies (policy only).
// WHY: Keep decision vocabulary separate from executor implementation.

#ifndef DFW_POLICY_CATCHUP_H
#define DFW_POLICY_CATCHUP_H

#include <cstdint>

namespace dfw {

// How to handle missed activations when the system falls behind.
enum class CatchUpPolicy : uint8_t {
    // Execute every missed activation until current time (may cause bursts)
    CatchAll = 0,
    // Execute up to a bounded number of missed activations (caller-defined in executor)
    Limited,
    // Skip missed activations and wait for next scheduled release
    SkipMisses,
};

} // namespace dfw

#endif // DFW_POLICY_CATCHUP_H
