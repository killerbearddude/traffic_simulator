# M2 — Following and stopping

Status: owner-approved design for TS-002; implementation and validation are not yet complete.

This document owns the M2 single-lane motion, stopping, release, and completion contract. It does not replace the TS-001 constant-speed regression contract in `docs/ARCHITECTURE.md`. The current implementation handoff authorizes the change; this system document preserves the resulting behavior for future tasks.

## 1. Purpose and boundary

Implement one finite six-vehicle demonstration of a queue forming at a persistent stop line and clearing after a one-way release command. Vehicles use the Intelligent Driver Model (IDM), not scripted trajectories. Each vehicle must complete its own stop before crossing the line.

The native application and headless runner use the same standard-library-only simulation. The model is a game-oriented development model, not a calibrated traffic-engineering tool. Parameter values and dwell time are project settings, not claims about legal stopping requirements.

There is one straight lane and no spawning, randomness, turning, intersection arbitration, routing, lane changing, persistence, scenario-file loading, or general event framework. Do not add these as supporting infrastructure.

## 2. Units, fixture, and configuration

Use `double` physical calculations in meters, seconds, meters/second, and meters/second squared. The integer tick is the simulation clock. One interval is 0.05 seconds, or 50,000,000 nanoseconds. Playback changes how many normal intervals execute, never their size.

| Setting | Approved value |
|---|---:|
| Upstream / downstream lane boundaries | 0 m / 600 m |
| Physical stop line | 400 m |
| Vehicle count | 6 |
| Initial center positions, front to back | 240, 200, 160, 120, 80, 40 m |
| Initial speed, every vehicle | `65.0 / 3.6` m/s, corresponding to 65 km/h |
| Vehicle length / width | 4.5 m / 1.8 m |
| Desired free-road speed `v0` | `65.0 / 3.6` m/s |
| Desired time headway `T` | 1.5 s |
| Standstill gap `s0` | 2.0 m |
| Acceleration parameter `a` | 1.5 m/s² |
| Comfortable deceleration parameter `b` | 2.0 m/s² |
| Acceleration exponent `delta` | 4 |
| Initial release state | Held |
| Initial qualification / dwell / event records | None / zero / absent |
| Initial native playback | Paused at 1× |

Keep one validated C++ configuration, shared by all six vehicles and fixed for the duration of a run. No live parameter editor is required. Use stable IDs; the canonical fixture labels the front-to-back vehicles 1–6. IDs survive into completion records and are not reused during a run.

The initial bumper gaps are 35.5 m. The leading front bumper begins at 242.25 m, 157.75 m upstream of the stop line. These are consequences of the fixture, not additional tuning parameters.

### Configuration validation

Reject nonfinite or invalid model parameters, nonpositive physical dimensions, negative or nonfinite initial speeds, duplicate IDs, nonfinite positions, overlapping initial vehicles, and inconsistent fixture geometry before running. Do not repair invalid configuration silently. State which field or vehicle is invalid.

The shipped fixture has exactly the values above. Smaller or deliberately invalid C++ fixtures may exercise individual functions and invariants in tests; they do not create a new user-facing scenario configuration interface.

## 3. Vehicle extent, order, and lifetime

A vehicle's stored longitudinal position `x` is its center:

```text
front = x + length / 2
rear  = x - length / 2
real_leader_gap = leader.rear - follower.front
```

The real leader is the nearest active vehicle ahead in the single-lane order, including a departing vehicle downstream of the stop line. A leader is not removed merely because its center or front reaches the downstream boundary.

M2 completes a vehicle at the first committed tick boundary where its rear is at or beyond 600 m. For the approved length this is `x >= 602.25 m`. Retain its actual computed state in the completion record; do not snap its center to the boundary. Remove it from active occupancy only at that commit. It participates in the start-of-tick leader relationships and safety checks for the entire interval in which it completes.

Each vehicle completes exactly once. At every committed boundary:

```text
active vehicles + completed vehicles = 6
```

Preserve front-to-back order. When all six have completed, stop the simulation clock and playback, clear pending elapsed-time backlog, and retain results until Reset. Further motion steps do nothing until Reset.

TS-001 remains different: its vehicle is center-referenced and completes under its existing exact constant-speed arrival rule. Do not apply either completion rule to the other model.

## 4. Exact IDM variant

