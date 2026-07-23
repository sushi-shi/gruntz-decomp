# Codex Guide

This file is the short, durable orientation. When changing reconstructed C++,
also read `.claude/agents/matcher.md`; it contains the detailed matching
doctrine. Do not add live scores, current assignments, queue snapshots, or
one-off campaign notes here.

## Objective And Authority

- Reconstruct the original C++ structure of **Gruntz** so that MSVC 5.0 SP3
  (`/O2 /MT`) emits COFF matching the retail `GRUNTZ.EXE`.
- `src/` is the source of truth. Retail bytes, relocations, RTTI, call graphs,
  and observed data layouts are the evidence used to improve it.
- Correct structure outranks a transient fuzzy score: real classes, types,
  ownership, storage, control flow, calling conventions, and relocation targets
  must not be distorted to protect a metric.
- Objdiff fuzzy percentage is a navigation signal, not proof. Raw instructions,
  constants, and ordered relocations decide whether a reconstruction is correct.
- The binary has no original PDB. Generated PDBs, delinked objects, inferred
  function boundaries, and contribution ranges are working models, not new
  ground truth.

## Repository Map

- `src/`: reconstructed C++, grouped by retail modules and translation units.
- `include/`: shared class and ABI headers; `rva.h` owns all address annotations.
- `scripts/gruntz/`: the only importable Python package and implementation of
  the build, semantic navigation, matching, cleanliness, permutation, and audit
  tools. Run modules as `python -m gruntz.<area>.<module>`.
- `config/units.toml`: translation-unit build manifest.
- `docs/patterns/`: indexed MSVC 5 code-generation idioms.
- `docs/domain/`: distilled game and WWD semantics.
- `vendor/`: verbatim third-party source. Read-only.
- `build/`: generated and fetched state.

## Environment And Front Doors

Use `nix develop`; it provides the pinned MSVC/Wine toolchain and analysis
utilities. The principal commands are:

```sh
gruntz init                    # cached Wine, clangd, and Ghidra setup
gruntz build                   # configure, compile, delink, diff, report, gates
gruntz build --fast            # iteration only; skips the gate tail
gruntz status                  # authoritative current match status
gruntz sema -h                 # semantic source/retail navigator
gruntz format                  # format reconstructed source and headers
gruntz sema xref 0x00... --callees # prefer to use xrefs for recovering structure
```

Builds are expected to be fast enough to run in the foreground. Use incremental
`gruntz build --fast` while iterating, but run a full `gruntz build` before
hand-off or commit. Do not trust a score copied into documentation or produced
by a bare objdiff report regeneration; the real build refreshes normalized
objects and the report.


## Investigation And Matching Loop

1. Inspect before editing. Start with the target's dossier, callers/callees,
   strings, vtable information, and target disassembly:

   ```sh
   gruntz sema rva 0x00......
   gruntz sema xref 0x00...... --callees
   gruntz sema strings 0x00......
   gruntz sema class ClassName
   gruntz sema disasm 0x00...... --lite
   ```

2. Recover semantics and shape: the real owner TU, class hierarchy, member
   layout, types, calling convention, control flow, locals, and external
   referents. Use Ghidra only when the lower-level evidence does not make the
   structure clear.
3. Build, then compare from the first genuine divergence:

   ```sh
   gruntz build --fast
   gruntz sema disasm 0x00...... --diff --lite
   gruntz sema disasm 0x00...... --base
   gruntz sema disasm 0x00...... --rich --lite
   ```

4. Audit raw constants and relocations. Objdiff masks address-sized immediates,
   so `--diff` can conceal a wrong constant or target. A near match still needs
   semantic, byte, and relocation review.
5. Fix source-level causes before compiler steering: signedness, types, loop
   form, condition polarity, declaration scope/order, lifetime, aliasing,
   calling convention, and inline/out-of-line shape. Use permutation tools only
   after the reconstruction is structurally credible.
6. Finish with a full build, focused disassembly/relocation review, formatting,
   and `git diff --check`.

Use `rg` for lexical searches. Use `gruntz sema`, clangd/LSP, retail xrefs, and
RTTI for semantic claims such as ownership, identity, call relationships, types,
and vtable slots. RVA proximity alone does not prove TU ownership.

## Source Modeling Rules

- A class has one real definition in a shared header. Do not create `.cpp`-local
  classes, layout views, or placeholder shells to make an access compile.
- When a receiver's identity is unclear, chase both directions: callers and
  allocation/storage sites, plus callees, mangled signatures, vptr stores,
  vtable slots, RTTI, and member offsets. If the evidence remains insufficient,
  record an `@identity-TODO`; do not fabricate an identity.
- Put each function and global in its evidence-backed owner TU/header. Do not
  scatter per-TU `extern` declarations or alias semantic names onto hex names
  with macros.
- Model fields and relationships so access is expressed through real members.
  Raw offset casts and offset-access macros are forbidden. Casting `this` is a
  class-model defect, not a solution.
- Avoid C-style casts. Prefer correct types; when a conversion is genuinely
  required, use the appropriate C++ named cast. Preserve authentic SDK/ABI
  types at external boundaries.
- Use `<Mfc.h>` for MFC translation units and `<Win32.h>` for pure Win32/DirectX
  units. Do not hand-roll Windows typedefs, imports, or calling conventions.
