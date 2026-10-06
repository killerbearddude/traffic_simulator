# Traffic Simulator

Traffic Simulator is a C++20 single-lane demonstration. M2 is the native and headless
default: six vehicles use IDM to approach a persistent stop line, each completes its
own stop, and a one-way Release queue command lets them depart. The native app has
Run/Pause, Reset, 1x/2x/4x playback, and fixed overview (0–600 m) and enlarged
stop-line (340–440 m) views. The original TS-001 constant-speed model remains available
in the headless runner with `--scenario ts-001`.

## Documentation

[Project state](docs/PROJECT_STATE.md) records implementation and review status.
[Architecture](docs/ARCHITECTURE.md) preserves the TS-001 timing contract and links
[M2 following and stopping](docs/systems/FOLLOWING_AND_STOPPING.md).
[Testing](docs/TESTING.md) gives the verification gate.
[AGENTS.md](AGENTS.md) and [Codex workflow](docs/CODEX_WORKFLOW.md) define the work process.

## Build and run

Requires CMake 3.25+, Ninja, Git, and a C++20 compiler. First configuration fetches
pinned dependencies from `cmake/Dependencies.cmake`. A supported display and SDL3 platform
development libraries are needed for the native app.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
./build/dev/traffic_app
./build/dev/traffic_app --force-fallback
./build/dev/traffic_headless
./build/dev/traffic_headless --scenario m2
./build/dev/traffic_headless --scenario ts-001

cmake --preset headless
cmake --build --preset headless
ctest --preset headless --output-on-failure
./build/headless/traffic_headless
./build/headless/traffic_headless --scenario m2
./build/headless/traffic_headless --scenario ts-001
```

The headless M2 run releases at tick 1200 after checking the held queue and allows
completion through tick 3600. Native release is manual and has no time limit. The app
starts paused and held at 1x. Reset restores the original fixture and clears records.
The renderer requests VSync and reports whether it uses VSync or timed fallback;
`--force-fallback` disables VSync for validation.

## Limits

M2 has one fixed lane and six fixed vehicles. It has no intersection, routing, spawning,
lane changes, editable scenario files, persistence, or analytics export. The model is a
development demonstration rather than calibrated traffic-engineering guidance.
