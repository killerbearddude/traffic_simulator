# Testing

## Command authority and verification status

`CMakeLists.txt`, `CMakePresets.json`, `cmake/Dependencies.cmake`, and the test sources define the build and test configuration. This document explains how to use them; resolve a discrepancy rather than blindly running stale prose.

TS-DOC-001 application baseline: local `main` at `5bd7eb7f67dad2e2c3a68a23690bfbc5857e15b1` on 2026-10-04 (America/Denver). On the local documentation review branch at the same commit, both presets configured and built, both CTest suites passed (19/19 each), and both headless executables produced the expected baseline. The earlier packet review had no local checkout and did not run these checks; its failed clone attempt is historical packet evidence, not this application result. Native display interaction remains unverified.

The README's earlier validation environment is historical evidence, not a result from this session. Future completion reports must name their own checkout, environment, exact commands, and outcomes.

## Environment requirements

Run commands from the repository root. The project declares CMake 3.25 or newer, Ninja presets, Git-based dependency fetching, and C++20. The current exact-arrival implementation requires IEEE 754 binary64 with subnormals as documented in [Architecture](ARCHITECTURE.md).

The `dev` preset builds the native app and tests. It needs the platform development libraries required by SDL3; a usable native display is additionally required to verify rendering and interaction. The `headless` preset turns the app off, so SDL3 and Dear ImGui are not fetched or linked by that preset. **It still has `BUILD_TESTING=ON` and fetches Catch2.** Do not mistake headless operation for an entirely offline first test configuration.

Dependency revisions are pinned in `cmake/Dependencies.cmake`. Preserve those pins unless the task authorizes a dependency change. A configuration with both `TRAFFIC_BUILD_APP=OFF` and `BUILD_TESTING=OFF` avoids these third-party fetches, but it does not satisfy the test gate.

## Standard commands

### Native development build

Status for TS-DOC-001: both commands passed on 2026-10-04.

```sh
cmake --preset dev
cmake --build --preset dev
```

### Headless build

Status for TS-DOC-001: both commands passed on 2026-10-04.

```sh
cmake --preset headless
cmake --build --preset headless
```

### Targeted regression selection

Status for TS-DOC-001: patterns were checked against test names, but these targeted commands were not run. Configure and build the chosen preset first. Inspect the selected tests and confirm the filter is not empty.

```sh
ctest --preset headless -N
ctest --preset headless --output-on-failure -R 'arrival|constant-speed|off-grid'
ctest --preset headless --output-on-failure -R 'pacing|frame budget|VSync'
ctest --preset headless --output-on-failure -R '^headless_baseline$'
```

Change a filter when the authorized task affects a different area; these examples are not substitutes for the full gate.

### Full regression gate and headless runs

Status for TS-DOC-001: all four commands passed on 2026-10-04. Both CTest runs passed 19/19 cases; both headless runs reported tick 200, 10.00 s, 100.00 m, zero active vehicles, and one completed vehicle.

```sh
ctest --preset dev --output-on-failure
./build/dev/traffic_headless
ctest --preset headless --output-on-failure
./build/headless/traffic_headless
```

The expected baseline is completion tick 200, time 10.00 seconds, distance 100.00 meters, zero active vehicles, and one completed vehicle. It was also observed in both TS-DOC-001 headless runs.

### Native visual and interaction checks

Status for TS-DOC-001: NOT CHECKED; this environment had no `DISPLAY` or `WAYLAND_DISPLAY`. A real supported display is required.

```sh
./build/dev/traffic_app
./build/dev/traffic_app --force-fallback
```

Check initial pause, Run/Pause without paused-time catch-up, Reset to tick zero/paused/1x, all playback choices, completion stopping the simulation, and the completion display. Resize the window and verify that presentation changes do not alter simulation state. Check controls and closing the app in both the default pacing path and forced fallback path.

Record the actual pacing message and any inability to exercise a path. The forced-fallback path must verify VSync is disabled; a successful default launch does not prove VSync was selected. A synthetic/offscreen launch, a successful compile, and helper unit tests do not establish visual usability or actual display pacing.

### Lint / static checks

No dedicated formatter, linter, or static-analysis target/configuration was found in the inspected repository tree. Do not invent a project-standard command or claim compiler warnings constitute a separate static-analysis run. `git diff --check` is a useful whitespace check, not a C++ correctness check.

## Test organization

`tests/traffic_tests.cpp` covers fixture validation, first-step and completion behavior, R1 and R2 arrival cases, exact-arithmetic edges, pause/resume, reset, playback, backlog preservation, presentation interpolation, and trajectory agreement across supplied frame schedules. Preserve the six R2 precision fixtures and nine R1 fixtures when changing relevant numerical behavior.

`tests/pacing_tests.cpp` tests the platform-independent helper in `src/app/frame_pacing.hpp`: VSync/fallback selection, waiting only for the remaining frame budget, and avoiding an extra fallback wait in VSync mode. These tests do not use an SDL display.

CMake builds one Catch2 test executable from both files and registers its test cases with CTest. It separately registers the `traffic_headless` executable as `headless_baseline`.

## Validation expectations

Start with the smallest affected check, then satisfy the `AGENTS.md` gate: both presets, both CTest runs, and a headless execution. Preserve a reproducer for a numerical or timing regression. For determinism checks, compare relevant tick-level states under the same build and inputs, not rendered frames or just final averages.

Only a currently authorized handoff may explicitly narrow a gate. If a required check cannot run, record NOT CHECKED with its reason and leave its acceptance criterion unmet; do not turn it into PASS. A documentation-only task can verify references and patch integrity, but those checks do not validate the application.
