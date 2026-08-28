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
  * inline/call-set: `gruntz walls inline-model --gap <rva>` FIRST names the
    differing callees and reports the base object's symbol evidence. `/O2`
    implies `/Ob1`, so an unmarked function cannot expand; however, an
    UNDEFINED or absent COFF symbol does NOT prove the body was unavailable.
    Header inlines can expand at one site while a nested or declined site
    remains external, and delinking can also erase the provider distinction.
    A locally defined COMDAT positively proves inline visibility; otherwise
    inspect the source declaration, `/Ob0` census, nested helper boundary, and
    ordered call-site topology before choosing budget versus duplicated-site
    work. PlaceObjectFull 0x78a50's `LoadCursorSprites` delta was proved to be
    a tail merge by its caller block layout, not by the undefined symbol.
    After candidacy is independently established, `--measure-cb` titrates it
    with the real compiler. The verb refuses to invent `cb`: a guessed deficit
    printed as model output is indistinguishable from a measured one.
  * front-end TU-state: the cl 5.0 IL tap (capture `/d1il`, feed `/d2il`;
    recipe and normalization in the tu-state-probe pattern's quantified
    section) - an inert-source A/B whose IL differs is C1 handle state, and
    the probe-kind stride table (typedef +1 ... class +11) is the steering
    lever.
  * regalloc: one piece is proven on cl 5.0's `c2.exe` - the preference
    table `{EAX,ECX,EDX,ESI,EDI,EBX,EBP}` is present, and the first
    call-crossing value USED after the call takes EBX; reorder that value's
    first post-call use to steer the pick
    (docs/relevations/cl5-callcrossing-ebx-first-by-use-schedule.md).
- Levers are applied as disposable A/B tests. Never retain unused includes,
  declarations, fake locals, manual `STATE` probes, volatile carriers, or
  source distortions to steer codegen. Blind random hill-climbing stays
  removed. `gruntz permute state|variants` is a bounded evidence generator:
  the public command first requires a regalloc/scheduling diagnosis and a
  historical MAX below 100; variants are deterministic, source-hash scoped,
  syntax-aware or exact-span reviewed, and stop at audited exact closure.
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
- CROSS-PROJECT SOURCE-SHAPE PRIOR (HoMM3 debug-symbol campaign, 2026-08-28):
  debug locals, scopes, line groups, and authentic source from a sibling
  contemporary MSVC project proved that many apparent regalloc walls were
  earlier source-shape defects. Gruntz has no equivalent symbol corpus, but
  those wins define a bounded checklist of hypotheses to test before declaring
  a residue irreducible:
  * recover exact parameter and local storage widths across the complete call
    family; in particular, distinguish `u8` from `bool` and do not insert bool
    normalization when dirty upper bytes or the callee ABI support a byte.
    Recover cv/ref and overload boundaries across the family too: a const
    source-facing wrapper, by-value argument, const-reference result, or
    receiver-qualified overload can be folded completely while still deciding
    the caller's temporaries and evaluation order. The emitted implementation
    ABI alone does not disprove such an inline adapter;
  * test the authentic local census and lifetimes: distinct old/new/result
    locals, deliberate parameter or local reuse, overwrite versus a new result,
    declaration and first-use order, removal of unjustified cached member
    locals, and pointer/base locals used by symmetric blocks. Treat scope
    topology as separate evidence: multiple sibling block locals can share one
    retail stack slot while one reused function-scope local stays live and
    enlarges the frame. Also test whether initialization belongs in a `for`
    header rather than at the declaration;
  * preserve abstraction boundaries before transcribing their bodies: inline
    helpers/macros, accessors, constructors, by-value min/max or selectors, and
    same-TU candidates all change statement count, pseudo-register lifetime,
    EH state, and the inliner candidate set even when their arithmetic is
    equivalent. This includes one-field setters/getters and const forwarding
    wrappers whose entire machine body disappears. Assignment order inside an
    expanded helper can also recolor caller locals and move the first
    divergence to before the expansion; after every helper-order A/B compare
    from the function's first real divergence, and use semantic/member-layout
    order before transcribing emitted stores. Do not replace an attested helper
    call with its arithmetic merely because the expansion is obvious: the call
    boundary can create the retail FP or integer temporary homes;
  * test source statement grouping and evaluation order: ternary versus split
    `if`, one expression versus sequenced assignments, constructor/member-init
    order, stores before ownership changes, independent operand order, and one
    reused result local passed through successive setters versus direct member
    assignments. An explicit state flag plus one shared exit tail is a distinct
    source shape from duplicated early-return arms even when both implement the
    same logic. For symmetric arms, reproduce equal-value store order in BOTH
    arms: changing only one can prevent VC5's cross-jumper from merging the
    common tail;
  * test loop and exit spelling, including post-decrement/count-down loops,
    `while (1)` plus `break`, separate `continue` paths, `goto` into a shared
    guard or exit, and duplicated symmetric arms whose textual statement order
    enables a retail tail merge. Read comparison order before choosing the
    construct: non-value-order tests can prove an `if` chain where a compiler
    would sort a `switch`, and equivalent arms may intentionally use different
    statement orders;
  * when equal operations appear in a different order, consider repeated
    inline-helper calls or source-line groups rather than assuming a scheduler
    permutation; statement count can also move unrelated inline-budget edges;
  * preserve semantic identity as well as layout. Two same-shaped help tables,
    fields, accessors, or folded overloads are not interchangeable merely
    because the masked bytes agree. Ordered relocations, addends, consumers,
    and source ownership decide whether they are the same entity;
  * when an older source oracle and retail assign an operation to different
    layers, inspect the caller/callee pair before rejecting the oracle. A later
    revision can move work from an inner handler to its outer dispatcher, or
    replace one field/accessor while retaining the surrounding source shape.
    Keep the shared positive structure and take the moved operation from retail;
    absence in the older body is never negative evidence for retail.
  The first bounded HoMM3 pass closed functions through four independent facts:
  a helper boundary plus its store order, the original local census/lifetimes,
  a post-decrement loop statement, and a byte parameter propagated across its
  call boundary. The follow-up closures added an explicit exit-state carrier
  and shared tail, const forwarding/accessor boundaries, separate FP/result
  temporaries, and source-visible setter/min-max statements. One authentic
  abstraction restoration deliberately moved a 99.987% local maximum down to
  95.88% before composition reached 100%; this is direct evidence for applying
  EXPLORATORY DESCENT and the INLINE/MACRO PRIOR together, not treating the
  first score dip as rejection.
  These are hypotheses, not imported ground truth. SH4/Dreamcast instruction
  order, an absent call, VC6 register choices, STL internals, EH lowering, and
  `/Ob2` budget behavior do not transfer directly to x86 VC5. Revision-skewed
  source may also be older than retail. Use such evidence in its positive
  direction to propose an A/B, then let Gruntz retail instructions, relocations,
  and the pinned VC5 build decide. Diagnose the inliner before attributing its
  downstream register texture, but apply caller-size/budget reasoning only to
  a callee proven eligible under Gruntz's `/Ob1` model. Pair call sets by
  resolved target identity rather than raw synthetic labels; equal call totals
  with reciprocal unmatched aliases indicate an attribution defect, not two
  opposite inline decisions. After the checklist is genuinely exhausted,
  a same-call-set/same-CFG residue is a useful stop signal rather than a reason
  for unbounded spelling churn.
