# cl 5.0 does not cross-jump /GX epilogues in a TU's first function - a TU-boundary oracle

tags: cpp:eh cpp:struct msvc5:gx | asm:ret asm:jmp | topic:method topic:tooling topic:wall topic:codegen-idiom
symptoms: retail duplicates the full /GX return epilogue (restore fs:0, pops,
add esp, ret) at every `return` arm while our compile funnels the arms through
one shared exit (`jmp` + single ret); diagnose says CFG with target rets >
base rets; no source construct moves it
confidence: 9/10 for the validated direction; the mid-TU unmerged case is OPEN

## The titration (2026-08-22, real cl 5.0 SP3, /O2 /MT /GX)

A minimal CreateChildren-shaped probe (three `if (call()==0) { err-store;
return 0; }` guards after three `new` expressions) compiled ALONE in a TU
emits retail's UNMERGED shape - four full epilogues, four rets. Adding ANY
preceding emitted function body flips it to the merged single-exit shape:

| preceding construct                   | probe's rets |
|---------------------------------------|--------------|
| nothing (first body in TU)            | 4 (unmerged) |
| plain function body                   | 1 (merged)   |
| body with EH object                   | 1            |
| tiny one-ret body                     | 1            |
| COMDAT-emitted inline (address-taken) | 1            |
| multi-return body                     | 1            |
| class DECLARATION w/ inline members   | 4 (stays cold)|

Once warmed, no tested construct between bodies cools it again (class decl,
template def, #pragma pack, dynamic-init static, string table, inline+user).
The merge state is an optimizer-warmup fact, not a source fact.

## The oracle and its first production

A function whose RETAIL epilogues are unmerged while ours merge is evidence
the function OPENED its era compiland. `CDDrawSubMgrPages::CreateChildren`
(82.64, parked as a "merge coin") carried exactly this signature; splitting
DDrawSubMgr.cpp at 0x1588f0 into a DDrawSubMgrPages.cpp compiland made it
**100.00 EXACT byte-identical** and the new unit 39/40 exact at 99.90%. The
four CDDrawSubMgrPages symbols BELOW the boundary (IsLoaded/GetClassId/
??_G/??1) are header-inline vtable realizations owned by the realizing TU,
not counter-evidence.

Corroboration required before splitting: the boundary must be a class-family
boundary in the rva order, and the split must keep both files' plain blocks
ascending (the tu-order gate arbitrates).

Candidate generator (both directions): for every sub-100 unit opener, check
whether the rva-PREDECESSOR function belongs to the SAME CLASS in a different
unit - a one-liner over the baseline. The 2026-08-22 run listed 34 such
boundaries; the two with a cold/warm ret signature both paid (AdvanceAnim
+7.2 via the WormholeActs fold, FillCombo +9.4 via the LatencyList fold -
the latter also recovering the era MFC prelude). A fold WITHOUT a codegen
signature measured neutral twice (gruntbricklayerstep, battlezrepath
prelude) - do not churn boundaries the bytes do not corroborate.

## Second production

`CWwdGrid::Setup` (1 ret vs retail 2): the only plain body before it was the
empty `~CWwdGridShell`, whose retail copy sits in the COMDAT band - a header
inline. Moving the dtor into the header (emission deferred to the realizing
TU, gamelevelmove) flips Setup's skeleton to retail's: 92.14 -> 95.18 with
the residue regalloc; the dtor stays EXACT. The oracle's cheap A/B is a
disposable hoist-first-in-file; the era-faithful fix is whatever retail's
COMDAT placement says the preceding bodies really were.

## Falsified refinements (2026-08-22, keep these from being re-run)

* `$E` dyninit thunks do NOT set the warm state in era compiles: spotlight's
  retail `$E`s sit MID-TU (0xb1590, pinned RVA_DYNINIT rows) before Tick
  (0xb1af0) and retail Tick is still UNMERGED (3 rets). Consistently, moving
  `GruntDirStatics.h` to the file tail in battlezunitstep and sbi_imageset
  reorders the obj's `$E` emission but changes no scores. The r1/r3 probe
  asymmetry (static before/after the body flipping the merge) therefore has
  a different driver than the `$E` itself - unresolved.
* A cold start does NOT unmerge PARTIAL-merge rows (base N rets vs target
  N+1: SetupImage 3v4 compiled first-in-obj stays 3v4; Step@battlezunitstep
  4v5 likewise). The oracle's reach is the FULL-merge rows (base 1 ret vs
  target N).
