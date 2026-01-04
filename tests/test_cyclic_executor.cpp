// tests/test_cyclic_executor.cpp
// Minimal unit tests for the cyclic executor using a tiny, self-contained
// assertion harness to avoid external dependencies. Tests are C++17.

#include <cstdio>
#include <cstdint>
#include <cstring>

#include "framework/include/core/device.h"
#include "framework/include/core/time.h"
#include "framework/include/core/health.h"
#include "framework/include/schedule/schedule_defs.h"
#include "framework/include/executive/executive_context.h"
#include "framework/include/executive/cyclic_executor.h"
#include "framework/include/policy/overrun_policy.h"

using namespace nimble;

static int tests_run = 0;
static int tests_failed = 0;

#define ASSERT_EQ(expected, actual) do { \
    tests_run++; \
    if ((expected) != (actual)) { \
        std::printf("ASSERT_EQ failed: %s:%d: expected=%lld actual=%lld\n", __FILE__, __LINE__, (long long)(expected), (long long)(actual)); \
        tests_failed++; \
        return; \
    } \
} while(0)

// Test helpers
struct Counters { int inits; int updates; };

static uint64_t g_time_us = 0;
static uint64_t fake_time_source() { return g_time_us; }

// Test 1: initialization calls device init() and sets up context
static void test_init_calls_device_init_and_sets_state() {
    Counters a{0,0}, b{0,0};
    auto a_init = [](Device* d){ (void)d; /*noop*/ };
    // Use functions that increment counters via capture by pointer is tricky in C++
    // so we use simple functions that modify static counters below.
}

// We'll create concrete test cases implementing behaviors below

static void test_init_and_basic_execution() {
    // device counters
    static Counters A{0,0};
    static Counters B{0,0};

    // lambdas cannot be converted to function pointers; provide static functions
    struct D0 { static void init(Device*) { A.inits++; } static void update(Device*) { A.updates++; } };
    struct D1 { static void init(Device*) { B.inits++; } static void update(Device*) { B.updates++; } };

    static Device devices[] = {
        { &D0::init, &D0::update, 1000, 200, nullptr, nullptr, nullptr },
        { &D1::init, &D1::update, 1000, 300, nullptr, nullptr, nullptr }
    };

    static const uint16_t minor0[] = { 0 };
    static const uint16_t minor1[] = { 1 };
    static const MinorFrameDef minors[] = { { minor0, 1, 100000 }, { minor1, 1, 100000 } };
    static const ScheduleDef schedules[] = { { minors, 2 } };

    static Health health[2];
    static DeviceState states[2];
    ExecContext ctx{};

    g_time_us = 0;
    bool ok = cyclic_init(&ctx, devices, 2, schedules, 1, 0, health, states, fake_time_source, OverrunPolicy::DropTask);
    ASSERT_EQ(true, ok);
    // init should have been called for both devices
    ASSERT_EQ(1, A.inits);
    ASSERT_EQ(1, B.inits);

    // advance time to trigger first minor
    g_time_us += 100000; // minor 0 deadline
    cyclic_tick_us(&ctx, g_time_us);
    // device 0 update executed
    ASSERT_EQ(1, A.updates);
    ASSERT_EQ(0, B.updates);

    // advance to next minor
    g_time_us += 100000;
    cyclic_tick_us(&ctx, g_time_us);
    ASSERT_EQ(1, B.updates);
}

// Test overrun behavior: DropTask should skip a task that would exceed minor budget
static void test_overrun_drop_task_skips_task() {
    static Counters A{0,0};
    static Counters B{0,0};
    struct D0 { static void init(Device*) { A.inits++; } static void update(Device*) { A.updates++; } };
    struct D1 { static void init(Device*) { B.inits++; } static void update(Device*) { B.updates++; } };

    // Set wcet of D0 large so it alone would exceed minor budget (e.g., declared 200000 > minor 100000)
    static Device devices[] = {
        { &D0::init, &D0::update, 1000, 200000, nullptr, nullptr, nullptr },
        { &D1::init, &D1::update, 1000, 100, nullptr, nullptr, nullptr }
    };

    static const uint16_t minor0[] = { 0, 1 };
    static const MinorFrameDef minors[] = { { minor0, 2, 100000 } };
    static const ScheduleDef schedules[] = { { minors, 1 } };

    static Health health[2];
    static DeviceState states[2];
    ExecContext ctx{};

    g_time_us = 0;
    bool ok = cyclic_init(&ctx, devices, 2, schedules, 1, 0, health, states, fake_time_source, OverrunPolicy::DropTask);
    ASSERT_EQ(true, ok);

    // advance to trigger minor
    g_time_us += 100000;
    cyclic_tick_us(&ctx, g_time_us);

    // D0 should be skipped due to declared WCET > budget; D1 should execute
    ASSERT_EQ(0, A.updates);
    ASSERT_EQ(1, B.updates);
}

int main() {
    // Run tests
    test_init_and_basic_execution();
    test_overrun_drop_task_skips_task();

    std::printf("tests run=%d failed=%d\n", tests_run, tests_failed);
    return (tests_failed == 0) ? 0 : 2;
}
