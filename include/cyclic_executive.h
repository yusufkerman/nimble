// Deprecated monolithic header.
// This header remained from a previous monolithic design and is kept only
// to preserve backwards-compatible include paths. It is deprecated —
// include the modular headers instead:
//   - include/core/device.h
//   - include/executive/cyclic_executor.h
//   - include/schedule/schedule_defs.h
// This file now forwards to the new cyclic executor public header.

#ifndef NIMBLE_CYCLIC_EXECUTIVE_DEPRECATED_H
#define NIMBLE_CYCLIC_EXECUTIVE_DEPRECATED_H

// Forward to the canonical cyclic executor header.
#include "executive/cyclic_executor.h"

#endif // NIMBLE_CYCLIC_EXECUTIVE_DEPRECATED_H

    // Overrun actions in cyclic mode (minor-frame exceeding budget)
    enum class OverrunPolicy { DropTask, SkipFrame, SignalFault, ResetSystem };

    using FaultHandlerFn = void(*)(std::size_t minor_index,
                                   std::size_t device_index,
                                   uint32_t used_us,
                                   uint32_t minor_budget_us);
    using ResetHookFn = void(*)(void);

    // ------------ Legacy periodic mode (caller provides next_release buffer) ------------
    // Initializes legacy periodic (next-release) mode. All time is kept
    // internally in microseconds to avoid truncation. The caller must
    // provide a preallocated `next_release_buffer` with length >= count.
    static bool init(Device* devices, std::size_t count, uint32_t major_frame_ms,
                     uint64_t* next_release_buffer,
                     CatchUpPolicy policy = CatchUpPolicy::Limited,
                     uint32_t max_catchups = 1) {
        if (!devices || !next_release_buffer) return false;
        devices_ = devices;
        count_ = count;
        // informational only
        major_frame_ms_ = major_frame_ms;

        // prepare legacy state
        next_release_ = next_release_buffer;
        policy_ = policy;
        max_catchups_ = max_catchups;
        current_time_us_ = 0;
        for (std::size_t i = 0; i < count_; ++i) {
            if (devices_[i].init) devices_[i].init(&devices_[i]);
            next_release_[i] = (devices_[i].period_ms == 0) ? kNever
                                : static_cast<uint64_t>(devices_[i].period_ms) * 1000ull;
        }
        legacy_mode_ = true;
        cyclic_mode_ = false;
        return true;
    }

    // Reset both modes' time accounting to zero; next_release buffer will
    // be reloaded for legacy mode.
    static void reset() {
        current_time_us_ = 0;
        if (next_release_) {
            for (std::size_t i = 0; i < count_; ++i) {
                next_release_[i] = (devices_[i].period_ms == 0) ? kNever
                                    : static_cast<uint64_t>(devices_[i].period_ms) * 1000ull;
            }
        }
        minor_elapsed_us_ = 0;
        current_minor_ = 0;
        skip_next_minor_ = false;
    }

    // ------------ Cyclic (major/minor) mode ------------
    struct MinorFrameDef {
        uint32_t duration_us;           // minor duration (us)
        const uint16_t* device_indices; // indices into devices[]
        std::size_t device_count;
    };

    // Initialize cyclic scheduling with static minor frame table.
    // Returns false on invalid indices or schedulability failure.
    // Health / state structures (caller-provided buffers)
    struct Health {
        uint64_t execution_count;
        uint64_t overrun_count;
        uint64_t last_exec_time_us;
        uint64_t max_exec_time_us;
    };

    enum class DeviceState : uint8_t { Uninitialized = 0, Initialized, Running, Suspended, Faulted };

    enum class FaultReason : uint8_t { WCET_VIOLATION = 0, FRAME_OVERRUN, MANUAL };

    // Initialize cyclic scheduling with static minor frame table.
    // The caller must provide:
    //  - `health_buffer` : array of Health sized `device_count`
    //  - `state_buffer`  : array of DeviceState sized `device_count`
    // Both buffers must remain valid for the lifetime of the executive.
    // Returns false on invalid indices or schedulability failure.
    using DeviceFaultHandlerFn = void(*)(std::size_t device_index, FaultReason reason);
    static bool init_cyclic(Device* devices, std::size_t device_count,
                            const MinorFrameDef* minors, std::size_t minor_count,
                            Health* health_buffer, DeviceState* state_buffer,
                            OverrunPolicy overrun_policy = OverrunPolicy::DropTask,
                            FaultHandlerFn frame_fault_handler = nullptr,
                            DeviceFaultHandlerFn device_fault_handler = nullptr,
                            ResetHookFn reset_hook = nullptr,
                            TimeSourceFn time_source = nullptr,
                            uint32_t overrun_threshold = 1) {
        if (!devices || !minors || minor_count == 0) return false;
        if (!health_buffer || !state_buffer) return false;
        devices_ = devices;
        count_ = device_count;
        minors_ = minors;
        minor_count_ = minor_count;
        overrun_policy_ = overrun_policy;
        frame_fault_handler_ = frame_fault_handler;
        device_fault_handler_ = device_fault_handler;
        reset_hook_ = reset_hook;
        time_source_ = time_source;
        health_ = health_buffer;
        states_ = state_buffer;
        overrun_threshold_ = overrun_threshold;

        // static schedulability check: sum WCET per minor must fit its budget
        major_frame_us_ = 0;
        for (std::size_t m = 0; m < minor_count_; ++m) {
            major_frame_us_ += static_cast<uint64_t>(minors_[m].duration_us);
            uint64_t sum_wcet = 0;
            for (std::size_t j = 0; j < minors_[m].device_count; ++j) {
                uint16_t di = minors_[m].device_indices[j];
                if (di >= count_) return false; // bad table
                sum_wcet += devices_[di].wcet_us; // wcet in us
                if (sum_wcet > minors_[m].duration_us) return false; // not schedulable
            }
        }

        // init cyclic state
        cyclic_mode_ = true;
        legacy_mode_ = false;
        current_minor_ = 0;
        minor_elapsed_us_ = 0;
        last_time_us_ = time_source_ ? time_source_() : 0;
        skip_next_minor_ = false;

        // Initialize health and state buffers deterministically
        for (std::size_t i = 0; i < count_; ++i) {
            health_[i].execution_count = 0;
            health_[i].overrun_count = 0;
            health_[i].last_exec_time_us = 0;
            health_[i].max_exec_time_us = 0;
            states_[i] = DeviceState::Initialized;
        }
        return true;
    }
    }

    // ------------ Tick helpers ------------
    // tick accepts milliseconds (common) and forwards to microsecond version
    static void tick(uint32_t delta_ms) {
        if (cyclic_mode_) {
            tick_us(static_cast<uint64_t>(delta_ms) * 1000ull);
            return;
        }
        // legacy periodic mode
        if (!legacy_mode_ || !devices_ || count_ == 0 || !next_release_) return;
        uint64_t delta_us = static_cast<uint64_t>(delta_ms) * 1000ull;
        current_time_us_ += delta_us;

        for (std::size_t i = 0; i < count_; ++i) {
            Device& d = devices_[i];
            if (!d.update || d.period_ms == 0) continue;
            if (current_time_us_ < next_release_[i]) continue;

            uint64_t period_us = static_cast<uint64_t>(d.period_ms) * 1000ull;
            uint64_t diff = current_time_us_ - next_release_[i];
            uint64_t total_due = diff / period_us + 1;

            // execute according to policy
            if (policy_ == CatchUpPolicy::CatchAll) {
                for (uint64_t k = 0; k < total_due; ++k) d.update(&d);
            } else if (policy_ == CatchUpPolicy::Limited) {
                uint64_t calls = total_due;
                if (calls > max_catchups_) calls = max_catchups_;
                for (uint64_t k = 0; k < calls; ++k) d.update(&d);
            }
            // SkipMisses => don't call missed activations

            // advance next_release_ with saturation
            advance_next_release_saturating(next_release_[i], period_us, total_due);
        }
    }

    // tick_us: microsecond-resolution tick used by cyclic-mode or time-source
    static void tick_us(uint64_t delta_us) {
        if (!cyclic_mode_ || !minors_) return;
        minor_elapsed_us_ += delta_us;

        // while we've completed current minor frame, execute it deterministically
        while (minor_elapsed_us_ >= minors_[current_minor_].duration_us) {
            const MinorFrameDef& mf = minors_[current_minor_];
            uint64_t frame_budget = mf.duration_us;
            uint64_t used_us = 0;
            bool overrun = false;

            for (std::size_t idx = 0; idx < mf.device_count; ++idx) {
                std::size_t dev_idx = mf.device_indices[idx];
                uint64_t wcet = devices_[dev_idx].wcet_us;

                // account the declared WCET deterministically
                used_us += wcet;
                if (used_us > frame_budget) { overrun = true; /* stop accounting */ break; }

                // execute the device's update (application work)
                if (devices_[dev_idx].update) devices_[dev_idx].update(&devices_[dev_idx]);
            }

            if (overrun) {
                // deterministic handling according to configured policy
                handle_overrun(current_minor_, used_us, static_cast<uint32_t>(frame_budget));
            }

            // advance minor frame timeline
            minor_elapsed_us_ -= mf.duration_us;
            current_minor_ = (current_minor_ + 1) % minor_count_;

            if (skip_next_minor_) {
                // skip execution of the next minor frame (deterministic)
                skip_next_minor_ = false;
                minor_elapsed_us_ = 0;
                current_minor_ = (current_minor_ + 1) % minor_count_;
            }
        }
    }

    // poll_from_time_source: call when a TimeSourceFn was provided in init_cyclic
    static void poll_from_time_source() {
        if (!time_source_ || !cyclic_mode_) return;
        uint64_t now = time_source_();
        uint64_t delta = now - last_time_us_;
        last_time_us_ = now;
        tick_us(delta);
    }

