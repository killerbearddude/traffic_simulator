# Traffic Simulator

TS-001 is a C++20 foundation for one vehicle moving at constant speed along one straight lane. The native SDL3 application and the headless executable use the same dependency-free simulation library.

## Documentation

Start with [Project state](docs/PROJECT_STATE.md) for the current boundary, [Architecture](docs/ARCHITECTURE.md) for the simulation contract, and [Testing](docs/TESTING.md) for verification requirements and command status. Codex work follows [AGENTS.md](AGENTS.md) and the reusable [handoff and completion-report templates](docs/CODEX_WORKFLOW.md).

## Prerequisites

- CMake 3.25 or newer, Ninja, Git, and a C++20 GCC or Clang toolchain.
- Network access on first configuration to fetch the exact dependency commits listed in `cmake/Dependencies.cmake`. Later builds use the downloaded source in `build/`.
- For the native app, a working SDL3-supported display and the platform development libraries required by SDL3. The headless preset needs no display or SDL3 development libraries.

The previously recorded validation environment was Ubuntu 24.04, GCC 13.3.0, CMake 3.28.3, and Ninja 1.11.1. Dependency versions are SDL3 3.2.22, Dear ImGui 1.91.9b, and Catch2 3.8.1, pinned to full commits in `cmake/Dependencies.cmake`. Third-party notices are in `third_party/NOTICES.md`.

## Build and run

These quickstart commands are also maintained in [Testing](docs/TESTING.md), which distinguishes source inspection from executed validation.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
./build/dev/traffic_app
./build/dev/traffic_app --force-fallback
./build/dev/traffic_headless

cmake --preset headless
cmake --build --preset headless
ctest --preset headless --output-on-failure
./build/headless/traffic_headless
```

`TRAFFIC_BUILD_APP` defaults to `ON`; set it to `OFF` to avoid SDL3 and ImGui discovery, fetching, building, and linking. `BUILD_TESTING=OFF` avoids Catch2. Both presets keep all build output under ignored `build/` directories.

The app starts paused. Run/Pause, Reset, and 1x/2x/4x control playback. Reset returns to the baseline at tick zero, paused at 1x. The baseline completes at tick 200 and 10.00 s, then stops. The headless runner prints the fixture and completion record and exits nonzero if the bounded run fails.

The app requests SDL renderer VSync and reports the selected pacing mode once at startup. If the request fails, it uses a nominal 60 FPS timed fallback. `--force-fallback` explicitly disables VSync and selects the timed path for validation. Fallback waits only for the frame budget remaining after input, simulation, rendering, and presentation; it does not change simulation steps or replace measured elapsed time. Actual frame intervals depend on the display and OS scheduler.

## Limits

This milestone has one lane and one vehicle. It does not include an intersection, other traffic, acceleration, routing, or editable scenarios. Native controls and rendering require a usable display for visual validation. The exact simulation contract and timing behavior are in `docs/ARCHITECTURE.md`.
