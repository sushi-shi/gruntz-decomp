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
- Never run, launch, replay, or capture the game. Do not use the Ghidra
  decompiler on `GRUNTZ.EXE`; use static assembly, xrefs, RTTI, vtables, data,
  and relocations.

## Validation Cadence

- While iterating on source, `gruntz build base compare` compiles and compares
  without the gates. Treat `ninja 0.0s` as no verification.
- Run a full `gruntz build` (compile, compare, MAX gate, `fast,normal` gate
  tiers) before hand-off or commit. A generated report alone is not
  authoritative.
- Matching and modeling work runs NO test suites: the compare report and the
  MAX gate are the verification. Do not re-run `gruntz verify selftest` or
  package unit tests to re-certify functions or gates you did not change.
- Tooling work runs only the tests of what it touched: the package's own
  `test_*.py` (`python3 -m unittest gruntz.<package>.test_<name>` from
  `scripts/`), and `gruntz verify selftest -k <gate>` when a gate changed.
- Do not add tests that re-certify a function's bytes against retail. The
  compare report already measures every function on every build; a
  per-function test only duplicates it and goes stale.

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
    implies `/Ob1`, so an unmarked ordinary non-template function cannot
    expand. Instantiated template members are an exception: an out-of-class
    body without `inline` expands under `/Ob1` and stays a call under `/Ob0`
    (docs/patterns/vc5-template-members-inline-without-inline-keyword.md).
    An UNDEFINED or absent COFF symbol does NOT prove the body was
    unavailable: header inlines can expand at one site while a nested or
    declined site stays external, and delinking can erase the provider
    distinction. A locally defined COMDAT positively proves inline visibility;
    otherwise inspect the source declaration, `/Ob0` census, nested helper
    boundary, and ordered call-site topology before choosing budget versus
    duplicated-site (tail-merge) work. After candidacy is independently
    established, `--measure-cb` titrates it with the real compiler; the verb
    refuses to invent `cb`.
  * front-end TU-state: the cl 5.0 IL tap (capture `/d1il`, feed `/d2il`;
    see docs/patterns/tu-state-probe-family-decides-reachability.md). Compare
    controlled inputs and replay before attributing a difference to handle
    state. Historical probe strides are not universal compiler constants.
  * regalloc: test authentic widths, helper boundaries, locals, and lifetimes
    with controlled compiler inputs. Do not treat a register table or a
    first-post-call-use recipe as a universal source-order rule.
- Levers are applied as disposable A/B tests. Never retain unused includes,
  declarations, fake locals, manual `STATE` probes, volatile carriers, or
  source distortions to steer codegen. Blind random hill-climbing stays
  removed. `gruntz permute state|variants` is a bounded evidence generator:
  it first requires a regalloc/scheduling diagnosis and a historical MAX
  below 100; variants are deterministic, source-hash scoped, and stop at
  audited exact closure (docs/permuter.md).
- EXPLORATORY DESCENT (user ruling): a single-lever dip is not a
  falsification of the path - it may be the right BASE for a second lever.
  When a spelling drops the score but moves the codegen TOWARD retail's
  texture (structure, addressing shape, register roles), keep it applied,
  diff the DIPPED state against retail, and compose the next lever on top;
  iterate a few levels before concluding. The MAX gate governs what is
  COMMITTED, not what may be tried mid-session: the final kept state must be
  humane source (no-sane-dev test) and either >= the bank or an adjudicated,
  documented keep. One-step hill-climbing that reverts at the first dip only
  finds local maxima.
- BASELINE-DELTA CHECK: before composing levers toward a structural feature
  read off a DIPPED state, confirm that feature is actually ABSENT from the
  baseline. Diff the FEATURE against the baseline, not just the dip against
  retail; a dip is only a base when it moves something the baseline did not
  already have.
- INLINE/MACRO PRIOR (user ruling): the era devs DID write inline functions
  and macros, so an inline/macro spelling is a priori MORE likely to be the
  real source than a hand-expanded transcription. Overrule it only with
  evidence. When a candidate scores LOWER than the tree, "ours wins" is
  decisive ONLY if ours is at 100%. If both are below 100, take the
  inline/macro form as a BASE and compose further levers on it. Record which
  base you explored from, so a later session does not redo it.
- SOURCE-SHAPE CHECKLIST: many apparent regalloc walls are earlier
  source-shape defects. Before declaring a residue bounded, dispose of the
  applicable families in the matcher skill's lever catalog
  (`.agents/skills/matcher/references/levers.md`): storage widths and cv/ref
  boundaries across the call family, local census and lifetimes, helper and
  macro boundaries, statement grouping and evaluation order, loop and exit
  spelling, and semantic identity beyond masked bytes. Evidence from sibling
  projects (HoMM3, VC6, SH4, `/Ob2`) proposes A/Bs; Gruntz retail and the
  pinned VC5 build decide. A same-call-set/same-CFG residue after the
  checklist is exhausted is a stop signal, not a reason for unbounded churn.
