# Sema greenfield — design (for review, no code yet)

Reconstruct `gruntz sema` as **one process with code paths** (the Rust-crate
shape: a lib `core/`, bin paths `sema/`, `cli.py` as `main.rs`) instead of the
current scripts-execute-scripts wrapper stack. Status: design only; the
package-boundary commits (`48b899438`, `0a77638a6`) are the P0 groundwork.

## Why (evidence from the usage log)

9,771 logged invocations across 17 worktrees (`build/gruntz_sema.log`):

| tool | calls | | tool | calls |
|---|---:|---|---|---:|
| disasm | 4,143 | | strings | 90 |
| rva | 2,043 | | rename | 79 |
| xref | 1,657 | | map | 69 |
| match | 1,283 | | vtable | 12 |
| class | 384 | | refs/hover/symbol/def | 5/2/0/0 |

Every call is a **cold start**: re-read the 2.5 MB EXE, re-parse the ~4k-row
`symbol_names.csv` + ghidra exports, answer one question, exit. `xref` rescans
all of `.text` per call. Worst offender: `sema class <fn>` writes a temp CSV,
spawns `vtable_hierarchy --csv`, then spawns it twice more for owner tables —
four python processes for one question.

Script-sprawl also *hides breakage*: this audit found `sema strings` had been
failing 71% of its calls (hardcoded pre-flake EXE path) and the caller-callee
gate metric silently skipped for a day (archived module still imported by a
live one). A single-process tree with direct imports fails loudly at import
time instead.

## The umbrella problem

`analysis/` (39 modules) conflates two roles:

- **interactive query engines** (xref, dump_target*, string_xref*, exe_map,
  vtable_scan, vtable_hierarchy, clangd_query) — sema's backends;
- **one-shot campaign audits** (data_home, interleavers, stale_walls, …).

`xref.py` shows the cost: 4 campaign tools (`caller_callee`, `data_home`,
`globals_attribute`, `interleavers`) import its *internals* (`_text`, reloc
walk, ILT chase) — the shared thing is **PE primitives**, not the xref tool.
That library deserves a name; the umbrella denies it one.
(* moved to `sema/` in P0.)

## Target architecture

```
scripts/gruntz/
  core/               # the lib crate - pure queries over loaded-once state
    pe.py             #   EXE bytes, sections, .text walk, reloc sites, ILT thunk chase
    symbols.py        #   rva<->name db: symbol_names + ghidra + sizes + units
                      #   + demangled aliases (CClass::Member / bare Member)
    report.py         #   objdiff report.json accessor (per-fn %, per-unit)
  sema/               # bin paths - each subcommand = functions over core
    xref.py disasm.py rva.py strings.py classof.py vtable.py match.py map.py
    dump_target.py    #   target-side disasm engine (already here)
    clangd.py         #   the one legitimate subprocess family (clangd server)
  cli.py              # main.rs: argparse -> direct call, same process
  analysis/           # shrinks to what it honestly is: one-shot campaign audits,
                      # importing core/ like everyone else
```

**Context object.** `core.Context` lazily loads and caches: EXE bytes + section
table, reloc sites (sorted), the symbol db, ghidra exports, report.json. Every
sema function takes `ctx` first: `xref.callers(ctx, rva)`,
`rva.dossier(ctx, rva)`, `strings.of(ctx, rva)`. One load per process. No
on-disk cache — staleness traps (see docs/gotchas.md on measuring stale
`build/` state); in-process + batch mode is enough.

**Subprocess policy.** Child processes ONLY for true externals: `llvm-objdump`
/`objdump` (disasm), `clangd` (LSP family), `wine cl` (`--rich` /Z7 objs).
Never `python -m gruntz.*` from inside gruntz.

**Batch/REPL mode** (the agent win): `gruntz sema -` reads newline-delimited
subcommand lines from stdin, answers each against ONE Context. A matcher's
40-query investigation currently pays 40 cold parses; batch pays 1. Output
framing: echo the command line as a `== gruntz sema <cmd>` header before each
answer (same text as today's per-call output; trivially splittable).

## Compatibility contract

- `gruntz sema <cmd> <args>` surface unchanged — nvim (`editor/nvim`, the
  vt/vb/vd/… bindings) and agent muscle memory keep working.
- `build/gruntz_sema.log` format unchanged; batch queries log one line each.
- `python -m gruntz.sema.<tool>` stays runnable per module (thin `main()`
  building a Context and dispatching) — agent briefs reference module paths.
- rc convention tightened: rc=0 answered, rc=1 answered-NO (diff differs, not a
  virtual, no hit), rc=2 real error. Today 1-and-error are conflated, which is
  why disasm "fails" 25% in the log (it's `--diff` reporting a difference).

## Migration plan

- **P0 (done):** package boundaries — `sema/` one module per subcommand,
  sema-only engines moved in, cli.py = parser + shims.
- **P1 (the core rewrite):** extract `core/pe.py` + `core/symbols.py` from the
  duplicated internals of `xref.py`/`dump_target.py`/`strings.py`/
  `vtable_scan.py` (each currently re-implements PE parsing + name loading);
  `core/report.py` from the readers in `rva.py`/`match`/status. Rewrite sema
  modules as `f(ctx, ...)` functions; cli dispatches in-process. Delete the
  `run_tool()` python-subprocess helper from the sema path.
- **P2:** batch/REPL mode + the rc convention + log field for it.
- **P3:** migrate the 4 attribution consumers (`caller_callee`, `data_home`,
  `globals_attribute`, `interleavers`) from `xref` internals onto `core/`;
  `sema/xref.py` stops being a library. `vtable_scan`/`vtable_hierarchy`
  re-base onto `core.pe` (they serve build gates too — they import core, they
  don't move).
- **P4:** prune dead surface: drop `symbol`/`def` (0 uses); keep
  `refs`/`hover`/`rename` under the clangd module. Update matcher/classifier
  briefs + build-system.md.

Each phase is one integrable commit, gated on: full `gruntz build` green
(score byte-identical — sema never touches matching), every subcommand
smoke-tested, `compileall` clean.

## Open questions (decide at review)

1. `class`/`classof` naming: keep the `class` subcommand name; is `classof.py`
   the right module name, or fold vtable+classof into one `vtables.py`?
2. Does batch mode need machine framing (`--json` per answer) for agents, or
   is the echoed-header text enough?
3. `exe_map`: leave in `analysis/` (campaign-heavy, docs/exe-map suite) with
   `sema/map.py` importing it in-process — or absorb its query half into core?
4. Retire `sema rename` in favor of the harness LSP? 79 uses says keep for now.
