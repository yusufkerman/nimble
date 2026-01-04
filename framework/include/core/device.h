// framework/include/core/device.h
// Device abstraction: function pointers and lifecycle state.
// Responsibility: declare `Device` and related function-pointer types.
// NOTE: No scheduling or execution logic belongs here.

#ifndef DFW_CORE_DEVICE_H
#define DFW_CORE_DEVICE_H

#include <cstdint>
#include <cstddef>

namespace dfw {

struct Device; // forward

using DeviceInitFn = void (*)(Device*);
using DeviceUpdateFn = void (*)(Device*);
using DeviceLifecycleFn = void (*)(Device*);
using DeviceFaultFn = void (*)(Device*, int reason);

// Device runtime state. Kept minimal and free of scheduling details.
enum class DeviceState : uint8_t {
    Uninitialized = 0,
    Initialized,
    Running,
    Suspended,
    Faulted,
};

struct Device {
    DeviceInitFn init = nullptr;
    DeviceUpdateFn update = nullptr;
    // Nominal period in milliseconds (informational — scheduler decides actual releases)
    uint32_t period_ms = 0;
    // Declared WCET in microseconds used for deterministic accounting in the scheduler.
    uint32_t wcet_us = 0;

    // Optional lifecycle hooks called by the framework when transitioning state.
    DeviceLifecycleFn start = nullptr;
    DeviceLifecycleFn stop = nullptr;
    DeviceFaultFn on_fault = nullptr;
};

} // namespace dfw

#endif // DFW_CORE_DEVICE_H
