# Architecture

## Responsibilities and dependency direction

The application and headless runner exercise one shared simulation implementation. Keep platform concerns outside the core and numerical state independent of rendering.

| Area | Responsibility | Dependency boundary |
|---|---|---|
| `src/core` / `traffic_core` | Fixture validation, world state, fixed ticks, completion, and the elapsed-time driver | C++ standard library only |
| `src/headless` / `traffic_headless` | Bounded noninteractive execution and terminal output | Core and C++ standard library; no app dependency |
| `src/app` / `traffic_app` | SDL events, elapsed-time measurement, controls, frame pacing, and rendering | Core, SDL3, and Dear ImGui |
| `tests` / `traffic_tests` | Simulation and platform-independent pacing regression checks | Core, the pacing helper, and Catch2; no SDL requirement |

Dependency direction is `app -> core` and `headless -> core`, never `core -> app/headless`. The test dependency is not a core runtime dependency.

## State ownership and control flow

`World` owns the fixture, integer tick, stored distance, completion flag, and completion record. `Driver` owns a `World`, pause/run state, playback multiplier, elapsed-time backlog, and previous-tick position used for interpolation. The app measures elapsed time and supplies it to the driver; it does not own an alternate simulation. The headless runner steps its world directly.

The native loop processes events, supplies measured elapsed time, applies controls, reads the resulting state, renders, and performs presentation pacing. Pixel layout and pacing never redefine the simulation step. The contract below distinguishes exact completion classification, approximate stored distance, and interpolated display distance.

## TS-001 simulation and timing contract

`traffic_core` has no graphics or platform dependency. A `Fixture` provides lane length and constant speed as finite, positive `double` values in meters and meters per second. The baseline is one center-referenced vehicle at distance 0 on a 100.0 m lane, moving at 10.0 m/s. Every fixed tick advances by `speed * 0.05 s`.

For this constant-speed milestone, arrival means the exact mathematical predicate `tick * speed >= 20 * lane_length` for the supplied binary64 values. The internal classifier requires IEEE 754 binary64 with subnormals (checked at compile time). It splits each positive input into an integer significand and a power-of-two exponent. The tick-times-speed significand fits in 117 bits for every `uint64_t` tick; 20 times the lane significand fits in 58 bits. Four 32-bit partial products form the former in two standard 64-bit limbs. Comparing the resulting top binary exponents, then corresponding bits when those exponents match, preserves the exact comparison sign without rounded floating-point products, wide shifts, or `long double` precision assumptions. Tick-counter rollover is outside this milestone.

Active distance remains approximate presentation state: it is derived from the tick, rounded to `double`, kept finite and nondecreasing, and held strictly below the endpoint if rounding would reach it. It never decides completion. On the first completing tick the world clamps distance to the exact input lane length, removes the vehicle from active state, and retains one completion record. The integer tick is the clock authority; displayed time remains `tick * 0.05 s`. The baseline ends at tick 200 / 10.0 s. A 100.25 m lane ends at tick 201 / 10.05 s, deliberately reporting the tick boundary rather than the analytical crossing time. Further steps do nothing until reset.

`Driver` accepts caller-supplied elapsed nanoseconds. It multiplies accumulated active elapsed time by 1, 2, or 4, then executes unchanged 50,000,000 ns simulation steps. Pause ignores new elapsed time while preserving an existing sub-step remainder; resuming does not catch up through the pause. The per-call step budget defers whole steps in its backlog and never drops them. Completion clears the backlog and stops the driver. Reset restores the baseline, paused at 1x, and clears history and counters.

The driver stores the previous completed position for presentation. `display_distance()` interpolates using the fractional accumulated step; if a whole step is outstanding, it displays the latest completed state. Rendering and window size cannot mutate the world. The headless runner calls `World::step()` directly and uses the same movement rule without SDL, wall-clock timing, or sleeping. Reproducibility is specified for the same build and inputs, not bitwise equivalence across platforms.

## Persistence and contract evolution

TS-001 has no save/load or scenario-file contract. The current app also disables Dear ImGui ini-file persistence. Do not add persistence, a road editor, or a generalized scenario format as incidental infrastructure.

The constant-speed arrival predicate above is specific to TS-001. A later acceleration or multi-vehicle task must explicitly define its own movement, vehicle-reference, spacing, and completion rules and identify any baseline it replaces. The bootstrap does not choose those rules. Keep the present contract together until a substantive second system justifies a separate document.
