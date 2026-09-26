# Gruntz decompilation

Reconstruct the original C++ of **Gruntz** so MSVC 5.0 SP3 (`/O2 /MT`) emits
COFF matching retail `GRUNTZ.EXE`. Correct structure (classes, types, ownership,
storage, control flow, calling conventions, referents) outranks any score.

## Environment

- Work inside `nix develop`. Builds are per worktree: `export GRUNTZ_DIR=$PWD`.
- Never run or launch the game. Never use the Ghidra decompiler on
  `GRUNTZ.EXE`; read assembly, xrefs, RTTI, vtables, data, and relocations.
- `CLAUDE.md` is a symlink to this file; skills live in `.agents/skills/`
  (`.claude/skills` links there). Edit the canonical file only.

## Objective and scores

- Every function has three scores in `config/match_baseline.tsv`, always
  `CUR <= MAX <= HIST`:
  * CUR (`cur_pct`): the score at the latest bank.
  * MAX (`best_pct`): the best score of the function's current source hash.
    It only rises while that source is unchanged; editing the function resets
    it to the new CUR.
  * HIST (`hist_pct`): the all-time peak across every source revision; it
    never resets.
- The goal is MAX = 100 for every function. `HIST > MAX` means an earlier
  source matched better: a lost match to recover from Git history.
- A CUR dip with MAX held (TU-wide codegen perturbation of an unchanged
  function) is not a regression. Overall fuzzy and exact counts are
  navigation only.
- Raw instructions, constants, and ordered relocations decide correctness.
  Objdiff scores relocation targets strictly.

## Workflow

1. Pick work from `gruntz walls inventory --todo --limit N` (ascending HIST:
   never-matched functions first) or from the `HIST > MAX` rows (lost
   matches). Check `gruntz walls priors <rva>` for an existing verdict.
2. Classify with `gruntz walls diagnose <rva> --asm`: referent, then
   inline/call-set, then CFG, then register/schedule. Fix the earliest class.
3. Reconstruct with the `matcher` skill; classify plateaus with
   `wall-identifier`; use `gruntz permute` (the `permute` skill) only for a
   diagnosed register/schedule residue with MAX < 100.
4. Iterate with `gruntz match <unit|source>`: it compiles, labels, delinks,
   and compares only that TU (a few seconds), even after a header edit other
   TUs include, and reports MAX changes only: an edited function against the
   MAX it replaces, an unchanged one only if it beats its MAX. CUR dips of
   unchanged functions are not reported and need no attention. An edit that
   keeps CUR but lowers MAX through the new source hash is a `reset`: the loop
   records it in `docs/todos/syntactic-recovery.tsv` for a later pass; do not
   chase it while matching. Run
   `gruntz build` (every TU, no gates) when the change spans units.
5. Gates run only when preparing a merge: `gruntz build verify` (MAX gate plus
   the fast and normal tiers).
6. Mark a complete body whose residue is bounded by evidence `@early-stop`.

Matching rules that are easy to get wrong:

- Levers are disposable A/B experiments. Never keep probes, unused
  declarations, fake locals, volatile carriers, or distorted source.
- A score dip is not a rejection: if a change moves codegen toward retail's
  shape, keep it and compose the next lever on top. The MAX gate governs only
  what is committed.
- An inline function or macro is a likelier original spelling than a
  hand-expanded body; prefer it as the base unless evidence overrules it.
- Surviving LithTech source (revision `845119c`) is presumptively authentic.
  Adoption decisions live only in `config/lithtech_lineage.tsv`
  (`gruntz lineage`).
- If unchanged source reaches exact under a disposable TU-state experiment,
  bank it while exact, then remove the experiment.
- A 100% match is a match: keep it even if its source breaks a project rule.
  Admit a gate violation through that gate's allow entry, and record the
  function, the rule, and the deviation in `docs/todos/rule-exceptions.tsv`
  (schema in `docs/todos/README.md`). A rule-breaking spelling that retail's
  bytes require below 100% is recorded there too.

## Tests

- Matching and modeling work runs no test suites; compare is the
  verification, and the MAX gate runs at merge preparation.
- Tooling changes run only the touched package's `test_*.py`
  (`python3 -m unittest gruntz.<pkg>.test_<name>` from `scripts/`) and
  `gruntz verify selftest -k <gate>` for a changed gate.
- Do not add tests that re-check a function's bytes against retail.

## Source rules

- One class, one definition, in a shared header. No `.cpp`-local classes,
  layout views, or placeholder shells.
- Each function and global lives in its evidence-backed owner TU/header. No
  scattered `extern`s; no macro aliases onto hex names.
- Access goes through typed members. No raw offset casts, offset macros,
  casts of `this`, or C-style casts; use named casts only at real boundaries.
- Unclear identity: chase callers, storage, callees, mangling, vptr stores,
  RTTI, and offsets; else leave `@identity-TODO`. Never fabricate.
- Names are semantic: no address-derived names, compiler ordinals, or
  `local_10`-style names.
- Platform headers come only from `<Win32.h>`, `<Mfc.h>`, `<MfcNoInline.h>`,
  `<MfcWin.h>` (MFC order: `Mfc.h`, `MfcNoInline.h`, `MfcWin.h`; MFC roots and
  `Win32.h` are mutually exclusive). Never include `<afx*.h>` or
  `<windows.h>` directly or hand-roll SDK declarations.
- Use typed enums for proven numeric domains; retyping a parameter changes
  mangling.
- Prove aggregates (`RECT`/`CRect`, `Coord`/`POINT`) from whole-object use; do
  not split one object into overlapping globals.
- Vtables come mechanically from `gruntz sema class <Class>`: inherited slots
  are not redeclared, overrides use `OVERRIDE`, new slots are `virtual`. Never
  pad with dummy virtuals.
- No fake code, storage, labels, aliases, or padding to improve a score or
  final layout.

## Labels and data

- Address labels are `include/rva.h` macros. Pin `$E` dynamic-init helpers at
  their owner with `RVA_DYNINIT`; never bind compiler ordinals.
- `DATA(...)` records identity, not linker placement. Write pooled strings and
  FP constants bare; `DATA_COMPGEN` only where the oracles cannot reach.
  Header COMMONs and per-TU header-static copies go in
  `config/retail/data_compgen.tsv` (see `docs/data-attribution.md`).
- Never model an interior address as separate storage; refine the owner.
- Aggregate objdiff data percentages do not prove `.data`/`.bss` correctness.
- Marker vocabulary: `docs/comment-markers.md`.

## Repository hygiene

- Tool inputs in `config/`, generated output in ignored `build/`, history in
  Git. Docs describe current usage and contracts only: no diaries, score
  snapshots, or campaign logs.
- `docs/patterns/` is a small mechanism reference; follow its README.
- C++ comments are operational only: markers, ABI/codegen constraints,
  unsafe-seam explanations. No history, addresses, scores, or banners.
- Keep every gate green. Preserve concurrent changes; stage only your unit
  of work. Commit messages like `match: reconstruct CThing::Method` or
  `tools: verify relocation targets`. Never commit build state.
