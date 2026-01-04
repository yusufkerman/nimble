/**
 * @file policy/catchup_policy.h
 * @brief Catch-up policy for periodic task scheduling.
 *
 * This header defines policy choices for handling missed periodic releases
 * when the system falls behind real-time. Used by legacy periodic executors
 * (not by the cyclic executive).
 */

#ifndef NIMBLE_POLICY_CATCHUP_H
#define NIMBLE_POLICY_CATCHUP_H

#include <cstdint>

namespace nimble {

/**
 * @enum CatchUpPolicy
 * @brief Policy for catching up on missed periodic activations.
 *
 * When a periodic task falls behind its next-release time, this policy
 * determines whether missed activations should be executed immediately,
 * bounded, or skipped entirely.
 */
enum class CatchUpPolicy : uint8_t {
    CatchAll = 0, /**< Execute all missed activations (may cause bursts). */
    Limited,      /**< Execute up to a bounded number of missed activations. */
    SkipMisses,   /**< Skip missed activations and wait for next release. */
};


} // namespace nimble

#endif // NIMBLE_POLICY_CATCHUP_H
