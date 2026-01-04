// examples/basic_cyclic_example.cpp
// Minimal, bare-metal-friendly example showing how to use the cyclic executive.
// - Static devices
// - Static schedule (minor frames)
// - Static ExecContext
// - Fake time source driving the executor
//
// This file intentionally avoids C++ iostreams and dynamic allocation so it
// can serve as documentation-by-example for embedded use.

#include <cstdio>
#include <cstdint>

#include "framework/include/device.h"                 // compatibility wrapper
#include "framework/include/schedule/schedule_defs.h"
#include "framework/include/executive/executive_context.h"
#include "framework/include/executive/cyclic_executor.h"
#include "framework/include/core/time.h"
#include "framework/include/core/health.h"
#include "framework/include/policy/overrun_policy.h"

using namespace dfw;

// Fake monotonic time source (microseconds)
static uint64_t g_time_us = 0;
static uint64_t fake_time_source() { return g_time_us; }

// Simple device-side state used in the example to show init/update were called.
struct DevState { uint32_t inits; uint32_t updates; };

static DevState dev0_state = {0,0};
static DevState dev1_state = {0,0};

// Device callbacks (no allocations)
static void dev0_init(Device* d) { (void)d; dev0_state.inits++; }
static void dev0_update(Device* d) { (void)d; dev0_state.updates++; }

static void dev1_init(Device* d) { (void)d; dev1_state.inits++; }
static void dev1_update(Device* d) { (void)d; dev1_state.updates++; }

// Static device table (caller-owned, remains valid for lifetime)
static Device devices[] = {
    // init, update, period_ms, wcet_us, start, stop, on_fault
    { dev0_init, dev0_update, 1000, 1000, nullptr, nullptr, nullptr },
    { dev1_init, dev1_update, 1000, 500,  nullptr, nullptr, nullptr }
};

// Minor frames: each minor references device indices in deterministic order.
static const uint16_t minor0_devs[] = { 0, 1 }; // execute dev0 then dev1
static const uint16_t minor1_devs[] = { 1 };    // execute dev1 only

static const MinorFrameDef minors[] = {
    // device_indices, device_count, duration_us
    { minor0_devs, 2, 100000 }, // 100 ms
    { minor1_devs, 1, 200000 }  // 200 ms
};

static const ScheduleDef schedules[] = {
    { minors, 2 }
};

// Statically-allocated runtime buffers (caller-provided)
static Health health[sizeof(devices)/sizeof(devices[0])];
static DeviceState states[sizeof(devices)/sizeof(devices[0])];
static ExecContext ctx; // context owned by caller

int main_example(void) {
    // Initialize the cyclic executor with our static context and tables.
    bool ok = cyclic_init(&ctx,
                          devices,
                          sizeof(devices)/sizeof(devices[0]),
                          schedules,
                          1,
                          0, // initial schedule index
                          health,
                          states,
                          fake_time_source,
                          OverrunPolicy::DropTask);
    if (!ok) {
        // In embedded use, handle init failure appropriately (here we just return)
        return -1;
    }

    // Drive the executor by advancing fake time. At each tick we may execute
    // zero or one minor frames depending on time progression. The example
    // advances time in 100 ms steps and polls the executor.

    for (int step = 0; step < 6; ++step) {
        // Advance time by 100 ms
        g_time_us += 100000;
        // Poll the executor; this will run the minor frame(s) whose deadline passed
        cyclic_poll(&ctx);

        // Example visibility: print counters using printf (non-iostream)
        // On a bare-metal system remove these prints and observe device-side state.
        std::printf("time=%llu us: dev0 inits=%u updates=%u; dev1 inits=%u updates=%u\n",
                    (unsigned long long)g_time_us,
                    (unsigned)dev0_state.inits, (unsigned)dev0_state.updates,
                    (unsigned)dev1_state.inits, (unsigned)dev1_state.updates);
    }

    return 0;
}