- SURVIVING SOURCE LINEAGE PRIOR (LithTech RezMgr/Bute audit, 2026-08-28):
  a public sibling revision can recover source layers that retail bytes alone
  underdetermine, but it is a positive-only oracle. Audit that repository's own
  history before importing anything: the public `libs/rezmgr` and `libs/lith`
  implementations were imported once and left source-unchanged, while ButeMgr
  later received portability edits to stream-size types and casts. Separate the
  historical import from those edits.
  * repeated complete-layout evidence outranks a layout-compatible shortcut.
    The surviving hash hierarchy proved that `CHashElement` ends after its
    parent/bin fields and each typed derived node owns the payload pointer at
    base+0x14. A union in the base preserved every 0x18-byte complete-node size
    but erased the real layer. Restoring typed nodes/tables, their inline
    setters/getters and typed `First`/`Insert`/`Delete` wrappers closed
    `CHashBase::Insert`, `CRezArchive::MergeArchive`,
    `CRezArchiveEntry::Initialize`, and `CRezArchive::AcquireEntry`;
  * source statement order is evidence even when C2 emits a different store
    order. The surviving header-to-member sequence closed the final residue in
    `CRezArchive::Open`. Separately, routing the type ctor through its authentic
    typed helper dropped it from 99.3548 to 95.6452; retaining that lower state
    and then adding the surviving authored order reached 100. This proves that
    an authentic lower-scoring compiler state can be the necessary BASE for the
    next independently evidenced fact. Never rank or revert such a path before
    bounded composition merely because its first step descends;
  * local census/shared tails, parameter reuse, and loop spelling transfer as
    hypotheses: the same lineage closed `CRezItm::Close`,
    `CRezArchiveEntry::Read`, and `CHashBase::Last`;
  * preserve the element type and authored index expression even when C2
    strength-reduces the loop to a byte induction variable. The surviving
    `dprintf.cpp` used an `unsigned short*` text buffer and element-indexed
    `for` loops; restoring that source closed the two final commuted-SIB walls
    in `MonoNewline` and `MonoClear`. A byte view plus manual byte offsets can
    match the arithmetic while hiding the source fact that selects the SIB;
  * never import negative absence or later additions. A later cursor-range
    guard changed Gruntz retail's CFG and was rejected; Bute's unsigned scanner
    byte was rejected by Gruntz's already-exact `char` family. Branch-specific
    derived locals that compile byte-flat remain structural evidence, not score
    evidence.
  * mine the public games and samples as well as the engine libraries. The
    byte-identical Shogo/Blood2 `NetStart_FillServiceList` copies recovered the
    authored loop alias, cursor-local census, and direct `LPARAM` casts for
    `CNetMgr::PopulateProviderList`; those facts are positive evidence even
    though the corrected Gruntz body is byte-flat. Repeated sibling copies are
    stronger source-shape evidence than one later engine implementation.
  * do not stop at the first related implementation. The later runtime PCX
    decoder supplied only a byte-type clue, while the overlooked 1996
    `libs/dibmgr::CDib::InitPcx` was the direct same-era ancestor. Its
    function-scope local census, reverse RLE fill, and direct three-plane
    indexing moved `CRezImage::DecodePcxData` from 97.6772 to exact extent and
    topology at 99.9240; target-adjacent C1 forest trial 4 then banked audited
    MAX 100 for that unchanged source with no probe retained. This falsifies
    the prior higher-scoring green/blue cursor interpretation: an emitted
    address sequence can reveal IL order without proving the named locals, and
    a surviving abstraction can be the base that escapes that local maximum.
  * exhaust a lineage-shaped branch as a bounded composition, not a spelling
    vote. For `CButeMgr::SetString`, one through five assignment-in-condition
    restorations were byte-flat, the sixth crossed to a lower C1 island, and
    adding the surviving `else` from that island returned to the baseline.
    With no intermediate state and the generated deleting-destructor cutoff
    unchanged, retain neither spelling and route to the next wall.
  The full controlled evidence and reverse-use procedure live in
  `docs/patterns/surviving-source-lineage-restores-typed-layers-and-order.md`.
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
