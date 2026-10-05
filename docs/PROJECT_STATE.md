# Project state

Last updated: 2026-10-04 (America/Denver)

## Current stage

Early native traffic-simulation foundation: TS-001 with its R1 completion/pacing correction and R2 exact-arrival correction. This is not yet a playable intersection scenario.

Repository observation for TS-DOC-001: local `main` and `origin/main` were at `5bd7eb7f67dad2e2c3a68a23690bfbc5857e15b1`, and the worktree was clean before creating `codex/ts-doc-001-documentation-bootstrap`. The local `ai/ts-001-r1-completion-pacing` branch also points to that commit. Recheck refs and worktree status before future work.

## Current working systems / features

The inspected implementation provides one finite positive lane/speed fixture, one constant-speed vehicle, exact arrival classification, a completion record, a fixed-step playback driver, a native SDL3/Dear ImGui viewer, and a bounded headless runner. The native viewer provides Run/Pause, Reset, 1x/2x/4x playback, and VSync with a timed fallback path.

These are implementation observations supported by source and existing regression definitions. The build, CTest, and headless results from this documentation application are recorded in [Testing](TESTING.md); native interaction remains unverified here. The accepted contract is in [Architecture](ARCHITECTURE.md).

## Recently completed and acceptance status

The previous Codex report marked TS-001-R2 complete. Its exact-arrival implementation and regression fixtures are present in the inspected `main` baseline. The subsequent design review accepted R2 without requesting R3. The owner accepted TS-DOC-001 separately; that acceptance does not change the R2 contract or reconstruct merge history from the branch listing.

## Active implementation

The owner accepted TS-DOC-001 after reviewing commit `c0d4cfae96b9ae9eea068885e52b1e3974f2723b`. Integration into `main` and native visual verification are separate, unconfirmed outcomes at this closeout. No new gameplay implementation is authorized by this document.

## Accepted decisions affecting current work

C++20, a standard-library-only simulation core, one shared core for native and headless execution, meters/seconds, fixed ticks, and presentation-only interpolation remain the baseline. SDL3 and Dear ImGui stay at the app boundary. Reproducibility is for the same build and inputs, not promised cross-platform bitwise identity.

Product direction from the project discussion is a phased traffic-control game, beginning with a fixed four-leg two-way-stop intersection, a four-way-stop intervention, increasing demand, and measurements. A road editor and economy are outside that initial direction. This is a future scenario target, not implemented TS-001 behavior or a complete executable handoff. Interstate interchanges are a longer-term direction, not current scope.

## Known limits and verification gaps

The present milestone has no multiple vehicles, following, acceleration/braking, intersection conflict handling, demand generation, routing, editable scenarios, or persistence. These are scope limits, not newly discovered bugs. Tick-counter rollover remains outside the TS-001 contract.

No new runtime defect was established during this documentation application. The `dev` and `headless` build/test gates and both headless runs passed in this checkout. Native display interactions were not executed because no display session was available. See [Testing](TESTING.md) for exact verification status.

## Open design questions

The next single-lane queue needs an explicit contract for vehicle extents/reference positions, following and braking, a stationary obstruction or stop line, stop qualification, release behavior, and multi-vehicle completion. IDM was suggested during design discussion but has not been adopted here as an implementation requirement. Numerical parameters and acceptance fixtures are not settled by this bootstrap.

The previously discussed intersection design has not yet been reconciled into a repository-authoritative scenario contract. Import and verify that accepted design when the intersection becomes an implementation boundary; do not invent details that are absent from the repository or assume they were never decided.

## Next likely boundary — not authorization

After integrating the accepted documentation bootstrap, a separate M2 vehicle-following-and-stopping handoff could define a reliable single-lane queue before intersection behavior. M2 remains unauthorized. The current constant-speed arrival rule must not silently become an acceleration or multi-vehicle completion rule.

## Relevant documents

[Architecture](ARCHITECTURE.md) owns the current contract. [Testing](TESTING.md) owns verification guidance. [Codex workflow](CODEX_WORKFLOW.md) supplies the handoff/report templates. [AGENTS.md](../AGENTS.md) owns repository-wide operating rules. This state file is a snapshot, not a changelog or backlog.
