# Shared-header declaration state re-colors an unrelated includer's /O2 schedule
tags: cpp:struct cpp:header | asm:mov | topic:wall topic:regalloc topic:tu-layout topic:codegen-idiom
symptoms: an unrelated function craters (e.g. 100→74%) after a header edit that only renamed
  members, changed an unused member-function signature, or swapped `struct X;` forward
  declarations; the edited header is pulled directly or transitively into the cratered TU
confidence: 9/10

Shared-header declarations are part of MSVC 5.0's per-TU type-table/compiler state. Changing that
state can re-color the register allocation or scheduling of an UNRELATED /O2 function in EVERY TU
that includes the header, even when the victim neither calls nor names the changed declaration.
Forward-declaration count is one proven lever, but not the complete rule: class definitions and
member-function signatures can fire the same family.

The first observed firing was a GameLevel.h forward-decl block growing 1→3 structs (total
file-scope fwd decls 2→4, pulled into the CSBI_MenuItem TU via GruntzMgr.h). It flipped
`CSBI_MenuItem::DecCounter`'s RenderFrame arg block
`mov edx,[this+0x18]; mov eax,[this+0x14]; … mov esi,[f+0x1c]` (retail) into the pointee-first
order, 100→74%. Threshold was 3-total-OK / 4-breaks (2 in the block still matched).

```cpp
// SHED one forward decl to stay under the threshold: a peripheral cross-TU-payload param
// whose concrete type is a .cpp-local view does not need a shared-header forward decl —
// pass it as void* in the class declaration and cast in the .cpp definition.
struct CGameObject;      // core, keep
struct CGameObjChain;    // keep
// (removed: struct EditSink;)
i32 EditDispatch(void* sink, i32, i32, i32);   // was EditSink*; cast to EditSink in the .cpp
```
```asm
; retail (matched, this-operand-first):     ; recompile at count+1 (pointee-first, craters):
mov edx,[eax+0x18]   ; this->m_18           mov edx,[ecx+0x1c]   ; f->anchorY
mov eax,[eax+0x14]   ; this->m_14           mov esi,[eax+0x18]   ; this->m_18
mov esi,[ecx+0x1c]   ; f->anchorY           add edx,esi
```
STEERABLE: reduce the transitively-visible forward-decl count (type a peripheral param `void*`;
a `.cpp`-local view never needs a header fwd decl). The count is chaotic per-consumer — verify
the fix with a full build, since a DIFFERENT includer may have improved at the higher count
(here `teleporter` 3/5→4/5 held at the reduced count; confirm each affected unit). Sibling of
[[fold-view-preserve-declaration-position]] (declaration-position variant of the same type-table
sensitivity). Evidence: DecCounter 0x0e82a0 74.04→100 by shedding GameLevel.h's `EditSink` decl.

SECOND FIRING (same fn, different source): the spine facet-object pass added two typed members to
`GruntzMgr.h` (`CSpriteRefTable* m_spriteFactory`, `CLightFxMgr* m_logicPump`), which meant two
NEW file-scope `class X;` fwd decls in `GruntzMgr.h` — directly in the SBI_MenuItem TU's closure
(SBI_MenuItem.cpp `#include`s GruntzMgr.h). Closure fwd-decl total 39→41 → DecCounter re-cratered
100→74. You CANNOT shed these (the members are typed with their real class, non-negotiable). The
LEVER that keeps the types AND drops the count: replace the fwd-decl with an `#include` of the
type's real (lightweight, Ints.h-only) header — an included full DEFINITION does not count as a
file-scope fwd decl, so the count drops even though the type is now MORE completely modeled.
`#include <Gruntz/InputState.h>` (CInput54, +0x54) + `#include <Gruntz/SpriteRefTable.h>`
(CSpriteRefTable, +0x74) in GruntzMgr.h converted 2 fwd decls to defs (41→39) → DecCounter 74→100,
0 other units moved (blast radius = the 13 GruntzMgr.h consumers; Play already pulled both). Do NOT
`#include` a header that itself carries fwd decls (LightFxMgr.h has 4 of its own → net +3, worse) —
prefer the zero-own-fwd-decl headers, and measure. A same-address global reached under different
per-TU extern NAMES (0x24556c `g_gameReg` vs `_g_mgrSettings`) is NOT this bug and needs no name
pin: DATA relocs pair by RVA/mask, so DecCounter is byte-EXACT with its `g_gameReg` DIR32 reloc.