private:
    // small helper: saturating advance of next_release by (period_us * times)
    static inline void advance_next_release_saturating(uint64_t &next, uint64_t period_us, uint64_t times) {
        if (next == kNever) return;
        if (period_us == 0) { next = kNever; return; }
        if (times == 0) return;
        if (times > UINT64_MAX / period_us) { next = kNever; return; }
        uint64_t incr = times * period_us;
        if (next > UINT64_MAX - incr) { next = kNever; return; }
        next += incr;
    }

    // overrun handling helper
    static inline void handle_overrun(std::size_t minor_idx, uint32_t used_us, uint32_t budget_us) {
        switch (overrun_policy_) {
            case OverrunPolicy::DropTask:
                // Do nothing further: remaining tasks in this minor are not executed
                break;
            case OverrunPolicy::SkipFrame:
                skip_next_minor_ = true;
                break;
            case OverrunPolicy::SignalFault:
                if (fault_handler_) fault_handler_(minor_idx, /*device_index*/ 0, used_us, budget_us);
                break;
            case OverrunPolicy::ResetSystem:
                if (reset_hook_) reset_hook_();
                break;
        }
    }

    // ---- static internal state (single-instance) ----
    static inline Device* devices_ = nullptr;
    static inline std::size_t count_ = 0;

    // legacy periodic state
    static inline bool legacy_mode_ = false;
    static inline uint64_t* next_release_ = nullptr; // caller-provided
    static inline CatchUpPolicy policy_ = CatchUpPolicy::Limited;
    static inline uint32_t max_catchups_ = 1;
    static inline uint64_t current_time_us_ = 0;

    // cyclic mode state
    static inline const MinorFrameDef* minors_ = nullptr;
    static inline std::size_t minor_count_ = 0;
    static inline bool cyclic_mode_ = false;
    static inline std::size_t current_minor_ = 0;
    static inline uint64_t minor_elapsed_us_ = 0;
    static inline uint64_t major_frame_us_ = 0; // informational
    static inline OverrunPolicy overrun_policy_ = OverrunPolicy::DropTask;
    static inline FaultHandlerFn fault_handler_ = nullptr;
    static inline ResetHookFn reset_hook_ = nullptr;
    static inline TimeSourceFn time_source_ = nullptr;
    static inline uint64_t last_time_us_ = 0;
    static inline bool skip_next_minor_ = false;
    // informational ms value
    static inline uint32_t major_frame_ms_ = 0;
};

} // namespace nimble
// Cyclic executive manager: supports both legacy periodic next-release
// mode and a true offline major/minor cyclic executive suitable for
// safety-critical embedded systems.
//
// Design constraints followed strictly:
// - No dynamic memory allocation
// - Freestanding-friendly (no exceptions, minimal headers)
// - Deterministic execution order
// - Static / caller-provided tables

