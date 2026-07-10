# Archived: the one-shot Stub backlog generators

These tools **generated** the `src/Stub/` backlog files at the start of the matching
campaign — empty `RVA()`-annotated stubs (and minimal placeholder classes) enumerating
the functions still to reconstruct:

- `gen_boundary_stubs.py` — the `Boundary*.cpp` proximity-pool stubs.
- `gen_attributed_stubs.py` — attributed (caller-traced) class stubs.
- `gen_unmatched_stubs.py` — `Unmatched.cpp` (the ~2400 unnamed `FUN_` bodies) as the
  `engine_unmatched` unit.

**Retired 2026-07-10.** Their job is done and re-running them is now *harmful*: the
campaign has been hand-**draining** `src/Stub/` (reconstructing each body and re-homing
it into its real class TU), so regenerating would clobber the real bodies + the
matchers' hand-left reconstruction blueprints. They are one-shot generators whose output
has since diverged from what any re-run would produce.

Sibling of the already-archived `gen_class_stubs.py` (see `../dynamic-trace/`), which
generated the runtime-trace-attributed stubs and was retired with the this/ecx trace
toolchain. Kept here for provenance; a couple of docs / docstrings still mention these by
name as historical context, not live dependencies.

Still live in the package: `discover_gaps.py` (the GapFunctions gap oracle — actively
regenerable), and the match tooling under `gruntz.match`.
