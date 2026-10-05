# Codex workflow

## Operating loop

Design and accept one change; update durable behavior documents when needed; hand off that coherent change; implement and validate; return the completion report; review and accept or correct; update project state. A suggestion in a completion report never authorizes the next task.

The handoff must work in a fresh Codex chat with the repository, applicable `AGENTS.md`, referenced documents, and current code/tests. Use a new chat for a new coherent unit. Keep the same chat for a focused correction or rerun of the unit just completed. Never require an agent to recover unstated decisions from earlier conversation history.

Use the question-specific source rules in `AGENTS.md`. Keep current status in `PROJECT_STATE.md`, intended behavior in the architecture or justified future system documents, and task-specific changes in the handoff. Do not archive every handoff in the repository by default; these reusable templates have an ongoing purpose, while individual reports may remain in the review conversation.

## Implementation handoff template

Fill in the task-specific fields and acceptance conditions before sending. Reference only documents that exist in the starting checkout or are explicitly supplied by the task. Proposed next features are not accepted requirements.

~~~~markdown
# Codex Implementation Handoff — Traffic Simulator

## Task
ID: [identifier]
Title: [one coherent change]

## Objective
[Observable result and why this unit is needed; 1–3 sentences.]

## Accepted Behavior / Design
[Explicit requirements for this delta. Identify every existing contract that changes.]
These requirements are accepted for this task, not optional suggestions.

## Out of Scope
[Adjacent behavior that must not be implemented.]
Do not promote TODOs, brainstorms, deferred ideas, or a suggested next milestone into scope.

## Authoritative References
- `AGENTS.md`
- `docs/PROJECT_STATE.md` — starting status, not task authorization
- `docs/ARCHITECTURE.md` — durable baseline and timing/numerical invariants
- `docs/TESTING.md` — commands and validation gate
- [Additional existing or supplied behavior document, only when needed]

The handoff defines the current delta. The repository documents remain the baseline unless this handoff explicitly changes them. Report material conflicts.

## Repository Starting Points
Repository: `killerbearddude/traffic_simulator`
Starting branch/commit: [verified ref or explicit instruction to inspect current refs]
Work branch: [specified branch policy]
Read first: [relevant existing source/test paths]

Inspect worktree status and applicable instructions. Preserve unrelated changes. Follow necessary dependencies, but avoid unrelated exploration. Do not assume a review branch still exists because an old report names it.

## Constraints
Preserve unrelated behavior. Keep changes simple and scoped; no unrelated refactoring or dependency upgrades. Preserve the standard-library-only core and presentation boundary. Preserve fixed-step, unit, numerical, and reproducibility contracts except for changes explicitly authorized above. Do not add persistence or public interfaces incidentally.

## Implementation Authority
You may inspect and modify files required by this task, add relevant tests, update affected documentation, and run appropriate checks.
Commit/push authorization: [state exactly; otherwise neither is authorized]
No automatic merge, deployment, history rewrite, destructive operation, or next milestone.

## Acceptance Criteria
1. [Observable requirement with a concrete expected result.]
2. [Affected edge or failure case and expected result.]
3. [Required regression evidence, including fixture/tick expectations when relevant.]
4. Unrelated behavior is preserved and affected durable docs agree with the result.
5. Every required validation check has passing evidence, or the task is reported PARTIAL/BLOCKED with the unmet criterion identified.

## Validation
Run targeted checks first, then the gate in `docs/TESTING.md`.
Required targeted checks: [commands or exact selection]
Required full gate: [both presets/CTest/headless unless an explicit scoped exception is stated]
Native visual/interaction checks: [required paths, or why not applicable]
Explicit gate exception, if any: [scope and reason; otherwise None]

For numerical changes, preserve a regression case and compare tick-level behavior. For presentation changes, distinguish helper tests and synthetic launches from native visual checks. Never claim a check passed without running it.

## Stop / Escalation Conditions
Stop expanding implementation and report a material requirement ambiguity, conflicting authority, unavailable required interface, unapproved architecture/invariant change, or acceptance criterion that requires materially broader scope. Resolve minor reversible choices using established conventions and report important choices afterward.

## Completion Report
Use the completion-report template in `docs/CODEX_WORKFLOW.md`. Include exact validation commands, acceptance evidence, deviations, and repository state. Do not continue into the next unit or replace evidence with a coding narrative.
~~~~

## Completion report template

Use COMPLETE only when the authorized unit and its required checks are satisfied. Implementation complete, owner accepted, committed, pushed, and merged are different facts.

~~~~markdown
# Codex Completion Report — Traffic Simulator

## Task
ID: [task ID]
Title: [task title]

## Status
COMPLETE / PARTIAL / BLOCKED
[State which acceptance conditions remain unmet, if any.]

## Result
[2–5 sentences describing what actually changed.]

## Repository State
Starting branch/commit: [observed]
Ending branch/commit: [observed]
Commits created/pushed: [actual values or None]
Working tree: [observed status; identify unrelated changes preserved]
Merge/deployment: [actual action or None]
Owner acceptance: [explicitly received or Awaiting review]

## Files Changed
- `path` — [purpose]

## Behavior / Design Implemented
[Actual accepted behavior implemented; not a list of future intentions.]

## Important Implementation Decisions
[Only choices not already dictated by the handoff, with reasons; or None.]

## Deviations From Handoff
[Differences and reasons; or None.]

## Validation Performed
Environment: [OS/toolchain/display conditions relevant to this run]

### Tests
- `[exact executed command]` — PASS / FAIL; [relevant observed result]

### Build / Lint / Static Checks
- `[exact executed command]` — PASS / FAIL; [relevant observed result]

### Native Visual / Interaction Verification
[Actual display, pacing paths, controls/resizing checked, and observed result.]
[Use Not applicable when unaffected; do not substitute synthetic display evidence.]

### Required Checks Not Run
- [check] — NOT CHECKED; [specific limitation]
[Or None. Keep unexecuted checks out of the PASS/FAIL execution lists.]

## Acceptance Criteria
| Criterion | Status | Evidence |
|---|---|---|
| [criterion] | PASS / FAIL / NOT CHECKED | [command/result or inspection] |

## Documentation Updated
- `path` — [change]
[Or None, with why no update was needed.]

## Remaining Issues
[Unmet requirements, blockers, or unresolved conflicts; or None.]

## Technical Debt Introduced
[Only debt created or knowingly deferred by this implementation; or None.]

## Suggested Next Implementation Boundary
[One concise recommendation; or None.]
This is informational only and does not authorize additional work.
~~~~

## Maintenance

Update `AGENTS.md` for enduring operating rules, not a current milestone. Update `PROJECT_STATE.md` when a unit is accepted, scope changes, or a meaningful problem/next boundary changes; retain the distinction between implemented and accepted. Update `ARCHITECTURE.md` when an accepted boundary or invariant changes. Update `TESTING.md` when commands, gate requirements, or environment constraints change.

Create a system document only for a substantive domain that cannot be kept clear in the existing contract. Create an ADR only when a consequential decision needs its rationale preserved. Do not create placeholders, a duplicate Git changelog, or speculative feature specifications merely to fill out a documentation tree.
