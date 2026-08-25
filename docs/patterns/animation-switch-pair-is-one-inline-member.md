# The `m_value = ...m_animation` + apply-new-animation PAIR is ONE inline member of CWapX

tags: cpp:inline cpp:member cpp:call | asm:mov | topic:codegen-idiom topic:scheduling
symptoms: a non-ctor method stuck at 95-99% whose whole residue is that retail emits a
member store (`mov [this+N],imm`) BEFORE the next statement's receiver load
(`mov ecx,[this+0x38]`) and cl emits the load first; instruction multisets and sizes
identical, `walls diagnose` says REGALLOC/SCHEDULING
confidence: 10/10 (106 sites converted, 46 functions up, 10 to EXACT, none down)

## The mechanism

The generalization of
[ctor-body-first-statement-is-an-inline-member.md](ctor-body-first-statement-is-an-inline-member.md)
past the ctor case: cl 5.0's list scheduler hoists a statement's RECEIVER LOAD over any
preceding successor-less store, not just over a leaf ctor's vptr stamp. The lever is the
same inline-expansion boundary, and it has the same precondition - the load must sit
INSIDE the expansion, which means the member is declared on the class that owns the
POINTER (`CWapX`, which owns `m_wwdObject` and `m_value`).

**The pair must be ONE member.** Splitting it into a `CacheAnimation()` plus a plain
`ApplyLookupGeometry()` forwarder is WORSE than not converting: the boundary does move the
load inside, but the call that follows still needs the receiver in ECX, so cl allocates
EDX inside the expansion and pays a `mov ecx,edx` (`CStaticHazard::LoadAttributes2`
97.857 -> 93.929, function grew 0xb2 -> 0xb4). With both statements in one member the call
inside the expansion pins the register and the extra move disappears.

## The devs' shape, from the site census

`m_value = m_wwdObject->m_animCursor.m_animation;` has **116** textual sites, and **109**
of them are immediately followed by exactly one of three calls:

| next statement | sites | member |
|---|---|---|
| `m_wwdObject->m_animCursor.Setup(a)` | 48 | `void SwitchAnimation(CAniElement*)` |
| `m_wwdObject->ApplyLookupGeometry(k, f)` | 46 | `i32 SwitchGeometry(const char*, i32)` |
| `m_wwdObject->ApplyGeometryDirect(a, d)` | 15 | `void SwitchGeometryDirect(CAniElement*, i32)` |

```cpp
class CWapX {
    CWwdGameObjectA* m_wwdObject;
    class CAniElement* m_value;

    void SwitchAnimation(CAniElement* anim) {
        m_value = m_wwdObject->m_animCursor.m_animation;
        m_wwdObject->m_animCursor.Setup(anim);
    }
};

// CStaticHazard::LoadAttributes2
m_fired = 1;
SwitchGeometry("LEVEL_STATICHAZARDGO", 0);
```
```asm
; retail 0xfc0f9                    ; cl, written out
mov  DWORD PTR [esi+0x60],0x1       mov  ecx,DWORD PTR [esi+0x38]   <- hoisted
mov  ecx,DWORD PTR [esi+0x38]       mov  DWORD PTR [esi+0x60],0x1
push 0x0                            push 0x0
push OFFSET "LEVEL_STATICHAZARDGO"  push OFFSET "LEVEL_STATICHAZARDGO"
mov  edx,DWORD PTR [ecx+0x1b4]      mov  edx,DWORD PTR [ecx+0x1b4]
mov  DWORD PTR [esi+0x40],edx       mov  DWORD PTR [esi+0x40],edx
```

## Measured: 106 sites converted, 46 functions up, 10 to EXACT, ZERO site regressions

To EXACT: `CGruntPuddle::Place` 93.560, `CWarlord::RaiseBattleAlert` 95.095,
`CGrunt::ResetGeometry` 95.700, `CWarlord::ResolveMovingAnimation` 97.171,
`CWarlord::ResolveDeathAnimation` 97.828, `CStaticHazard::LoadAttributes2` 97.857,
`CGrunt::LoadVehicleGruntAnimations` 98.623, `CGrunt::StepEntranceRelatchA` 98.799 - all
-> **100.000**. Largest partials: `CProjectile::AdvanceMotion` 88.536 -> 93.387,
`CGrunt::ResetEntranceAnimation` 88.505 -> 92.242, `CGrunt::UpdateArrival` 92.509 ->
95.475, `CGrunt::UpdateEntranceAnim` 94.346 -> 96.816, `CRollingBall::CRollingBall`
77.007 -> 79.313, `CGrunt::SetFacing` 93.374 -> 95.397, `CGrunt::LoadFreezeSpellAssets`
87.381 -> 89.127, `CEyeCandyAni::CEyeCandyAni` 96.601 -> 98.105, `CGrunt::SetupTubeAnim`
97.396 -> 98.825, `CObjectDropper::CObjectDropper` 96.445 -> 97.274,
`CPathHazard::CPathHazard` 98.947 -> 99.925, `CStaticHazard::LoadAttributes` 98.913 ->
99.774. **Not one converted site scored lower**, over three measured batches (5 sites, then
61 across 7 TUs, then 22 across 16 TUs). The remaining 10 textual sites are NOT this shape -
the cache is guarded, branched, reached through `m_object`, or routed via a local
`CAniAdvanceCursor*` - and stay written out.

Corroboration that CWapX is the right owner: `CWapX::ApplyAnimation(CAniElement*, i32)` at 0x6b2e0
is a REAL retail out-of-line method whose body is this same pair plus an optional
`Advance` - the devs kept this exact pair on this exact class.

## The boundary: the receiver must feed a CALL inside the expansion

The sibling census is the negative control. `m_previousAnimationActId = m_logicRecord->m_eventCode;`
followed by `m_logicRecord->m_eventCode = ActFindId(name);` is an even stronger textual pattern -
**108 of its 110 sites** - and `CPathHazard::Tick` shows exactly the same signature
(`mov edx,[esi+0x14]` hoisted over the four `m_leg` i64 stores). Folding both statements
into `CUserLogic::SwitchAct(const char*)` is **byte-IDENTICAL**: 97.5678 before and after,
instruction for instruction. The difference is that `m_logicRecord` feeds only two memory
operands, never a call receiver, so the expansion pins no register and the scheduler is
still free to hoist the load. Do not convert this pair - the census alone does not predict
the lever.

## Cost

Three new declarations in `include/Gruntz/UserLogic.h` moved the /O2 decl-count window
(`seams-stay-local-shared-headers-ripple`, `declaration-count-window-steers-regalloc`) and
produced **8 fresh sub-bank rows in TUs that were never edited**: `CPlay::StepScroll`
100 -> 88.035 (exactly the pre-fix value that pattern doc records for it),
`CNetSession::Verify` 100 -> 89.535, `CSBI_GruntMachine::Render` -3.77,
`CTriggerMgr::WireTileSwitchLogic` -2.15, `CSpotLight::Update` -3.55, and three under 0.25.
The banked MAX is preserved for all eight, against 46 rows raised and 10 taken to EXACT,
so this is the accepted structure-over-current-% trade. `CPlay::StepScroll` and
`CNetSession::Verify` are the follow-up: both reached 100 on a decl-count landing, so both
want a real missing include of their own.

related: [ctor-body-first-statement-is-an-inline-member.md](ctor-body-first-statement-is-an-inline-member.md),
[repeated-container-call-is-an-inline-member.md](repeated-container-call-is-an-inline-member.md),
[declaration-count-window-steers-regalloc.md](declaration-count-window-steers-regalloc.md)