* A hoist alone did not flip `AddLogic` (1v3) or `CSpotLight::Tick` (1v3);
  in Tick's case the template static `CActRegPool<CSpotLight>::s_table`
  kept `$E33` as the obj's first emission, so the hoist never produced a
  cold start - re-test those rows only with ALL earlier emissions removed
  or relocated to their retail positions.

## Refined map (second titration round)

* `$E` dyninit thunks are emitted AND compiled at the STATIC'S SOURCE
  POSITION (probe objs: before -> head, between -> middle, after -> tail),
  and they DO carry the warm state at that position (static-before-fn =
  merged, static-after-fn = cold). Template member statics with dynamic
  init behave identically (t2). The earlier "$Es don't warm" reading
  conflated era rva placement with compile order.
* The era contradiction narrows to a WINDOW: retail spotlight's Tick sits
  AFTER the unit's `$E`s in rva order (warm by the position rule) yet keeps
  3 rets - PARTIAL merging. Ours merges the same function to 1 ret. The
  remaining unmapped factor is why the two warm compiles merge to different
  DEGREES - a cross-jump distance/window input (retail AddLogic keeps two
  IDENTICAL return-0 epilogues 0x1f1 bytes apart that ours folds).
  Arm-distance titrated: NO window - a warmed probe still fully merges with
  456 insns between the arms. And ours is not always-merge either
  (battlezunitstep Step keeps 4 rets warm). The residual is a per-PAIR
  merge predicate shared by both compilers with one unmapped input;
  FOUND (third round): a WARMED probe whose arms return at DIFFERENT EH
  STATES (an extra destructible object live at one arm) keeps separate
  epilogues (e1/e2: 3 rets warm). cl cross-jumps return epilogues only
  between returns at the SAME EH state. So warm-side merge-degree deltas
  reduce to EH-STATE-MAP deltas - the ctor-inlining axis `walls ehactions`
  reads. Caveat from the first application: AddLogic's state MAPS match
  (4/4 funclets, same actions) and only the cleanup SLOT differs (ours
  tracks the new-result in the dead arg home [ebp+0x8], retail in a
  dedicated local [ebp-0x70]); per-arm derived-type temps did not move it.
  The slot/home assignment is the still-unmapped residue for such rows.

## Adjacent bound: the sret-slot schedule (GetTilePos, 2026-08-22)

`CGrunt::GetTilePos` (29 B, 85.70) isolates a second slot/home input: retail
loads the sret pointer at insn 2, binds it in edx for the whole body and
returns it through a final `mov eax,edx`; ours loads it late straight into
eax. Six spellings measured inert or worse (field-assign, h-local, &out
pointer local, Set-returning-this, void-Set, GetScreenPos+shift - the last
emits a real call, 10.40, because era GetScreenPos is a PLAIN out-of-line
function at 0x29a50 in battlezmapconfig, ruling out cross-TU inlining).
Probe TU reproduces ours cold AND warm - not a TU-state input. Same residue
family as AddLogic's EH cleanup home ([ebp+0x8] vs [ebp-0x70]): cl's
slot/home assignment carries one more unmapped input.

## Round 4: the real-TU warm driver is still unidentified

