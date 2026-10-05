# Repository workflow

## Scope and review

Work only in this Traffic Simulator repository. Preserve unrelated changes and implement one coherent authorized unit at a time. Prefer the smallest clear solution; avoid unrelated refactoring, speculative abstractions, and dependency upgrades. Stop for owner review at the requested boundary. Do not merge, deploy, rewrite history, perform destructive operations, or start later milestones without explicit authorization.

Inspect the worktree and applicable instructions before editing. Do not overwrite someone else's work. A handoff must state any branch, commit, or push authorization; missing authorization is not permission to publish changes.

## Requirements and sources

The current handoff defines what changes now. This file defines how to work. [Architecture](docs/ARCHITECTURE.md) defines the durable simulation and timing baseline; any later accepted system document owns its explicitly assigned domain. [Project state](docs/PROJECT_STATE.md) records current scope and open questions, not authorization to start the next task.

Source code shows what is implemented. Tests are evidence only for behavior they cover. Neither alone proves intended design. TODOs, brainstorms, suggestions, and deferred ideas are not requirements.

If sources materially conflict, identify the conflict and check whether the handoff explicitly authorizes a baseline change. If not, report it rather than silently choosing a new behavior. Resolve minor reversible implementation choices using established conventions and report consequential choices.

## Architectural boundaries

- `src/core` owns simulation state and fixed-step timing and may depend only on the C++ standard library. `src/headless` and `src/app` use that same core. SDL3 and Dear ImGui belong only to the native app.
- Keep physical state in meters and seconds, independent of pixel coordinates and wall-clock reads. Presentation interpolation must never change world state.
- Preserve the accepted timing and numerical invariants in [Architecture](docs/ARCHITECTURE.md) unless the current handoff explicitly changes them. Do not infer variable-speed or multi-vehicle requirements from the existing constant-speed implementation.

## Validation

Run the smallest relevant check first, then the required broader gate. Verify both `dev` and `headless` presets with CTest and run the headless executable; commands and test organization are in [Testing](docs/TESTING.md). Report any required check that could not run. Do not silently reduce this gate because a targeted check passed.

Treat a build or synthetic display as distinct from native visual and interaction verification. Record exact commands, environment limits, and actual results. Never report an unexecuted check as passing. Preserve a regression case for a bug when practical.

## Documentation and completion

Update durable documentation when accepted behavior, interfaces, invariants, architecture, or verified workflow commands change. Keep temporary task details out of this file. Record implementation and validation status separately from owner acceptance in project state; do not mark a proposed next boundary as authorized.

Use the fresh-chat handoff and completion-report structures in [Codex workflow](docs/CODEX_WORKFLOW.md). Return concise evidence, deviations, remaining issues, and newly introduced debt, not a narrative of the coding process.