#pragma once

#include <cstddef>
#include <cstdint>
#include "device.h"

namespace nimble {

// Special value meaning "never schedule".
static constexpr uint64_t kNever = UINT64_MAX;

class CyclicExecutive {
public:
    // Time source abstraction: returns monotonic time in microseconds.
    using TimeSourceFn = uint64_t(*)(void);

    // Catch-up policy for legacy periodic mode.
    enum class CatchUpPolicy { CatchAll, Limited, SkipMisses };

    // Overrun handling policies for cyclic-mode minor frames.
    enum class OverrunPolicy { DropTask, SkipFrame, SignalFault, ResetSystem };

    using FaultHandlerFn = void(*)(std::size_t minor_index, std::size_t device_index, uint32_t used_us, uint32_t minor_budget_us);
    using ResetHookFn = void(*)(void);

    // --- Legacy periodic next-release mode init (no heap) ---
    static bool init(Device* devices, std::size_t count, uint32_t major_frame_ms,
                     uint64_t* next_release_buffer,
                     CatchUpPolicy policy = CatchUpPolicy::Limited,
                     uint32_t max_catchups = 1) {
        if (!next_release_buffer) return false;
        devices_ = devices;
        count_ = count;
        major_frame_ms_ = major_frame_ms;
        current_time_ms_ = 0;
        policy_ = policy;
        max_catchups_ = max_catchups;

        next_release_ = next_release_buffer;
        for (std::size_t i = 0; i < count_; ++i) {
            Device& d = devices_[i];
            // Call device init if provided (framework-responsible init)
            if (d.init) d.init(&d);
            if (d.period_ms == 0) next_release_[i] = kNever;
            else next_release_[i] = static_cast<uint64_t>(d.period_ms) * 1000ull; // store in us
        }
        legacy_mode_ = true;
        cyclic_mode_ = false;
        return true;
    }

