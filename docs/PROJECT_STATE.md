# Project state

Last updated: 2026-10-05 (America/Denver)

## Current stage

TS-001's exact constant-speed core and timing contract remain available through explicit
headless selection. The TS-DOC-001 documentation bootstrap was merged into `main` at
`d4936dec9b3b23503b2fc2434bffbc79127b84b9`. TS-002 implements the owner-approved
M2 following-and-stopping contract. The owner accepted TS-002 after native visual and
interaction verification on a usable desktop. TS-002 is the completed M2 milestone;
M3 is not authorized.

## Implemented scope

The M2 core holds a six-vehicle fixed fixture, IDM following and virtual stop constraint,
synchronous ballistic steps, independent swept-separation and line checks, individual
20-interval stop qualification, a one-way release, crossing and rear-clearance records,
and latched invalid-run diagnostics. The headless default checks the 60-second held state,
releases at tick 1200, and enforces the 180-second completion limit. The native app uses
the same M2 world with manual release, playback controls, two fixed top-down views, and
read-only state/event display. TS-001 remains a separate headless scenario.

The accepted M2 behavior is in [Following and stopping](systems/FOLLOWING_AND_STOPPING.md).
[Architecture](ARCHITECTURE.md) preserves TS-001 and the shared boundaries.
[Testing](TESTING.md) describes the validation gate. The TS-002 completion report records
actual test and sanitizer results separately from owner verification. Both preset suites passed 39/39,
and the isolated AddressSanitizer/UndefinedBehaviorSanitizer suite passed 39/39
with leak detection disabled due an environment limitation; LeakSanitizer was not
verified. Native visual and interaction verification passed on the owner's usable
desktop, including the normal and forced-fallback paths. The earlier Codex-host
attempts could not initialize an SDL video device and did not provide that evidence.

## Boundaries and open work

M2 remains one fixed lane and fixture. There is no intersection arbitration, routing,
spawning, lane changing, persistence, editor, or scenario-file interface. Tick-counter
rollover remains outside the TS-001 contract. A future intersection milestone needs its
own authorized handoff and accepted system design. No next milestone is authorized by
this state document.
