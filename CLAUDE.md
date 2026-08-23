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

## The Wall Campaign

- The easy matches are drained: what remains of the matching objective IS the
  walls. The worklist is DERIVED, never hand-kept:
  `gruntz walls inventory` joins the current compare report, the Model, and
  `config/match_baseline.tsv`, ordered by ascending historical MAX. Work the
  lowest bank first; do not preferentially select easy high-score rows while
  lower structural mismatches remain.
- Classify before touching source: `gruntz walls diagnose <rva|name>` reads
  the normalized base/target pair and names the FIRST divergence class -
  referent (masked bytes identical, relocation targets differ: an identity
  question, fix the claim), inline/call-set (call-target multisets differ),
  CFG (branch/return skeleton differs: a structural reconstruction problem),
  then regalloc/scheduling (same calls and skeleton: instruction selection,
  lifetime, or allocation).
- Per class, the proven levers:
  * inline/call-set: `gruntz walls inline-model --gap <rva>` FIRST screens
    /Ob1 CANDIDACY, which decides whether the budget question applies at
    all: `/O2` implies `/Ob1`, so a callee that is an UNDEFINED external in
    our own base obj cannot be expanded at ANY budget, and a call-set delta
    naming one is a DUPLICATED CALL SITE - a tail merge - not a starved
    inline (PlaceObjectFull 0x78a50: `LoadCursorSprites` target 14 / base
    15, not a candidate; the fix was the caller's block layout, 86.41 ->
    90.84). Only for a callee this TU emits a COMDAT for does the budget
    arithmetic mean anything, and there `--measure-cb` titrates it with the
    real compiler. The verb refuses to invent `cb`: a guessed deficit
    printed as model output is indistinguishable from a measured one.
  * front-end TU-state: the cl 5.0 IL tap (capture `/d1il`, feed `/d2il`;
    recipe and normalization in the tu-state-probe pattern's quantified
    section) - an inert-source A/B whose IL differs is C1 handle state, and
    the probe-kind stride table (typedef +1 ... class +11) is the steering
    lever.
  * regalloc: `{EAX,ECX,EDX,ESI,EDI,EBX,EBP}` is a ROTATION order read from
    a persistent cursor, reset per basic block; one register is picked per
    IL tuple. Call-crossing values bind ESI, EDI, EBX, EBP in DEFINITION
    order and post-call USE order is inert (the older use-order lever is
    falsified - its probe pair confounded term order with coefficients).
    The catalogue of allocation, spill, selection and schedule reasons, each
    with a LEVER/PARTIAL/PARK verdict and a detection signature, is
    docs/relevations/wall-reasons-allocation.md.
- Levers are applied as disposable A/B tests. Never retain unused includes,
  declarations, fake locals, manual `STATE` probes, volatile carriers, or
  source distortions to steer codegen. Blind permutation search stays
  removed: walls are broken by understood levers, not ground.
- EXPLORATORY DESCENT (user ruling 2026-08-22): a single-lever dip is not a
  falsification of the path - it may be the right BASE for a second lever.
  When a spelling drops the score but moves the codegen TOWARD retail's
  texture (structure, addressing shape, register roles), keep it applied,
  diff the DIPPED state against retail, and compose the next lever on top;
  iterate a few levels before concluding. A % drop is never problematic
  while it is exploratory. The MAX gate governs what is COMMITTED, not what
  may be tried mid-session: the final kept state must be humane source
  (no-sane-dev test) and either >= the bank or an adjudicated, documented
  keep. One-step hill-climbing that reverts at the first dip prunes every
  composed path and only finds local maxima.
- BASELINE-DELTA CHECK (2026-08-23): before composing levers toward a
  structural feature you read off a DIPPED state, confirm that feature is
  actually ABSENT from the baseline. Blit1624 cost four composed levers chasing
  retail's spilled byte temp (`mov BYTE PTR [esp+0x1c],cl` / `mov bl,...`)
  that the 95.15 baseline was already emitting identically - the feature was
  read from the dipped disassembly and assumed new. Diff the FEATURE against
  the baseline, not just the dip against retail; a dip is only a base when it
  moves something the baseline did not already have.