    // Reset legacy periodic state
    static void reset() {
        current_time_ms_ = 0;
        if (next_release_) {
            for (std::size_t i = 0; i < count_; ++i) {
                if (devices_[i].period_ms == 0) next_release_[i] = kNever;
                else next_release_[i] = static_cast<uint64_t>(devices_[i].period_ms) * 1000ull;
            }
        }
        // reset cyclic state too
        minor_elapsed_us_ = 0;
        current_minor_ = 0;
        skip_next_minor_ = false;
    }

    // --- Cyclic executive (offline major/minor table) ---
    struct MinorFrameDef {
        uint32_t duration_us; // minor frame duration in microseconds
        const uint16_t* device_indices; // indices into devices[]
        std::size_t device_count;
    };

    // Initialize cyclic-mode with caller-provided minor-frame table. All
    // buffers must be statically allocated by the caller. This function
    // performs static schedulability checks (sum WCETs <= minor duration).
    static bool init_cyclic(Device* devices, std::size_t device_count,
                            const MinorFrameDef* minors, std::size_t minor_count,
                            OverrunPolicy overrun_policy = OverrunPolicy::DropTask,
                            FaultHandlerFn fault_handler = nullptr,
                            ResetHookFn reset_hook = nullptr,
                            TimeSourceFn time_source = nullptr) {
        if (!devices || !minors || minor_count == 0) return false;
        // assign
        devices_ = devices;
        count_ = device_count;
        minors_ = minors;
        minor_count_ = minor_count;
        overrun_policy_ = overrun_policy;
        fault_handler_ = fault_handler;
        reset_hook_ = reset_hook;
        time_source_ = time_source;

        // compute major frame and validate per-minor WCET sums (static check)
        major_frame_us_ = 0;
        for (std::size_t m = 0; m < minor_count_; ++m) {
            major_frame_us_ += static_cast<uint64_t>(minors_[m].duration_us);
            uint64_t sum_wcet = 0;
            for (std::size_t j = 0; j < minors_[m].device_count; ++j) {
                uint16_t di = minors_[m].device_indices[j];
                if (di >= count_) return false; // invalid index
                sum_wcet += devices_[di].wcet_us;
                if (sum_wcet > minors_[m].duration_us) return false; // schedulability fail
            }
        }

        // initialize cyclic state
        cyclic_mode_ = true;
        legacy_mode_ = false;
        current_minor_ = 0;
        minor_elapsed_us_ = 0;
        last_time_us_ = time_source_ ? time_source_() : 0;
        skip_next_minor_ = false;
        return true;
    }