THIRD FIRING (same fn + a sibling, GruntzMgr.h again, 2026-07-05 gzstar megafold): folding the
CWorldSub28/CWorldSub2c defs onto CSndHost (SoundCue.h include OR a `struct CSndHost;` fwd decl -
both spellings measured IDENTICAL), retyping m_cmdGrid CCmdGrid->CTriggerMgr (fwd-decl swap) and
typing CWorldZ::m_8 `struct CSpriteFactory*` (elaborated) re-cratered DecCounter 100->74.04 AND
CPlay::StepScroll 81.62->63.52 (+ LoadMainStatusBarSprite -7.09, FlashColor -3.85). Net closure
fwd-decl delta ~+2 with no zero-own-fwd-decl include available to shed (the remaining GruntzMgr.h
fwd decls are all GruntzMgr.cpp-local classes with no headers). Both lever spellings tried, no
movement -> ACCEPTED as the clean-room cost of the binary-proven folds (the same commit RAISED
HandleCommand +2.71 by fixing the views' fake call shapes). Re-attack when the CWorld*/EngObj
locals get real headers (each include-as-lever conversion sheds one decl).


FOURTH FIRING (2026-07-19, CButeSection==CButeMgr fold): adding a class DEFINITION
(`struct CBSecStream : zPTree {...}`, + one file-scope fn decl + two ctor member decls) to
ButeMgr.h re-cratered DecCounter 100→74.04 AND flipped one load pair in
`CDDrawWorkerHost::Load` (levelplane, 100→99.98) with the closure fwd-decl census UNCHANGED
(216=216, multiset-identical, verified via clang /E on both branches). So the sensitivity is
to the per-TU type-table CONTENT/state, not only the fwd-decl COUNT — a new class definition
entering the closure fires it too, and then there is NO count lever to pull. Accepted as the
butterfly floor (structure-over-current-%); MAX-fuzzy retains the 100s.

FIFTH FIRING (2026-07-23, `CGruntzMgr::ResetWorldState` ABI correction): changing the shared
header declaration from the false `ResetWorldState(i32)` to the retail-backed
`ResetWorldState()` made that method exact (`ret 4`→`ret`) and raised global current/MAX fuzzy.
The full rebuild also moved three source-identical functions in three separate TUs that directly
include `GruntzMgr.h`: `CGrunt::PhaseStep` 39.7717→38.5960,
`CLightFxRender::Shape6` 77.0497→76.9834, and `CGrunt::RectSegProbe` 78.7723→78.7228.
None of the three source files changed. This proves that member-function signature state, not
only forward declarations or class bodies, belongs to the butterfly family. Keep the correct ABI
and preserve the per-function MAX; use the includer intersection to explain and later reverse
similar dips when other missing authentic declarations are restored.

SIXTH FIRING (2026-07-23, one-letter data-alias cleanup): removing a contiguous block of 15
unused `extern const char g_codeX[]` declarations from `Grunt.h` changed code generation across
its includers even though no remaining function ODR-used any declaration in the block. A
controlled A/B kept the three real source substitutions (`"F"`, `"I"`, and
`"LEVEL_UFOHAZARDLASER%d"`) identical and varied only the declaration block. Restoring the block
returned the whole affected set to its prior codegen and overall fuzzy to 73.64%; removing it
raised fuzzy/MAX to 73.65%. The clean state notably moved `CPlay::StepScroll`
63.5172→71.6207, `CGrunt::StepCompassMove` 32.9266→35.2959, and
`CSpotLight::Tick` 52.0887→53.5444, while five source-identical functions took small current-only
dips (largest 0.0750). `CGrunt::GruntInRadius` visibly changed load/register ordering. This proves
that otherwise-unused namespace-scope data declarations also participate in this state family.
The experiment isolates the block, not an individual declaration or a simple numeric threshold;
do not claim that a particular letter is the lever without a finer A/B. Preserve the authentic
declaration set and use missing or spurious data declarations as a reverse-search dimension for
similar unexplained scheduling residues.

SEVENTH FIRING (2026-07-23, ILT-proven helper-alias cleanup): removing the unused
`CueVisible`, `GruntPointVisible`, and `BoardTest` declarations from `Grunt.h` after
their calls were proven to target the existing
`CGameLevel::PointInBounds` body moved source-identical `CSpotLight::Tick`
53.5444→52.0887. Its source fingerprint did not change; the value is the same
lower compiler-state coloring observed before the sixth firing's declaration
cleanup. `CGrunt::ArrivalScanC` also moved 47.2696→47.2280, but that function
directly changed its call relocation from the placeholder to the real mangled
target and is not an unrelated butterfly control. Keep the real destination and
the two-symbol declared-only reduction; MAX preserves both prior scores while
later authentic `Grunt.h` declaration recovery can reverse the TU-state change.

EIGHTH FIRING (2026-07-23, Grunt helper/signature cleanup): a later `Grunt.h`
batch removed four more xref-disproved helper declarations and one pooled
floating-literal declaration, and corrected `CGrunt::StepArrivalDrop` from
`void` to its wrapper-proven `i32` return. Two direct includers moved without
body edits: `CTriggerMgr::HitTestCell` 39.7921→39.7723 and
`CGrunt::Activate` 34.2545→33.5727. The latter's dependency-aware fingerprint
changed with the header signature even though its source body did not. The
other outstanding regression rows were already isolated above:
`LoadScrollSpeedOptions` belongs to the sixth firing, `ArrivalScanC` to the
seventh, and `PhaseStep` to the fifth. Thus the two new deltas are header-state
recolorings, not evidence against the helper or return-type corrections.
Preserve both per-function MAX values and include the declaration/signature
batch as a reverse-search dimension for these residues.

NINTH FIRING (2026-07-23, larger Grunt thunk-alias collapse): replacing 17
declared-only helper calls with their per-call-site ILT-proven destinations and
removing the corresponding `Grunt.h` member declarations left the exact count
unchanged but recolored direct and transitive includers. Nine unrelated,
source-identical rows moved below their saved current values: the largest were
`CGruntSelectedSprite::Update` 99.2424→84.8485,
`CTriggerMgr::ToggleRegionA` 76.5663→71.9277, and
`CGrunt::LoadWingzGruntSprites` 76.6337→75.2651; smaller movements affected
`CGruntToySprite::Update`,
`CTriggerMgr::ScrollToActiveRecord`/`ToggleRegionB`,
and `CGrunt::StepArrivalDefense`/`ArrivalScanB`/`GruntInRadius`. Two other
rows, `CGrunt::ArrivalRecycle` 55.4211→54.8607 and
`CGrunt::StepAnimDispatchA` 43.5366→43.5229, directly changed their helper-call
targets and are not unrelated butterfly controls. Two further reported rows
were older retained dips
(`CGrunt::Activate` from the eighth firing and
`CPlay::LoadScrollSpeedOptions` from the sixth). The destination corrections
are relocation-backed, so the lower coloring is retained and every prior MAX
is preserved. This broader sample strengthens the reverse heuristic: unexplained
register-order residues in a `Grunt.h` includer may move when another fake or
missing authentic member declaration is corrected, even when that includer
does not call the edited member.