Let `v` be the follower speed, `vL` the leader speed, `s` the bumper gap, and `dv = v - vL` positive when closing. Use:

```text
s_star = s0 + max(0, v*T + v*dv / (2*sqrt(a*b)))
alpha  = a * (1 - (v/v0)^delta - (s_star/s)^2)
```

When there is no applicable leader or stop constraint, use free-road IDM:

```text
alpha_free = a * (1 - (v/v0)^delta)
```

The `max(0, ...)` is part of the approved model, not optional. Do not substitute the variant with an unrestricted negative dynamic desired-gap term. `alpha` is the calculated acceleration; it is not necessarily equal to configuration parameter `a`.

`b` is comfortable deceleration, not a hard maximum. Do not clamp braking to `-b`, add an unapproved maximum-deceleration law, cap speed after integration, or introduce reaction delays or randomized drivers. `s0` is a desired standstill gap, not a hard minimum enforced by position correction. A positive actual gap slightly below 2 m is not itself a collision.

Require positive finite model gaps before division. A zero gap is not a valid IDM input. Reject invalid or nonfinite calculations with a diagnostic rather than replacing denominators with an arbitrary epsilon or substituting a plausible acceleration.

### Real leader and virtual stop constraint

The stop constraint is a stationary, zero-length virtual obstacle whose longitudinal reference is 401.5 m. It is not a real vehicle and never enters active/completed accounting.

```text
virtual_leader_speed = 0
virtual_gap = 401.5 - vehicle.front
```

With `s0 = 2 m`, this targets a front-bumper stop approximately 0.5 m before the physical line. The virtual obstacle's location is not permission to cross the physical line.

For a vehicle without departure permission, evaluate IDM against the real leader when present and against the virtual stop obstacle. Use the lower acceleration: more braking or less acceleration. Without a real leader, use the virtual-obstacle result alone.

When that vehicle has both its own stop qualification and global release permission, remove only its virtual stop constraint. Keep following its real leader, or use free-road IDM if no real leader remains. The global release command must not remove every vehicle's stopping constraint at once.

## 5. Synchronous motion and ballistic integration

Every vehicle's motion decision reads the same start-of-interval state. Precompute proposed motions for all active vehicles before changing physical state. Do not allow container iteration order to expose an already-updated leader to a follower.

Treat `alpha` as constant during the interval of length `dt = 0.05`:

```text
v_next = v + alpha*dt
x_next = x + v*dt + 0.5*alpha*dt*dt
```

This is ballistic numerical integration, not an exact integration of changing IDM acceleration.

### A stop inside the interval

If `alpha < 0` and speed reaches zero before the interval ends, let `tau = -v/alpha`. Integrate only through `tau`, then remain stationary for the rest of that interval:

```text
x_next = x + v*tau + 0.5*alpha*tau*tau
v_next = 0
```

An already stationary vehicle with a negative acceleration request stays stationary. An already stationary vehicle with positive acceleration is allowed to start moving normally. Equality at the interval end is an ordinary zero-speed endpoint.

Do not clamp every braking vehicle to zero, and do not merely clamp negative final speed while retaining the unrestricted full-interval position update. A vehicle braking without reaching zero during this interval must retain its positive final speed.

The same piecewise trajectory must be used by the independent safety and dwell checks. Numerical substeps, acceleration retuning, and position snapping are not substitutes for the approved integration method.

## 6. Independent safety validation and atomic commit

Before committing, validate finite state, nonnegative speeds, nondecreasing positions, order and separation, and the physical stop-line restriction. Keep this safeguard separate from the function that chooses IDM acceleration.

Check separation over the entire proposed interval, not just at its endpoints. For two piecewise-ballistic trajectories, partition at either vehicle's stopping time. On each segment their gap is quadratic or linear. Inspect segment endpoints and any interior minimum. Endpoint-only checks or a handful of time samples are insufficient to establish the invariant.

Overlap or passing through a leader invalidates the run. Zero/nonpositive gaps must also be diagnosed before they can become IDM inputs. Do not enforce the 2 m desired gap as a collision boundary.

A vehicle without departure permission must never have its front beyond 400 m during an interval. Merely touching the line is not a recorded crossing; entering beyond it is. The first committed boundary with `front > 400 m` records a crossing, after the whole interval has passed the permission/safety checks.

