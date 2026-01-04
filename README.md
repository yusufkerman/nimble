# 🚀 Nimble

A small, **deterministic C++17 framework** for building predictable, safety-oriented embedded applications. Nimble provides a **time-triggered cyclic execution model** to run simple device components with explicit budgets and well-defined lifecycle hooks.

Perfect for **bare-metal systems, microcontrollers, and freestanding environments** where dynamic allocation, exceptions, and RTTI are undesirable.

---

## ✨ Key Features

- ⏱️ **Deterministic Cyclic Executive** — Major/minor frame scheduling with predictable timing
- 🔧 **POD Device Descriptors** — Simple structs with lifecycle hooks (init/start/stop/update/on_fault)
- 📋 **Compile-Time Schedules** — Static schedule definitions with per-device WCET annotations
- 🛡️ **Budget Enforcement** — Per-device execution time tracking and overrun policies
- 🔄 **Multiple Schedules** — Safe schedule switching at frame boundaries
- 📦 **Freestanding-Friendly** — No dynamic memory, no exceptions, no RTTI
- ⚡ **Minimal Footprint** — Lightweight runtime, suitable for resource-constrained targets
- 🎯 **Auditable Design** — Clear separation of concerns, easy to reason about

---

## 📚 Documentation

📖 **Full API Documentation:**  
👉 **[https://yusufkerman.github.io/nimble/](https://yusufkerman.github.io/nimble/)**

The documentation is generated with **Doxygen** and published via **GitHub Pages**.

---

## 🚀 Quick Start

### 1. Include Headers
```cpp
#include "framework/include/core/device.h"
#include "framework/include/schedule/schedule_defs.h"
#include "framework/include/executive/cyclic_executor.h"
```

### 2. Define Your Devices
```cpp
// Device callbacks
void sensor_init(nimble::Device* d) { /* setup */ }
void sensor_update(nimble::Device* d) { /* read sensor */ }

// Device descriptor
static nimble::Device devices[] = {
  { sensor_init, nullptr, sensor_update, nullptr, 1000, 500 }
};
```

### 3. Create a Schedule
```cpp
static const uint16_t minor0_devs[] = { 0 };
static const nimble::MinorFrameDef minors[] = {
  { minor0_devs, 1, 100000 }  // 100ms frame
};
static const nimble::ScheduleDef schedules[] = { { minors, 1 } };
```

### 4. Initialize & Run
```cpp
static nimble::Health health[1];
static nimble::DeviceState states[1];
static nimble::ExecContext ctx;

nimble::cyclic_init(&ctx, devices, 1, schedules, 1, 0,
                   health, states, my_time_source,
                   nimble::OverrunPolicy::DropTask);

// Main loop
while (true) {
  nimble::cyclic_poll(&ctx);
}
```

See the **[`examples/`](examples/)** directory for more examples.

---

## 🏗️ Architecture

| Layer | Purpose |
|-------|---------|
| **Core** | `Device`, `Health`, `Time` abstractions |
| **Schedule** | Minor/major frame definitions |
| **Policy** | Overrun & catch-up strategy enums |
| **Executive** | Runtime driver & cyclic executor |

---

## 🔨 Build & Integration

1. **Include** headers from `framework/include/`
2. **Link** compiled objects from `framework/src/`
3. **Recommended flags:** `-std=c++17 -O2 -fno-exceptions -fno-rtti`

---

## 📖 Examples

Check out the **[`examples/`](examples/)** directory for practical usage patterns:
- Basic cyclic scheduling
- Multi-frame coordination
- Health monitoring
- Fault handling

---

## 📝 License

This project is licensed under the **MIT License**. See [LICENSE](LICENSE) for details.

---

## 🤝 Contributing

Contributions, bug reports, and feature requests are welcome! Please open an issue or pull request.

---

**Built by [Yusuf Kerman](https://github.com/yusufkerman)**
