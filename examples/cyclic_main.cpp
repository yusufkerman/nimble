// examples/cyclic_main.cpp
// Clean example showing how to use the modular cyclic executive API.
// - Uses only modular headers (core/, schedule/, executive/, policy/)
// - Bare-metal-friendly (uses printf for visibility)

#include <cstdint>
#include <cstdio>

#include "framework/include/core/device.h"
#include "framework/include/core/time.h"
#include "framework/include/core/health.h"
#include "framework/include/schedule/schedule_defs.h"
#include "framework/include/executive/executive_context.h"
#include "framework/include/executive/cyclic_executor.h"
#include "framework/include/policy/overrun_policy.h"

using namespace dfw;

// Fake monotonic time source (microseconds). In real embedded systems provide
// a hardware timer or OS monotonic clock.
static uint64_t g_time_us = 0;
static uint64_t fake_time_source() { return g_time_us; }

// Simple device-local counters to demonstrate init/update being called.
struct Counters { uint32_t inits; uint32_t updates; };
static Counters devA_c = {0,0};
static Counters devB_c = {0,0};

// Device callbacks (no dynamic allocation)
static void devA_init(Device* d) { (void)d; devA_c.inits++; }
static void devA_update(Device* d) { (void)d; devA_c.updates++; }

static void devB_init(Device* d) { (void)d; devB_c.inits++; }
static void devB_update(Device* d) { (void)d; devB_c.updates++; }

// Static device table (caller-owned)
static Device devices[] = {
    { devA_init, devA_update, 1000, 200, nullptr, nullptr, nullptr },
    { devB_init, devB_update, 1000, 300, nullptr, nullptr, nullptr }
};

// Minor frames (deterministic ordered device indices)
static const uint16_t minor0_devs[] = { 0 }; // minor 0: run device 0
static const uint16_t minor1_devs[] = { 1 }; // minor 1: run device 1

static const MinorFrameDef minors[] = {
    { minor0_devs, 1, 100000 }, // 100 ms
    { minor1_devs, 1, 100000 }  // 100 ms
};

static const ScheduleDef schedules[] = { { minors, 2 } };

// Caller-provided runtime buffers (statically allocated)
static Health health[sizeof(devices)/sizeof(devices[0])];
static DeviceState states[sizeof(devices)/sizeof(devices[0])];
static ExecContext ctx;

int main() {
    // Initialize the cyclic executor using our static context and tables.
    bool ok = cyclic_init(&ctx,
                          devices,
                          sizeof(devices)/sizeof(devices[0]),
                          schedules,
                          1, // schedule_count
                          0, // initial schedule index
                          health,
                          states,
                          fake_time_source,
                          OverrunPolicy::DropTask);
    if (!ok) return 1;

    // Drive the executor: advance time and poll. Each poll may execute completed
    // minor frames deterministically based on the fake time source.
    for (int i = 0; i < 6; ++i) {
        g_time_us += 100000; // advance by 100 ms
        cyclic_poll(&ctx);

        // Print counters to show where init/update ran.
        // On a constrained target replace printf with suitable telemetry.
        std::printf("t=%lluus: A inits=%u updates=%u | B inits=%u updates=%u\n",
                    (unsigned long long)g_time_us,
                    (unsigned)devA_c.inits, (unsigned)devA_c.updates,
                    (unsigned)devB_c.inits, (unsigned)devB_c.updates);
    }

    return 0;
}