If any proposed motion or calculation fails, do not partially commit other vehicles. Retain the last valid physical state, tick, dwell counters, and vehicle event records. Latch invalid status and record the failure category, attempted interval/tick, affected IDs, relevant states/gaps/positions, fixture identity, and applied release-command tick when present. The fixture is deterministic and has no seed to invent.

The native app stops advancing and exposes the diagnostic. The headless runner terminates unsuccessfully. A failed run cannot resume without Reset. No teleportation, position clamps, hidden braking replacement, automatic retry under different parameters, or manufactured completion is allowed.

A headless time limit or failed acceptance checkpoint is not automatically a physical safety violation. Report those failure reasons separately.

## 7. Stop qualification

| Condition | Approved rule |
|---|---|
| Eligibility | Foremost active vehicle that has not crossed the physical line |
| Position | Front between 399.25 m and 399.75 m, inclusive |
| Speed | At or below 0.1 m/s throughout the interval |
| Dwell | 20 complete, consecutive qualifying intervals |
| Qualification | Latched at the ending tick of interval 20 |

Only the eligible vehicle can earn dwell. Stopping farther back in the queue does not count. All not-yet-permitted vehicles still consider the stop obstacle for braking, regardless of dwell eligibility.

Credit an interval only if eligibility, position, and speed conditions hold throughout it. Do not credit a partial interval in which the vehicle first becomes eligible, enters the qualifying region, or slows through the threshold. If any condition breaks before qualification, reset the counter to zero.

Use an integer dwell counter, not repeated floating-point addition of 0.05 seconds. For a monotone piecewise-ballistic trajectory, position and speed extrema can be checked directly. Evaluate eligibility against the consistent boundary state: a follower newly exposed by its leader crossing during an interval does not receive a full interval of dwell credit retroactively.

After qualification, retain that result for this stop until Reset. While held, keep the virtual stop constraint even on a qualified vehicle. Qualification may be earned during the hold, and that completed dwell still counts after release.

For example, a first fully qualifying interval of tick 400 to 401 yields qualification at tick 420 after 20 intervals. If release is enabled, departure motion may start during 420 to 421, never during the interval that earned qualification.

## 8. Release, commands, and tick ordering

Release queue is a one-way permission for the current run. It does not delete the stop line, skip any vehicle's own stop, or impose a fixed departure interval. Departure acceleration and following gaps come from IDM. Only Reset restores the hold.

Apply commands at simulation boundaries and record their applied tick. A native release request takes effect at a boundary before subsequent motion, not retroactively inside an interval. A paused request can be latched for that boundary without advancing simulated time. Repeated requests after release are idempotent. No command scheduler framework or arbitrary script loader is needed.

For each outgoing interval from tick `k` to `k+1`:

1. Apply a due release command at boundary `k`.
2. Read start-of-interval vehicle order, qualification, and release state; determine departure permissions and dwell eligibility.
3. Calculate all accelerations and proposed piecewise-ballistic trajectories.
4. Validate proposed trajectories and determine complete-interval dwell eligibility.
5. Commit the motions and tick together, then the corresponding dwell/qualification, crossing, and completion records for tick `k+1`.
6. Expose the committed state to the driver and presentation; stop if the run has completed or become invalid.

A qualification earned at `k+1` cannot authorize the motion from `k` to `k+1`. A command due at tick 1200 must execute between the 1200th and 1201st completed steps, even when one elapsed-time call processes many steps.

## 9. Native behavior and presentation

M2 is the native default, with no native scenario picker in this milestone. Preserve Run/Pause, Reset, 1×/2×/4× playback, VSync handling, and `--force-fallback`.

Run/Pause does not count paused wall time, change tick size, or lose deferred whole steps. Retain the existing substep-remainder semantics. Reset restores the entire original fixture at tick zero, held and paused at 1×, and clears qualification, dwell, release commands, event records, diagnostics, interpolation history, and time backlog.

Native release is manual. It may occur early; every vehicle must still stop. The native app has no automatic release at 60 seconds and no 180-second session limit. A valid held run may remain held until the user releases or resets it.

Provide two fixed, labeled, top-down views of the same state:

| View | Extent and purpose |
|---|---|
| Overview | Full 0–600 m lane plus enough downstream margin to show full-vehicle clearance |
| Enlarged stop-line view | 340–440 m, showing queue gaps, individual stops, and initial departures |

