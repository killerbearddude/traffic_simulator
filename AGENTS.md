# Repository workflow

- Work only in this Traffic Simulator repository. Preserve unrelated changes and keep each milestone reviewable; stop for owner review after the requested boundary. Do not merge or start later milestones automatically.
- `src/core` owns simulation state and fixed-step timing and may depend only on the C++ standard library. `src/headless` and `src/app` use that same core. SDL3 and Dear ImGui belong only to the native app.
- Keep physical state in meters and seconds, independent of pixel coordinates and wall-clock reads. Presentation interpolation must never change world state. See `docs/ARCHITECTURE.md` for the TS-001 contract.
- Verify both `dev` and `headless` presets with CTest and run the headless executable. Treat a build or synthetic display as distinct from native visual and interaction verification. Record exact commands and any checks that could not be run.
- Do not expand TS-001 into following, intersection behavior, or unrelated infrastructure.
