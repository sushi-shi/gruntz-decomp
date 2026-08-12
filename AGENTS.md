## Objective And Authority

- Reconstruct the original C++ structure of **Gruntz** so that MSVC 5.0 SP3
  (`/O2 /MT`) emits COFF matching the retail `GRUNTZ.EXE`.
- The binary has no original PDB. Generated PDBs, delinked objects, inferred
  function boundaries, and contribution ranges are working models, not new
  ground truth.
- `src/` and `config/retail/` are used to label retail executable.
- Correct structure outranks a transient fuzzy score: real classes, types,
  ownership, storage, control flow, calling conventions, and relocation targets
  must not be distorted to protect a metric.
- The matching objective is **per-function MAX fuzzy = 100%**. MAX is derived
  for each function source hash.
- MAX gates current fuzzy per function hash. Overall fuzzy and exact-function
  totals are navigation only. Earlier TU declarations or definitions can rarely
  change VC5 register allocation and instruction scheduling. Expect these
  perturbations; do not distort correct source to restore a current score.
- `hist` is a retail function's highest observed fuzzy score across source
  hashes. It records known headroom, not proof of the current implementation.
- Raw instructions, constants, and ordered relocations decide whether a
  reconstruction is correct.
## Session And Evidence Discipline

- Work in the pinned `nix develop` environment.
- Inspect ownership, callers/callees, strings, types, and the retail disassembly
  before editing. Compare from the first real divergence after each build.
- Objdiff uses strict relocation scoring: target name/address, pointed-to data,
  and absolute DIR32 addends participate in the score. Linked-image referent
  audits remain authoritative for aliases, indirect calls, and final placement.
- Run a full `gruntz build` before hand-off or commit. A generated report
  alone is not authoritative.

## Historical-MAX Wall Campaign

- Work the lowest historical-MAX functions first, except rows already bounded
  by reproducible evidence in `wall-break.md`. Do not preferentially select easy
  high-score functions while lower structural mismatches remain.
- Classify a plateau in this order: inline/call-set divergence, CFG/branch
  divergence, then register allocation and scheduling. A branch-count mismatch
  is a structural reconstruction problem; matching branch sequences and return
  counts are evidence that the remaining wall is instruction selection,
  lifetime, or allocation.
- Use compiler-state or permutation experiments only as disposable A/B tests.
  Never retain unused includes, declarations, fake locals, manual `STATE`
  probes, volatile carriers, or source distortions to steer codegen. These are
  not appropriate while a function's structure is still unresolved and are
  rarely justified below 90%.
- Historical MAX is banked only by a real build against the same per-function
  source fingerprint. If an unchanged function reaches exact under a disposable
  TU-state experiment, update the ledger while exact, remove the experiment,
  rebuild, and keep the historical proof. Do not call a current dip a regression
  when the MAX gate remains green.
- Record every structural break and every genuinely bounded wall in
  `wall-break.md`: the before/after historical MAX, the retail evidence, the
  retained source lever, negative controls, and the remaining mismatch class.
  Keep investigation history there, not in C++ comments.
- Classify a plateau with the project `wall-identifier` skill
  (`gruntz sema diagnose <rva>`): inline/call-set → CFG → register →
  masked/referent, with only cl 5.0-proven levers. HoMM3's VC6 mechanics
  (register model, IL capture) stay hypothesis-only until re-proven here.
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
- Names describe semantics, not storage accidents: do not introduce address-
  derived identifiers, compiler ordinals, or contextless stack-slot names.
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
- Treat adjacent same-width scalars as a possible aggregate, not a conclusion.
  Four dwords used as one Win32 rectangle should be modeled as `RECT`/`CRect`;
  copied coordinate pairs should be modeled as `Coord`/`POINT`. Prove the type
  from complete-object calls, field order, copies, serialization, and stack or
  data extents. Do not split one retail object into overlapping globals, and do
  not invent an aggregate merely because it changes a score.
- For polymorphic classes, derive declarations mechanically from
  `gruntz sema class <Class>` or
  `python -m gruntz.core.vtable_hierarchy --class <Class>`:
  inherited slots are not redeclared, overrides use `OVERRIDE`, and new slots
  are plain `virtual`. Never pad a vtable with dummy virtual methods.
- Define a symbol or label only when evidence supports it. Never add fake code,
  storage, aliases, or padding solely to improve objdiff or final RVA layout.

## Address Annotations And State Markers

- Address labels live in `include/rva.h` macros, never comments. Let the
  label-style gate enforce mechanical spelling rather than duplicating it here.
- Never bind volatile compiler ordinals such as `_$E<n>` with
  `RVA_COMPGEN`; their suffix is emission-order state, not semantic identity.
  Keep observed RVA/name/size evidence in
  `config/retail/compiler-generated-functions.tsv` instead.
- `DATA_SYMBOL` is RETIRED and gone from `rva.h`; there is no declaration-only
  data pin. Every datum is a real C++ definition carrying `DATA(rva)`.
- The DATA analog of `RVA_COMPGEN` is therefore a manifest, not a macro:
  `config/retail/compiler-generated-data.tsv` names a datum cl emits as a COFF
  COMMON from a header-inline's local static (and the `??_B` guard byte beside
  it, which has no source spelling). It has no owning TU to host a source pin,
  so it states only the retail address; `gruntz.audit.compgen_data` re-proves
  the rest against the base objs and ratchets coverage.

- The marker vocabulary is closed by `docs/comment-markers.md`. `@early-stop`
  means a complete, evidence-bounded body, not missing logic or unresolved
  relocation work. Re-derive its residue instead of trusting an old source comment.

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
- Put a newly proven reusable MSVC idiom in `docs/patterns/` and its index rather
  than leaving it only in a source comment or commit message.
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
- Do not investigate ordinary current-score or exact-count movement caused by a
  correctness fix. Codegen perturbation is expected and unrelated functions do
  not impose a cost. Investigate only evidence of a substantive modeling error,
  a build failure, or a MAX-gate failure. A reproducible perturbation mechanism
  may be documented for later reverse use, but attribution is not a prerequisite
  for keeping or committing correct work.
- Documentation and green tests are claims, not authority. If retail evidence
  falsifies a documented tool contract, correct the documentation and add a
  negative or integration control that exercises the full path which failed;
  a recognizer-only test does not prove its consumer uses the result.
- Preserve user and concurrent changes. Do not revert unrelated edits, and
  stage only files belonging to the current unit of work.
- Keep C++ comments operational: machine-visible state markers, concise
  ABI/codegen constraints, interleaver/dead-code evidence, real fallthrough
  annotations, and explanations for unavoidable unsafe seams. Do not retain
  reconstruction history, address notes, score history, section banners, or
  prose that duplicates the code. Trailing include-guard labels such as
  `#endif // HEADER_GUARD` are allowed.
- Prefer focused commits such as `match: reconstruct CThing::Method` or
  `tools: verify relocation targets`. Do not commit generated build state.
