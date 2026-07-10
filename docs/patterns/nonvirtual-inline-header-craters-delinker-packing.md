# Non-virtual header-inline member craters includers (delinker per-unit packing)

**Context.** A scattered method that retail emits as a *deduped header-inline COMDAT*
(placed alone among uncarved gaps, called through incremental-link thunks — not packed
in its class's out-of-line `.obj` block) belongs, structurally, **inline in the class
header** (SPLIT_PLAN category D). For **virtual** inline members this works today: the
vtable forces emission in exactly ONE predictable TU (the class's ctor unit), so the
COMDAT's rva is attributed to that unit and the delinker packs it correctly (this is how
the ~61 existing header-inline cases — `GetTypeTag`, small accessors — work).

**Symptom.** Moving a **non-virtual, multi-caller** method into the header as an inline
member (removing it from its `.cpp`) does NOT just relocate it — it **craters unrelated
functions in the includer TUs** and LOSES the method itself. Observed moving
`CBrickzGrid::ComputeCellFlags` (0x77790, 893 B) from `Brickz.cpp` into `Brickz.h`:

    REGRESS RockBreakParticles ?BuildRockBreakParticles@CRockBreakMgr@@...  81.13 -> 0.00
    REGRESS TileGridCommand    ?ApplyMove@CTileGridCommand@@...             70.36 -> 0.00
    LOST    brickz             ?ComputeCellFlags@CBrickzGrid@@...           (was 0.00, absent)

**Cause.** A non-virtual inline member is emitted as a COMDAT in **every includer TU that
odr-uses it** (each caller: `RockBreakParticles::BuildRockBreak…`, `CTileGridCommand::
ApplyMove`, …). `labels.py` dedups the same-name rva to one `symbol_names.csv` row (correct)
but attributes it to the **emitting caller's unit** (keep-last by unit sort), not to the
declaring class's unit. The delinker then packs that unit's `RVA()` fns into ONE target
`.text` section (see [[delinker-packs-rva-fns-no-rename-owner]]); adding a far-off rva
(0x77790) to e.g. `rockbreakparticles` (real fns ~0x7b440) mis-packs the whole section,
cratering that unit's real functions to 0 — the same distant-rva packing crater as folding
`CNetSession2 -> CNetSession`.

**Verdict: `topic:wall` for now — DEFERRED on tooling, not a permanent wall.** The method
IS a header-inline member; keeping it out-of-line in the `.cpp` is a *reconstruction
constraint*, not a claim that retail was out-of-line (both spellings are byte-identical, so
this is match-neutral — do NOT document out-of-line as the original design; see
[[correctness-not-artifacts]]). The unblock is **delinker owner-attribution**: attribute an
inline COMDAT's rva to its *declaring class's* unit (from the mangled `?name@Class@@`, or a
designated-emitter annotation), not to whichever caller emitted it — then the delinker packs
0x77790 into `brickz` and the includers stay intact. Until that lands, leave such singletons
out-of-line in the class `.cpp` and record the DEFERRED migration.

**How to recognize the inline (vs out-of-line) singleton.** It sits ALONE among uncarved
gaps far from the class's contiguous out-of-line `.obj` block, is reached only via an
incremental-link thunk (or, for a virtual, only as a vtable-slot DATA ref), and has multiple
caller TUs. Contrast a genuine out-of-line member: forcing it `inline` makes MSVC5 inline it
into a single caller (it vanishes) — MSVC5 only auto-inlines inline-MARKED fns, so a retail
standalone-and-called fn is out-of-line and must stay in the `.cpp`.