    // Advance the executive by `delta_ms` (milliseconds). This will
    // dispatch either legacy periodic mode or cyclic-mode processing.
    static void tick(uint32_t delta_ms) {
        if (cyclic_mode_) {
            tick_us(static_cast<uint64_t>(delta_ms) * 1000ull);
            return;
        }

        if (!legacy_mode_) return;
        if (!devices_ || count_ == 0 || !next_release_) return;

        // convert delta to microseconds for internal consistency
        uint64_t delta_us = static_cast<uint64_t>(delta_ms) * 1000ull;
        current_time_us_ += delta_us;

        for (std::size_t i = 0; i < count_; ++i) {
            Device& d = devices_[i];
            if (!d.update) continue;
            if (d.period_ms == 0) continue;

            if (current_time_us_ < next_release_[i]) continue;

            uint64_t diff = current_time_us_ - next_release_[i];
            uint64_t total_due = diff / (static_cast<uint64_t>(d.period_ms) * 1000ull) + 1;

            switch (policy_) {
                case CatchUpPolicy::CatchAll: {
                    for (uint64_t k = 0; k < total_due; ++k) d.update(&d);
                    // saturating add
                    if (d.period_ms == 0) break;
                    uint64_t mul = static_cast<uint64_t>(d.period_ms) * 1000ull;
                    if (total_due > UINT64_MAX / mul) next_release_[i] = kNever;
                    else {
                        uint64_t incr = total_due * mul;
                        if (next_release_[i] > UINT64_MAX - incr) next_release_[i] = kNever;
                        else next_release_[i] += incr;
                    }
                    break;
                }
                case CatchUpPolicy::Limited: {
                    uint64_t calls = total_due;
                    if (calls > max_catchups_) calls = max_catchups_;
                    for (uint64_t k = 0; k < calls; ++k) d.update(&d);
                    if (d.period_ms == 0) break;
                    uint64_t mul = static_cast<uint64_t>(d.period_ms) * 1000ull;
                    if (total_due > UINT64_MAX / mul) next_release_[i] = kNever;
                    else {
                        uint64_t incr = total_due * mul;
                        if (next_release_[i] > UINT64_MAX - incr) next_release_[i] = kNever;
                        else next_release_[i] += incr;
                    }
                    break;
                }
                case CatchUpPolicy::SkipMisses: {
                    if (d.period_ms == 0) break;
                    uint64_t mul = static_cast<uint64_t>(d.period_ms) * 1000ull;
                    if (total_due > UINT64_MAX / mul) next_release_[i] = kNever;
                    else {
                        uint64_t incr = total_due * mul;
                        if (next_release_[i] > UINT64_MAX - incr) next_release_[i] = kNever;
                        else next_release_[i] += incr;
                    }
                    break;
                }
            }
        }
    }

    // tick in microseconds - used for cyclic mode
    static void tick_us(uint64_t delta_us) {
        if (!cyclic_mode_ || !minors_) return;

        minor_elapsed_us_ += delta_us;

        // process expired minor frames in order
        while (minor_elapsed_us_ >= minors_[current_minor_].duration_us) {
            uint32_t frame_budget = minors_[current_minor_].duration_us;
            uint32_t used_us = 0;

            const uint16_t* devs = minors_[current_minor_].device_indices;
            for (std::size_t di = 0; di < minors_[current_minor_].device_count; ++di) {
                std::size_t dev_idx = devs[di];
                uint32_t w = devices_[dev_idx].wcet_us;
                // account WCET deterministically
                used_us += w;

                if (used_us > frame_budget) {
                    // deterministic overrun handling
                    switch (overrun_policy_) {
                        case OverrunPolicy::DropTask:
                            goto advance_minor;
                        case OverrunPolicy::SkipFrame:
                            skip_next_minor_ = true;
                            goto advance_minor;
                        case OverrunPolicy::SignalFault:
                            if (fault_handler_) fault_handler_(current_minor_, dev_idx, used_us, frame_budget);
                            goto advance_minor;
                        case OverrunPolicy::ResetSystem:
                            if (reset_hook_) reset_hook_();
                            goto advance_minor;
                    }
                }

                // execute device update (application work)
                if (devices_[dev_idx].update) devices_[dev_idx].update(&devices_[dev_idx]);
            }

        advance_minor:
            // consume minor frame time and advance
            minor_elapsed_us_ -= frame_budget;
            current_minor_ = (current_minor_ + 1) % minor_count_;
            if (skip_next_minor_) {
                // deterministically skip next minor frame execution
                skip_next_minor_ = false;
                // skip its execution and reset elapsed
                minor_elapsed_us_ = 0;
                current_minor_ = (current_minor_ + 1) % minor_count_;
            }
        }
    }