- SURVIVING SOURCE LINEAGE PRIOR: the pinned public LithTech revision
  `845119c` is presumptively authentic source for matching Gruntz families.
  Adopt its complete owner, class, declaration, helper, local-census,
  statement-order, and loop layer unless retail instructions, ordered
  relocations, ABI/layout evidence, or the absence of a retail owner
  specifically disproves a fact. A lower first score is not a rejection: keep
  the sourced base and compose independently evidenced facts. ButeMgr
  portability commit `458a14f` is comparison evidence only.
  * `gruntz lineage discover|inventory|verify` and
    `config/lithtech_lineage.tsv` define the derived adoption queue. Every
    candidate becomes `take`, `take-adapted`, or `do-not-take`; retained
    divergences and their retail evidence live ONLY in that ledger.
  * Apply a surviving layer as a complete composition (types, unions,
    operators, helpers, macros, initializers, storage scope, names) before
    ranking its intermediate compiler states. Layout-compatible shortcuts
    (`void*`, overlays, flattened arrays) do not substitute for the typed
    family, and matching bytes do not justify invented identities when the
    surviving owner exists.
  * Mine games and samples as well as engine libraries; repeated sibling
    copies outrank one later body. Retail sibling binaries and paired
    Debug/Release objects can prove a shared family, widths, locals, and
    scopes, but never supply source text or negative absence
    (docs/patterns/cross-game-binary-oracle-proves-shared-source-family.md,
    docs/patterns/surviving-source-lineage-restores-typed-layers-and-order.md).
  * When an older source and retail place an operation in different layers,
    keep the shared structure and take the moved operation from retail;
    absence in the older body is never negative evidence for retail.
  * An authentic complete body invalidates a bounded review of a different
    source hash: reopen the diagnosis.
- Historical MAX is banked only by a real build against the same per-function
  source fingerprint. If an unchanged function reaches exact under a
  disposable TU-state experiment, bank while exact, remove the experiment,
  rebuild, and keep the proof. Do not call a current dip a regression while
  the MAX gate remains green.
- Keep `docs/patterns/` a small mechanism reference, not a wall diary. Follow
  its README admission rules; consolidate rather than add a file per closure.
  Function state belongs in the derived report/MAX inventory, not a pattern
  entry, hand-kept ledger, or reconstruction-history C++ comment.

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
  order is a real contract: `<Mfc.h>` then `<MfcNoInline.h>` then
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
  string payload or an FP slot with no reloc-corroborated referrer. The pins
  extract as the `src_data_compgen` channel: a pin is admitted only when the
  TU's own base obj emitted that exact payload AND the retail image holds
  those bytes at the pinned address; a site that fails to bind is FATAL.
  Removing a load-bearing pin is adjudicated by compare (the identity degrades
  to a `$gap_`/`DAT_` referent and the referencing functions' scores dip).
  Separately, `config/retail/data_compgen.tsv` is a manifest, not a macro, in
  two classes: `class=common` names the COFF COMMONs cl emits from a
  header-inline's local static (plus the `??_B` guard byte beside it) — only
  the retail address is stated and `gruntz delink` re-proves the rest against
  the base objs' COMMON tables; `class=copy` names the per-TU copies of header
  statics, whose owner is the emitting TU. Details: `docs/data-attribution.md`,
  "Header statics and COMMONs".
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

- Keep docs about ongoing usage and contracts. Tool inputs belong in `config/`,
  generated reports in ignored `build/`, and old investigations in Git history.
  Keep useful format diagrams beside parsers; do not duplicate field maps or
  maintain PR diaries, score snapshots, or mirrored references in `docs/`.
- This file is the one agent guide: `CLAUDE.md` is a symlink to it, and the
  skills live once in `.agents/skills/` (`.claude/skills` links there). Edit
  the canonical file, never a copy.
- Keep every build gate green. Cleanliness work removes the underlying modeling
  debt rather than hiding its textual signature.
- Update an existing compiler-pattern entry when new evidence changes its
  reusable mechanism. Add an entry only for a distinct, reproducible mechanism.
- When a build refresh disproves matching doctrine, document both the failed
  assumption and the recognizable reverse-audit signature. Do not preserve an
  outdated explanation merely because an old cache or high-water score once
  appeared to support it.
- A matching surprise is an observation, not automatically a compiler rule.
  Keep pattern entries short: signature, reproducible evidence, and limits.
  Remove falsified claims rather than accumulating contradictory addenda.
  Do not add score tables, campaign logs, confidence ratings, or universal
  impossibility claims from a finite search. Historical links are provenance,
  not current authority.
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
