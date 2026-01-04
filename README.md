# Nimble

Nimble is a small, deterministic C++17 framework for building predictable,
safety-oriented embedded applications. It provides a time-triggered cyclic
execution model to run simple device components with explicit budgets and
well-defined lifecycle hooks.

## Key Features
- Deterministic cyclic executive (major/minor frames)
- Lightweight POD `Device` descriptors (init/start/stop/update/on_fault)
- Compile-time schedule descriptions (`nimble::MinorFrameDef`, `nimble::ScheduleDef`)
- Per-device WCET annotations with budget enforcement
- No dynamic memory, no exceptions, no RTTI — freestanding-friendly

## Documentation
The generated API documentation is published via GitHub Pages at:

https://yusufkerman.github.io/nimble/

If you see a 404, GitHub Pages may still be propagating the site — wait a few
minutes and try again. If the problem persists, check the repository Pages
settings and ensure the `gh-pages` branch is selected as the site source.

## Quick Start
1. Include headers from the `framework/include/` directory.
2. Define your `Device` array and `Schedule` structures following the examples.
3. Initialize runtime with `nimble::cyclic_init(...)` and drive execution with
	`nimble::cyclic_poll(&ctx)` in your main loop.

See the `examples/` directory for concrete usage examples.

## GitHub Pages
The documentation is served from the `gh-pages` branch. The site URL is above.

## License
See the repository for license details.

Contributions and issues are welcome via GitHub.
