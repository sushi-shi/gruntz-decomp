---
name: permute
description: Use when a Gruntz function is a COMPLETE, correct reconstruction (right types, right control-flow shape) but plateaus below 100% on MSVC /O2 codegen residue - operand-load order, a spill/materialization point, instruction scheduling. Trigger phrases: "climb to 100%", "hit a wall / plateau", "permute", "source permutation", "objdiff won't reach 100", "@early-stop but structure is correct". Runs a semantics-preserving source-permutation hill-climber (real wine cl + objdiff-cli). NOT for wrong structure/types/control-flow - fix those by hand first.
version: 0.1.0
---

# Permuter — climb a correct reconstruction to 100%

A source-permutation hill-climber for the MSVC 5.0 `/O2` codegen wall. It applies
**semantics-preserving** text mutations to the real `.cpp` (commutative-operand
swaps, independent-line reorders, additive reassociation, decl splits), recompiles
each variant with the REAL `wine cl`, scores the COFF against the delinked retail
target with `objdiff-cli`, and keeps improvements. Every mutation is value-preserving
by construction, so a higher byte-score can never come from a wrong program.

## When to use it (and when NOT)

USE it when a function is **already the right shape** — correct types (no casts/views
standing in for real classes), correct control flow, correct call/EH conventions —
and the only thing between it and 100% is codegen ordering the source can't directly
pin. This is the automated form of the by-hand spelling chase.

Do **NOT** reach for it to paper over a wrong reconstruction. It cannot fix:
- a **control-flow-shape** mismatch (that needs the real `for`/`goto`/`while`/early-exit
  restructuring done by hand — the permuter does not restructure control flow), or
- **wrong types / a cast-hacked view** (fix the class model first — matcher.md rule 0), or
- the core **register-coloring** residue (which physical reg holds a value) on `/O2`.

On `/O2` (Gruntz's `base` profile) gains are usually **incremental** — treat it as a
nudge that finishes operand-order / materialization residuals, not a guaranteed closer.

## THE methodology: top-down in source order

**MSVC /O2 codegen can interact across a translation unit.** Permute a TU's functions
**from the top of the source downwards** — lock in each function before the ones below
it. If you permute a lower function first and then change an upper one, the upper
change can shift the lower function's codegen and invalidate the win. `permute_sweep`
enforces this ordering; when driving `permute` by hand, honor it yourself.

## Tools

Run inside `nix develop .#build`, from the repo/worktree root, AFTER a `gruntz build`
(so `build/objdiff/{target/*.c.obj, report.json}` and `build/gen/symbol_names.csv`
are current). The `<unit>` is the `unit` stem from `config/units.toml`; the
`<mangled-sym>` is the exact mangled name (from the objdiff row / `llvm-nm` the base obj).

- **One function:**
  ```
  python3 -m gruntz.match.permute <src.cpp> <unit> <mangled-sym> [iters]
  # e.g.
  python3 -m gruntz.match.permute src/Gruntz/GameLevel.cpp gamelevel \
      '?ProbeFootBlocked@CGameLevel@@AAEHPAUCGameObject@@H@Z' 400
  ```
  Edits `<src.cpp>` IN PLACE, leaving the best-scoring variant (prints `start`/`FINAL`).
  Interrupt-safe: a `finally` always writes back the best-known variant (never worse
  than the original).

- **A whole unit, top-down (preferred):**
  ```
  python3 -m gruntz.match.permute_sweep <unit> [iters]
  # e.g.
  python3 -m gruntz.match.permute_sweep gamelevel 60
  ```
  Discovers the unit's `<100%` functions in SOURCE order and permutes each in turn,
  accumulating wins. Prints a per-function `before -> after` line and a WIN summary.

## After a run

`git diff` the source, then **rebuild and confirm** (`gruntz build`) before committing —
the permuter's own score is a fast objdiff read, but the build is the source of truth
and re-checks the whole unit. Commit the wins; each is matching-positive and
semantics-preserving. If a function did not move, that residual is a genuine wall
(regalloc-coloring or a control-flow shape) — leave the `@early-stop` with its
byte-level reason.

## How it works (two seams that matter)

- **Scoring is per-symbol.** `objdiff-cli` ignores its `<symbol>` arg for JSON output
  and emits every symbol in the TU; the tool pulls the TARGET symbol's `match_percent`
  by exact mangled name. (A global max would saturate at 100 the instant any sibling
  is 100% — and every real Gruntz TU has 100% siblings.)
- **Mutations are scoped** to the target function's `RVA()`-marker span (via
  `build/gen/symbol_names.csv`), so a run is fast and **cannot regress a 100% sibling**
  (a stacked mutation otherwise could, since scoring only reads the target).

Ported from the HoMM2 sibling decomp (same units.toml + cc_wrap + objdiff-cli pipeline).