Use the same physical vehicle dimensions and interpolated positions in both views. Within each view use a uniform world-to-pixel scale, not independent vehicle exaggeration. No mouse pan, zoom, camera tracking, vehicle selection, production assets, or alternative rendering framework is required. Fit/clip and label the views so window resizing does not change physical state or hide the distinction between the two views.

Keep controls and essential status readable: run/paused/complete/invalid, tick/time, held/released state, active/completed counts, and diagnostic details on failure. A small read-only presentation of existing per-vehicle stop/event state is sufficient; do not add an analytics subsystem.

Rendering is observational. Interpolation never modifies simulation state, safety decisions, qualifications, or event ticks. If a whole step is pending, show the latest committed state as in the existing driver policy.

## 10. Headless scenarios and event records

Required selections:

```sh
traffic_headless                   # M2
traffic_headless --scenario m2     # M2 explicitly
traffic_headless --scenario ts-001 # Original constant-speed baseline
```

Unknown scenarios or malformed arguments fail with a useful usage message and nonzero exit. Do not silently fall back to M2. This selects built-in C++ fixtures, not scenario files or a generalized loading framework.

The default M2 headless run checks the pre-release state at tick 1200, then applies release at that boundary before stepping to tick 1201. It allows at most 3600 completed steps (180 simulated seconds), inclusive of completion on tick 3600. Wall-clock delays and rendering are irrelevant.

Retain one compact event record per vehicle: stable ID, stop-qualification tick, first stop-line-crossing tick, and rear-clearance completion tick. Absent events must remain explicitly absent, not appear as fabricated zero-time events. Print these records and the final summary. No CSV, JSON, charts, database, or generic event bus is required.

Crossing and completion ticks are the first committed boundaries satisfying their conditions. They are not analytically reconstructed fractional crossing times. Qualified and released permission must have existed at the start of the crossing interval; continuous trajectory checks still detect unauthorized within-interval motion.

On a timeout, report the remaining vehicles and their states, and exit unsuccessfully. Do not delete them or manufacture records. Distinguish timeout, checkpoint failure, invalid input, and physical/numerical invalidity in diagnostic text; exact numeric exit-code taxonomy is an implementation detail.

## 11. Required acceptance observations

### Immediately before release: tick 1200 / 60.0 seconds

All six vehicles remain upstream of the physical line; all speeds are at or below 0.1 m/s; active count is six; completed count is zero. Vehicle 1 has qualified its stop. Vehicles 2–6 have not. No vehicle has crossed the line and no safeguard has activated.

### After release and throughout the run

Each vehicle earns its own qualification before its first line crossing. Every crossing has both permissions. Preserve order, nonnegative speeds, finite/nondecreasing positions, valid separation, and six-vehicle accounting. Record no safety activation in the canonical run.

### Completion: no later than tick 3600 / 180.0 seconds

All six have completed exactly once through rear clearance. Active count is zero and completed count is six. Each event record is complete and has an ordered qualification, crossing, and completion. The simulation remains stopped on subsequent step attempts until Reset.

The time limit is a test budget, not an expected trajectory or permission to retune the model. If the implementation cannot satisfy this contract, report the contradiction or defect; do not silently weaken the checkpoint, alter parameters, extend the timeout, or adopt another IDM variant.

## 12. Repeatability and boundary preservation

With the same build, fixture, and release-command tick, direct stepping and elapsed-time driving at 1×/2×/4× under regular, irregular, and backlog-producing frame schedules must produce the same committed tick-level physical state, dwell/qualification state, validity status, and event records.

Native human clicks at different simulated ticks are different command traces and need not produce identical results. For repeatability tests, replay the same tick-stamped command rather than the same wall-clock click time.

Do not promise cross-platform bitwise equality. Keep the original TS-001 precision regressions and timing behavior intact. The exact constant-speed binary64 arrival classifier is not used to decide M2 arrival.

## 13. Deferred decisions

M3 intersection geometry and route transitions, M4 right-of-way/conflict rules, and M5 spawning, scenario files, and measurement exports remain outside TS-002. A next-milestone suggestion does not authorize any of them.

Reference background: the IDM author's [model and ballistic integration notes](https://traffic-simulation.de/info/info_IDM.html) explain the model family and stopped-vehicle integration. This document's explicitly approved equation, parameters, combined constraints, and gameplay rules are the M2 contract; do not import a different variant or parameter set from a reference implementation.
