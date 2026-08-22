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
