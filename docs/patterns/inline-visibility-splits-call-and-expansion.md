# One tiny helper, two shapes: cl 5 NEVER declines it, so retail's `call` sites saw only a declaration

tags: cpp:inline cpp:call | asm:add asm:cmp | topic:codegen-idiom topic:wall
symptoms: a leaf helper has both out-of-line `call` sites and open-coded expansions in the retail
image - sometimes in the SAME function; making it a header inline reproduces every expansion
byte-for-byte but converts every retail `call` site into an expansion too
confidence: 8/10

## The `add reg,K` tell — an expansion is an inline FUNCTION, not open-coded text

When a bounds/accessor helper takes a pointer parameter, an inline expansion binds the argument
to a **temporary**, so the pointer is materialised as a value:

```asm
; retail CSecretTeleporterTrigger::SpawnTeleporter, 0x42c85
mov  ebx,[eax+0x60]      ; y   <- pushed FIRST: __cdecl right-to-left arg evaluation
mov  ecx,[eax+0x5c]      ; x
mov  eax,[edx+0x30]      ; g->m_world
mov  eax,[eax+0x24]      ;   ->m_level
mov  eax,[eax+0x5c]      ;   ->m_mainPlane
add  eax,0x40            ; &->m_viewRect   <-- THE TELL
cmp  ecx,[eax+0x8]       ; x <  right
jge  out
cmp  ecx,[eax]           ; x >= left
jl   out
cmp  ebx,[eax+0xc]       ; y <  bottom
jge  out
cmp  ebx,[eax+0x4]       ; y >= top
jl   out
```

Open-coded `x < rc->m_viewRect.right && ...`, or a textual macro, folds `+0x40` into every
displacement (`[eax+0x48]`, `[eax+0x40]`, ...) and never emits the `add`. So `add reg,K`
immediately before a run of small-displacement compares off that same register **proves the
source called a function there**. The reversed operand order (y loaded before x) is the second
tell: `__cdecl` evaluates arguments right-to-left, so `F(r, x, y)` touches `y` first.

Reconstructions transcribed per-site betray themselves by hoisting locals in that unnatural
order - `i32 y = obj->m_screenY; i32 x = obj->m_screenX;` - to imitate an argument evaluation
they had not recognised.

## The wall: our cl 5 never declines this body

`CGameLevel::PointInBounds` (`0x6b330`, 42 B) is the worked example: **89** expansions across 32
units, **30** out-of-line calls across 14 functions. Folding all 89 open-coded blocks onto one
in-class definition in `include/Gruntz/GameLevel.h` is byte-neutral at every one of the 89 and
*improves* 15 functions (`CSecretTeleporterTrigger::SpawnTeleporter` 96.43 -> **100.00 EXACT**,
`CGrunt::StepPeerTracking` +2.93, `CDroppedObject::AdvanceFall` +3.68, `CInGameIcon::PlaceAt`
+3.15, `CPlay::OnLButtonDblClk` +3.64).

But cl 5 at `/O2` (`/Ob1`) then expands it at the 30 retail *call* sites as well, and no TU
declines - so **no COMDAT is emitted anywhere** and `0x6b330` goes unclaimed. Measured cost:
`CGrunt::BuildGruntExitAnimation` 100 -> 79.52, `CGrunt::LoadGruntDeathAnimations` 90.57 ->
78.20, `CGrunt::ResetEntranceAnimation` 61.67 -> 51.44, `CGrunt::UpdateArrival(i32,i32)` 82.39 ->
77.86, `CGrunt::RunMoveConfig` 79.97 -> 75.45.

**This is not an inline BUDGET.** `LoadGruntDeathAnimations` is 3536 B and expands the helper 13
times without cl ever declining, so neither caller size nor expansion count gates it. Retail's
`call` sites must therefore have seen only a **declaration** - the inline body lived in a header
those compilands did not include. `LoadGruntDeathAnimations` has 12 calls *and* one expansion in
one body, so at least some of retail's expansions are genuine hand-written copies made in
declaration-only TUs.

There is no single-body C++ spelling that yields both shapes, and a per-TU include split is a
guard device (see the no-guard-devices ruling). So the fold is kept and the five callers above
are `@early-stop` on this wall.

## Do not use the COMDAT band as evidence of inline-ness

`/Gy` is **forced on by `/O2`** on this project (`docs/linker-flags.md`), so *every* retail
function is a COMDAT and the linker may group small functions from several compilands into one
adjacent run. `0x6b330` sits in exactly such a run (`CAniElement::AtChecked` 0x6b270,
`CDDrawSubMgrLeaf::LookupValue` 0x6b2a0, `CWapX::Apply` 0x6b2e0), which looks like an
inline-COMDAT tail and proves nothing. Use the `add reg,K` argument-temporary tell instead.

## Recovering the pin later

`0x6b330` becomes claimable again the moment any base obj emits
`?PointInBounds@CGameLevel@@SAHPBUtagRECT@@HH@Z` - then pin it with
`RVA_COMPGEN(0x0006b330, 0x2a, ?PointInBounds@CGameLevel@@SAHPBUtagRECT@@HH@Z)` in that unit.
Check with `llvm-nm build/objdiff/base/*.obj | grep PointInBounds`.
