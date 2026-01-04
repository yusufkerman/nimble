/**
 * @file core/device.h
 * @brief Device abstraction and lifecycle state types.
 *
 * This header provides the core `Device` structure and related function pointer
 * types used throughout the framework. The Device structure is intentionally minimal
 * and contains only function pointers and declarative fields to keep it suitable
 * for static allocation in bare-metal embedded systems.
 */

#ifndef NIMBLE_CORE_DEVICE_H
#define NIMBLE_CORE_DEVICE_H

#include <cstdint>
#include <cstddef>

namespace nimble {

/** Forward declaration for function pointer types. */
struct Device;

/** @brief Function pointer type for device initialization. */
using DeviceInitFn = void (*)(Device*);

/** @brief Function pointer type for periodic device update/work. */
using DeviceUpdateFn = void (*)(Device*);

/** @brief Function pointer type for lifecycle transitions (start/stop). */
using DeviceLifecycleFn = void (*)(Device*);

/** @brief Function pointer type for fault notifications with reason code. */
using DeviceFaultFn = void (*)(Device*, int reason);

/**
 * @enum DeviceState
 * @brief Runtime state of a device tracked by the executor.
 *
 * The executor uses this state to determine whether a device should be
 * scheduled for execution. States are intentionally minimal to keep
 * storage requirements low.
 */
enum class DeviceState : uint8_t {
    Uninitialized = 0, /**< Device has not been initialized. */
    Initialized,       /**< Device init() has been called. */
    Running,           /**< Device is actively being scheduled. */
    Suspended,         /**< Device is temporarily suspended. */
    Faulted,           /**< Device has entered a fault state. */
};

/**
 * @struct Device
 * @brief Compact device descriptor consumed by framework executors.
 *
 * This structure contains only function pointers and declarative metadata.
 * All fields are plain POD types to ensure compatibility with freestanding
 * and bare-metal environments. Executors rely on `wcet_us` for deterministic
 * budget accounting.
 */
struct Device {
    DeviceInitFn init = nullptr;       /**< Optional initialization function called once. */
    DeviceUpdateFn update = nullptr;   /**< Periodic work function called by scheduler. */
    
    uint32_t period_ms = 0;            /**< Nominal period in ms (informational only). */
    uint32_t wcet_us = 0;              /**< Declared worst-case execution time in µs. */

    DeviceLifecycleFn start = nullptr; /**< Optional callback on transition to Running. */
    DeviceLifecycleFn stop = nullptr;  /**< Optional callback on transition to Suspended. */
    DeviceFaultFn on_fault = nullptr;  /**< Optional callback on fault with reason code. */
};

} // namespace nimble

#endif // NIMBLE_CORE_DEVICE_H
