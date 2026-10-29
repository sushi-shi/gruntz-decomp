# Pattern sweeps

`gruntz walls patterns <sweep>` (`scripts/gruntz/walls/patterns.py`) turns a
compiler mechanism that once closed a wall into a candidate list over the whole
image. Each sweep reads the normalized comparison pair objdiff scored
(`build/objdiff/compare-new/{base,target}`), the MAX/HIST ledger
(`config/match_baseline.tsv`) and the unit profiles (`config/units.toml`). It
edits nothing. Run `gruntz build` first; a stale pair answers for the old source.

| Command | Mechanism | Signal (ours vs retail) |
| --- | --- | --- |
| `cc` | calling convention: explicit keyword or `WINAPI`-style macro, static vs non-static member, member vs free function | bytes each `ret` pops and ecx/edx live on entry; stack each paired call site releases after the call |
| `flags` | per-profile x87 shape | frame-slot store/reload pairs (the `/Op` shape), inline `fabs` vs `_fabs` calls, `__ftol` calls; counts per profile and unit |
| `flags --try UNITS --drop F --add F` | profile A/B control | compiles the units with the changed profile, normalizes and scores them as the build does, lists every function whose score moves |
| `inline` | `/Ob1` expansion | per-callee call/expand asymmetries; callee substitutions at aligned call sites |
| `loops` | loop rotation | conditional vs unconditional back edges |
| `ctors` | construction shape behind store order | same-base store permutations, ranked by the share of the difference they explain |

Common options: `--unit U`, `--limit N` (rows per list, `0` for all), `--json`.
Only functions with a ledger row are read: an unscored name can pair two
unrelated bodies. Rows carry the retail RVA and the current MAX.

## Reading the lists

- **Decoding.** Bodies are decoded from offset 0 and from the case targets of
  the function's own jump tables. cl 5.0 emits every table after the code, so
  the code ends at the lowest table a reached instruction indexes; table bytes
  are never read as instructions.
- **`cc`.** An entry-signature row is a declaration question; settle it from
  callers, the vtable slot and mangling (`gruntz sema`). A callee row whose
  sites mostly agree is the deferred, merged `add esp` of consecutive calls
  (a call-schedule residue), not a convention.
- **`inline`.** `+N` means we call where retail expands; `-N` means retail
  calls where we expand. On cl 5.0, `/O2` expands declared-inline bodies and
  instantiated template members (`gruntz walls inline-model`). A callee whose
  asymmetry recurs across callers points at its declaration; one caller with
  many callees points at that caller's budget. A substitution row (ours calls
  X where retail calls Y at the aligned site) is an identity or object-type
  lead, such as a wrong accessor, the base-class constructor of the object
  retail builds, or a CRT alias, and is subtracted from the asymmetry list.
  Recursion is left out: the delinked target resolves a self-call without a
  relocation.
- **`loops`.** `retail rotates, ours does not` usually means retail's source
  is a `do`/`while` whose test the compiler duplicated, or ours is one it
  could not duplicate.
- **`ctors`.** The same swap in several functions is one shared inline body
  or constructor, not several walls.

## Profile controls

`flags --try` is the same-source control that
[compiler profiles](compiler-flags.md) asks for before a profile change:

```sh
gruntz walls patterns flags --try spotlight,imagepolyclip --add /Op
gruntz walls patterns flags --profile cpp-noeh --add /GX
```

`--profile NAME` adds every unit of that profile. Objects, normalized copies
and the report go to `build/patterns/flags/`; the build's own objects and
report are untouched. Moves are measured against the current
`build/objdiff/compare-new/report.json`, so build first. A control with a
no-op flag (`--add /nologo`) must move nothing. The control proposes; a profile
change still goes through `config/units.toml` with the evidence that rule
names.