`CGrunt::ChargeStep` isolates it: retail keeps TEN identical 4-insn
epilogues (fully cold, +30 insns - the whole 83.91 residue's skeleton half),
ours merges them even when ChargeStep is the obj's FIRST emission with the
`$E`s tailed. The probe stays COLD under every axis tried: the real MFC
prelude (Mfc/MfcNoInline/MfcWin), /GR, 50/200/800 parsed-but-unused inline
bodies, statics/templates in any position. Identical cflags
(`/O2 /MT /GX`). RESOLVED (round 5): the missing warm input is INLINE EXPANSION INSIDE
THE FUNCTION ITSELF - a first-in-TU probe with one tiny `inline` callee
expanded in its first guard emits the MERGED shape (2 rets), and the
preprocessed real TU minimizes to `GruntDirStatics.h` alone as the
preceding-warmth carrier while headers-without-it stay cold. The full
model: C2's cross-jump activates at the first function-like IL processed -
a preceding body, a `$E` at its source position, OR the function's own
first inline expansion. ChargeStep warms itself via `m_coordList.GetCount()`
(MFC inline; no MfcNoInline in this TU). POSITIONAL variant falsified in
the same round: the expansion warms the WHOLE function wherever it sits
(early-guard and last-guard probes both fully merge). The sharp residue:
retail ChargeStep is cold with EQUAL call multisets and equal operands -
the era compile expanded NO inline there, yet its count reads are direct
member loads like ours and its accessor calls match ours. What non-inline,
non-call spelling produced its member reads (or what suppressed its C1
expansions TU-wide) is the single open question for the step band; test
/Ob-flag and era-header-inline-visibility hypotheses against a full-band
call census before any source churn. The step-band rows' skeleton deltas (ChargeStep
10v11, WanderStep 11v12, ScanNearestTarget 11v12, RunEntranceMove 5v6) are
all THIS one input; mapping it would close the band's CFG residue in one
move.

## Round 6: the mechanism, probe-complete (our side)

The size/warmth framings were artifacts. The controlling pair, isolated by
matched probes:

| probe (first in TU, /GX)                  | rets |
|-------------------------------------------|------|
| guards + NEW-expressions (61..549 insns)  | UNMERGED (4) |
| same guards, dtor-local EH, NO news       | MERGED (2)   |
| news + ANY preceding function-like IL     | MERGED       |

So: cl 5.0 SP3 suppresses return-epilogue cross-jumping exactly when the
function is the TU's FIRST function-like IL AND contains new-expression EH
(allocation-cleanup states). A preceding body, `$E` thunk, or the
function's own earlier inline expansion lifts the suppression; local-object
dtor EH does not trigger it. This exactly reproduces CreateChildren
(3 news, first -> EXACT unmerged; mid-TU -> merged) and every fold
production. STILL OPEN for the era side only: retail's step band
(ChargeStep etc.) is unmerged WITHOUT news and not first - no SP3
configuration reproduced that; the leading remaining hypothesis is a
different compiler build (RTM vs SP3) for those compilands, untestable
until an RTM toolchain is provisioned.

V3-consistent datum: hoisting `AddLogic` first does NOT unmerge it - its
`AddTail` MFC-inline expansions self-warm it (the TU has no MfcNoInline),
so ours can never reproduce retail's 3 epilogues under SP3 regardless of
position. With equal call multisets on both sides, retail's cold copy joins
the ChargeStep-class era anomaly - more weight on the RTM-provenance test.

The anomaly is WIDER than epilogue merging: `CMulti::PollSession`
(0xb95f0, 88.41, 96v103) shows retail RE-TESTING a register it just tested
(`cmp ecx,edi; jne` on ecx still holding m_5bc from the immediately
preceding null guard - so era CSE ran, era jump-threading did not) while
ours threads the provably-redundant second guard away. Source carries both
guards verbatim. Era-class signature: redundant-compare retention with CSE
intact. Same provenance test decides it.

Second mid-TU production of the wider anomaly, ARM cross-jump this time:
`CWwdSpatialMgr::Relocate` (0x168500, 86.88, 14v19 calls). Retail keeps FOUR
`RemoveAll` release arms where arms 2 and 3 are BYTE-IDENTICAL to each other
(only arm 1 differs, by an eax/ecx schedule) and still un-merged; our SP3
compile cross-jumps them to two sites. Splitting the TU so Relocate opens its
compiland does NOT unmerge (falsified 2026-08-22) - first-in-TU gates only
/GX EPILOGUE merging, not mid-function arm cross-jump, and Relocate has one
ret on both sides anyway. Identical-arm retention under a converging join is
therefore another era-class datum for the RTM-provenance test, not a source
shape.