- Use named, typed enums for proven numeric domains instead of magic macros.
  Enumerate only values supported by evidence. Changing a function parameter
  or return type to an enum changes MSVC mangling, so verify such signature
  changes deliberately.
- Preserve proven packed layouts, sizes, storage widths, and member offsets.
  Improve placeholder names when their meaning is established; never invent a
  name merely to reduce a cleanliness counter.
- For polymorphic classes, derive declarations mechanically from
  `gruntz sema class <Class>` or
  `python -m gruntz.core.vtable_hierarchy --class <Class>`:
  inherited slots are not redeclared, overrides use `OVERRIDE`, and new slots
  are plain `virtual`. Never pad a vtable with dummy virtual methods.
- Define a symbol or label only when evidence supports it. Never add fake code,
  storage, aliases, or padding solely to improve objdiff or final RVA layout.

## Address Annotations And State Markers

All bindings use macros from `include/rva.h`; address labels never live in
comments. Keep their enforced spelling:

- addresses are zero-padded eight-digit lowercase hex, such as `0x0008c750`;
- size arguments are unpadded lowercase hex; `0x0` means unknown;
- one annotation invocation per line;
- use the dedicated compiler-generated forms where applicable.
- Never bind volatile compiler ordinals such as `_$E<n>` with
  `RVA_COMPGEN`; their suffix is emission-order state, not semantic identity.
  Keep observed RVA/name/size evidence in
  `config/compiler-generated-functions.tsv` instead.
- A `$S*` `DATA_SYMBOL` is narrower: use it only for a real named source
  static whose exact emitted COFF symbol proves the semantic prefix and whose
  sole ambiguity is the unstable numeric suffix.

The machine-visible comment vocabulary is closed; see
`docs/comment-markers.md`. The common states are:

- `// @stub` with required confidence/source evidence: an empty unreconstructed
  body;
- `// @early-stop` followed by an assembly-level reason: a complete
  reconstruction parked below exact match;
- `// @identity-TODO`: identity or ownership remains unproven after an evidence
  chase.

Do not invent new `@` markers. A reconstructed method is normally exact and
unmarked, or complete with a justified `@early-stop`; the marker never excuses
missing logic or unresolved relocation work.

## Data, Generated Models, And Linking

- `DATA(...)` records semantic/audit identity; it does not force a linker
  address. Model the retail storage class, initializer, type, and owner.
- Delinked target data sections may be synthesized, duplicated, or zero-filled.
  Do not infer original `.data`/`.bss` membership or global data correctness
  solely from aggregate objdiff data percentages.
- Never model an interior address as overlapping independent storage. Refine the
  owning object and access its real member or table element.
- Final-image gaps are link-layout facts, not justification for giant padding
  arrays in reconstructed source.
- Changes to label annotations can regenerate the fake PDB and re-delink target
  objects. Treat resulting broad movement as something to inspect, not bypass.

## Quality And Change Discipline

- Keep every build gate green. Cleanliness work removes the underlying modeling
  debt rather than hiding its textual signature.
- Formatting is automated and matching-neutral. Run `gruntz format`; never
  format `vendor/`.
- Update `README.md` and the relevant durable documentation when commands,
  paths, build flow, or tool contracts change. Put a newly proven reusable MSVC
  idiom in `docs/patterns/` and its index rather than in an isolated comment.
- When a build refresh disproves matching doctrine, document both the failed
  assumption and the recognizable reverse-audit signature. Do not preserve an
  outdated explanation merely because an old cache or high-water score once
  appeared to support it.
- Treat a reproducible matching surprise as a matching pattern, including
  cross-function MSVC optimizer-state effects. Record the controlled A/B
  evidence, detection signature, and safe reverse-use heuristic in
  `docs/patterns/` plus `docs/patterns/INDEX.md`; correct older pattern claims
  that the new evidence falsifies. Do not leave this knowledge only in a source
  comment or commit message.
- When a correctness fix lowers a metric, identify the exact functions, data,
  or relocations that stopped pairing and document the reusable cause before
  committing. Treat that delta as a reverse-audit queue for similar hidden
  mismodelling; do not erase the evidence merely to restore MAX.
- Documentation and green tests are claims, not authority. If retail evidence
  falsifies a documented tool contract, correct the documentation and add a
  negative or integration control that exercises the full path which failed;
  a recognizer-only test does not prove its consumer uses the result.
- Preserve user and concurrent changes. Do not revert unrelated edits, and
  stage only files belonging to the current unit of work.
- Prefer focused commits such as `match: reconstruct CThing::Method` or
  `tools: verify relocation targets`. Do not commit generated build state.

## Focused References

- `docs/build-system.md`: CLI, pipeline, manifests, generated state, and clangd.
- `docs/gotchas.md`: measurement and build traps worth checking when results
  look impossible.
- `docs/matching-patterns.md` and `docs/patterns/INDEX.md`: MSVC 5 behavior and
  source-shape recipes.
- `docs/tu-partition-brief.md`: TU ownership and contribution evidence.
- `docs/cast-metric-policy.md`: typing and cast policy.
- `docs/comment-markers.md`: complete marker contract.