    // Time-source driven poll: caller can call this regularly in main loop
    // when a TimeSourceFn was provided during cyclic init. It advances
    // timeline using the time source.
    static void poll_from_time_source() {
        if (!time_source_ || !cyclic_mode_) return;
        uint64_t now = time_source_();
        uint64_t delta = now - last_time_us_;
        last_time_us_ = now;
        tick_us(delta);
    }

private:
    // NOTE: this implementation uses global static state (single instance).
    // For RTOS or multi-executive tests you may prefer an instance-based
    // `CyclicExecutiveContext` that holds these fields; that allows multiple
    // executives to coexist and simplifies unit testing. We keep the
    // single-instance design for simplicity, but it is a documented
    // design constraint.
    static inline Device* devices_ = nullptr;
    static inline std::size_t count_ = 0;
    // Legacy mode state
    static inline bool legacy_mode_ = false;
    static inline uint64_t* next_release_ = nullptr;
    static inline CatchUpPolicy policy_ = CatchUpPolicy::Limited;
    static inline uint32_t max_catchups_ = 1;
    static inline uint64_t current_time_us_ = 0;

    // Cyclic-mode state
    static inline const MinorFrameDef* minors_ = nullptr;
    static inline std::size_t minor_count_ = 0;
    static inline bool cyclic_mode_ = false;
    static inline std::size_t current_minor_ = 0;
    static inline uint64_t minor_elapsed_us_ = 0;
    static inline uint64_t major_frame_us_ = 0;
    static inline OverrunPolicy overrun_policy_ = OverrunPolicy::DropTask;
    static inline FaultHandlerFn fault_handler_ = nullptr;
    static inline ResetHookFn reset_hook_ = nullptr;
    static inline TimeSourceFn time_source_ = nullptr;
    static inline uint64_t last_time_us_ = 0;
    static inline bool skip_next_minor_ = false;
    // informational
    static inline uint32_t major_frame_ms_ = 0;
};

} // namespace nimble
// Cyclic executive manager: holds a static table of `Device` and ticks them
// according to their `period_ms` inside a manually-provided major frame.

#pragma once

#include <cstddef>
#include <cstdint>
#include "device.h"

namespace nimble {

// Special value meaning "never schedule". Use UINT64_MAX to avoid
// <limits> dependency and keep this header friendly to freestanding/toolchain-restricted builds.
static constexpr uint64_t kNever = UINT64_MAX;

// Improved CyclicExecutive: uses per-device next-release times (absolute
// timeline) so late ticks or variable delta_ms do not cause missed
// activations. Major-frame / hyperperiod concepts are noted but not
// automatically computed here (TODO: LCM/hyperperiod + minor-frame packing).

class CyclicExecutive {
public:
    // Catch-up policy for handling missed releases when `tick` is delayed.
    enum class CatchUpPolicy {
        // Call update for every missed activation (may starve others).
        CatchAll,
        // Call at most `max_catchups` updates per tick for a device, then
        // advance its next-release to the first future release.
        Limited,
        // Do not call missed updates; advance next-release to the first
        // release after the current time (misses are skipped).
        SkipMisses
    };

    // Provide the device table (statically allocated by caller), count and
    // a major frame duration in milliseconds (informational). The caller
    // MUST provide a `next_release_buffer` with length >= `count` to avoid
    // heap allocations. Returns true on success. The `policy` controls how
    // missed activations are handled; `max_catchups` applies when policy is Limited.
    static bool init(Device* devices, std::size_t count, uint32_t major_frame_ms,
                     uint64_t* next_release_buffer,
                     CatchUpPolicy policy = CatchUpPolicy::Limited,
                     uint32_t max_catchups = 1) {
        if (!next_release_buffer) return false;
        devices_ = devices;
        count_ = count;
        major_frame_ms_ = major_frame_ms;
        current_time_ms_ = 0;
        policy_ = policy;
        max_catchups_ = max_catchups;

        next_release_ = next_release_buffer;
        for (std::size_t i = 0; i < count_; ++i) {
            Device& d = devices_[i];
            // Call device init if provided (framework-responsible init)
            if (d.init) d.init(&d);
            if (d.period_ms == 0) next_release_[i] = kNever;
            else next_release_[i] = static_cast<uint64_t>(d.period_ms); // first release at period
        }
        return true;
    }