- INLINE/MACRO PRIOR (user ruling 2026-08-22): the era devs DID write inline
  functions and macros, so an inline/macro spelling is a priori MORE likely to
  be the real source than a hand-expanded transcription. We may still overrule
  it, but only with evidence, and the question stays OPEN rather than settled.
  The resolution rule that follows: when a candidate spelling scores LOWER than
  the one in the tree, "ours wins" is decisive ONLY if ours is at 100%. If
  BOTH are below 100 neither is proven, so do not stop at the higher number -
  take the inline/macro form as a BASE and compose further levers on it (see
  EXPLORATORY DESCENT above); it may be the shape that reaches 100 while the
  higher-scoring transcription is a local maximum. Record which base you
  explored from, so a later session does not redo it.
- Historical MAX is banked only by a real build against the same per-function
  source fingerprint. If an unchanged function reaches exact under a
  disposable TU-state experiment, bank while exact, remove the experiment,
  rebuild, and keep the proof. Do not call a current dip a regression while
  the MAX gate remains green.
- A broken wall's reusable mechanism goes to `docs/patterns/` (+ INDEX) with
  the controlled A/B evidence and detection signature; a genuinely bounded
  wall keeps its state in the derived inventory (the report and the MAX
  ledger), never in a hand-kept ledger file or C++ comments.

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
- Platform preludes come from four headers and nothing else - never an
  `<afx*.h>` or `<windows.h>` directly. `<Win32.h>` is the pure Win32/DirectX
  root; `<Mfc.h>` is the MFC root, and `<MfcWin.h>` (the `<afxwin.h>` surface)
  and `<MfcNoInline.h>` (MFC's accessors parsed OUT OF LINE, a per-TU codegen
  device) are supersets that pull `<Mfc.h>` themselves. The two MFC roots are
  mutually exclusive with `<Win32.h>` as a TU's first include. Their relative
  order is a real contract, not a style: `<Mfc.h>` then `<MfcNoInline.h>` then
  `<MfcWin.h>`, because `_AFX_ENABLE_INLINES` must be defined by `<afx.h>`
  before it can be undefined and `<afxwin.h>` must be parsed after that.
- Do not hand-roll Windows typedefs, imports, or calling conventions: take the
  SDK's own declaration. Where cl 5.0 provably cannot take the SDK header,
  state the measurement at the declaration instead of inventing a spelling.
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
  `gruntz sema class <Class>` (slot-by-slot, every vtable the class holds):
  inherited slots are not redeclared, overrides use `OVERRIDE`, and new slots
  are plain `virtual`. Never pad a vtable with dummy virtual methods.
- Define a symbol or label only when evidence supports it. Never add fake code,
  storage, aliases, or padding solely to improve objdiff or final RVA layout.

## Address Annotations And State Markers

- Address labels live in `include/rva.h` macros.
- Never bind volatile compiler ordinals such as `_$E<n>` with
  `RVA_COMPGEN`; their suffix is emission-order state, not semantic identity.
  A `$E` dynamic-init helper is pinned at its OWNER instead:
  `RVA_DYNINIT(rva, size, owner)` on the owning datum's definition line
  (`gruntz labels` scrapes the pins into the `src_dyninit` channel; the current
  build's ordinal is derived from the emitting obj when needed, never stored).
- Compiler-generated data follows ONE rule: identity comes from the automatic
  oracles, and a pin exists only where they cannot reach. Use-site literals
  (pooled `??_C@` strings, `$T` FP-pool constants) are written bare — the
  string content oracle and the retail-reloc FP oracle re-prove them every
  build. A `DATA_COMPGEN(rva, value)` wrap is kept only for an ambiguous
  string payload or an FP slot with no reloc-corroborated referrer — all 17
  current pins are measured load-bearing. The pins extract as the
  `src_data_compgen` channel: a pin is admitted only when the TU's own base
  obj emitted that exact payload AND the retail image holds those bytes at
  the pinned address; a site that fails to bind is FATAL. Removing a
  load-bearing pin is adjudicated by compare (the identity degrades to a
  `$gap_`/`DAT_` referent and the referencing functions' scores dip).
  Separately — and disjointly —
  `config/retail/data_compgen.tsv` is a manifest, not a macro, in two classes:
  `class=common` names the COFF COMMONs cl emits from a header-inline's local
  static (plus the `??_B` guard byte beside it, which has no source spelling at
  all) — no owning TU exists, so only the retail address is stated and
  `gruntz delink` re-proves the rest against the base objs' COMMON tables
  (a row with no emitting base obj is an error);
  `class=copy` names the per-TU copies of header statics (the GruntDirStatics
  device), whose owner is the emitting TU. Details:
  `docs/data-attribution.md` §3b-iii.

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
