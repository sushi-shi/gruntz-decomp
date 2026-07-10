# Archived: the runtime this/ecx trace toolchain

These tools attributed functions to C++ classes by **observing the `this`
pointer (`ecx`) at call time** in a running `GRUNTZ.EXE` (Frida, with a gdb
fallback), then clustering co-called functions and tying each cluster to a class.

**Retired 2026-07-10.** Class attribution is now done **statically** from the
binary + the reconstruction, which is bigger, reproducible, and correct:

- **`gruntz.analysis.vtable_scan`** recovers *every* vtable in the EXE — RTTI
  *and* non-RTTI (via code-ref / reloc-run analysis) — and, for any function,
  `find_holding(rva)` reports which vtable/slot holds it.
- **`VTBL(Class, addr)` in `src/`** binds each vtable start to its class (236
  bindings). Together they map any vtable-slot function straight to its owning
  class — with **no runtime, no Frida worktree, and no thiscall-only caveat**.

Measured: for the `(unmatched)` backlog, RTTI COLs named only 19 vtables, but the
`vtable_scan` + `VTBL()` catalog covered **all 38 vtable-slot functions**.

Why the trace was the wrong tool:
- Not reproducible without provisioning the Frida catch-ecx worktree (168 MB
  retail-asset fetch); never regenerated in CI.
- Its class attribution is reliable **only for `__thiscall`** — `__cdecl`/
  `__stdcall` callees get a stale-ecx (wrong) owner.
- A leaked/aliased `this` mis-merges classes.

Kept here for provenance. Two headers still cite `this_cluster` as the *evidence*
for a historical class split (`include/Gruntz/BattlezMapConfig.h`,
`include/Gruntz/Brickz.h`) — those are dated notes, not live dependencies.

Static replacements that stay in the live package: `caller_callee` / `caller_audit`
(static call-xref), `xref`, `vtable_scan`, `vtable_hierarchy`.

Files: `tie_classes.py` (function→class join), `this_cluster.py` (label-propagation
clustering), `gen_frida_script.py` + `gdb_trace_this.py` (tracers),
`provision_trace.py` (worktree/asset setup), `gen_class_stubs.py` (integrated the
trace's ties as `engine_discovered` stubs), `dynamic-analysis.md` (the how-to).