    // Reset timeline and reload next-release times into the provided buffer.
    static void reset() {
        current_time_ms_ = 0;
        if (next_release_) {
            for (std::size_t i = 0; i < count_; ++i) {
                if (devices_[i].period_ms == 0) next_release_[i] = kNever;
                else next_release_[i] = static_cast<uint64_t>(devices_[i].period_ms);
            }
        }
    }

    // Advance the executive by `delta_ms` on an absolute timeline. For each
    // device we decrease its time-to-next-release and invoke `update` once
    // per deadline that has passed (multiple firings allowed).
    static void tick(uint32_t delta_ms) {
        if (!devices_ || count_ == 0 || !next_release_) return;

        current_time_ms_ += delta_ms;

        for (std::size_t i = 0; i < count_; ++i) {
            Device& d = devices_[i];
            if (!d.update) continue;
            if (d.period_ms == 0) continue;

            // Determine how many activations are due
            if (current_time_ms_ < next_release_[i]) continue;

            uint64_t diff = current_time_ms_ - next_release_[i];
            uint64_t total_due = diff / d.period_ms + 1; // at least one

            switch (policy_) {
                case CatchUpPolicy::CatchAll: {
                    for (uint64_t k = 0; k < total_due; ++k) d.update(&d);
                    // saturating add: avoid multiplication overflow
                    if (d.period_ms == 0) break;
                    uint64_t max_mul = UINT64_MAX / static_cast<uint64_t>(d.period_ms);
                    if (total_due > max_mul) {
                        next_release_[i] = kNever; // saturate
                    } else {
                        uint64_t incr = total_due * static_cast<uint64_t>(d.period_ms);
                        if (next_release_[i] > UINT64_MAX - incr) next_release_[i] = kNever;
                        else next_release_[i] += incr;
                    }
                    break;
                }
                case CatchUpPolicy::Limited: {
                    uint64_t calls = total_due;
                    if (calls > max_catchups_) calls = max_catchups_;
                    for (uint64_t k = 0; k < calls; ++k) d.update(&d);
                    // advance to the first release after current_time (skip remaining misses)
                    if (d.period_ms == 0) break;
                    uint64_t max_mul2 = UINT64_MAX / static_cast<uint64_t>(d.period_ms);
                    if (total_due > max_mul2) {
                        next_release_[i] = kNever;
                    } else {
                        uint64_t incr = total_due * static_cast<uint64_t>(d.period_ms);
                        if (next_release_[i] > UINT64_MAX - incr) next_release_[i] = kNever;
                        else next_release_[i] += incr;
                    }
                    break;
                }
                case CatchUpPolicy::SkipMisses: {
                    // do not call missed updates; schedule next after current time
                    if (d.period_ms == 0) break;
                    uint64_t max_mul3 = UINT64_MAX / static_cast<uint64_t>(d.period_ms);
                    if (total_due > max_mul3) {
                        next_release_[i] = kNever;
                    } else {
                        uint64_t incr = total_due * static_cast<uint64_t>(d.period_ms);
                        if (next_release_[i] > UINT64_MAX - incr) next_release_[i] = kNever;
                        else next_release_[i] += incr;
                    }
                    break;
                }
            }
        }
    }

private:
    // NOTE: this implementation uses global static state (single instance).
    // For RTOS or multi-executive tests you may prefer an instance-based
    // `CyclicExecutiveContext` that holds these fields; that allows multiple
    // executives to coexist and simplifies unit testing. We keep the
    // single-instance design for simplicity, but it is a documented
    // design constraint.
    static inline Device* devices_ = nullptr;
    static inline std::size_t count_ = 0;
    // Optional informative major frame (not used for wrapping here).
    static inline uint32_t major_frame_ms_ = 0;
    // Monotonic absolute time since init() in milliseconds
    static inline uint64_t current_time_ms_ = 0;
    // Per-device next absolute release time (ms). nullptr until init()
    static inline uint64_t* next_release_ = nullptr;
    static inline CatchUpPolicy policy_ = CatchUpPolicy::Limited;
    static inline uint32_t max_catchups_ = 1;
};

} // namespace nimble
