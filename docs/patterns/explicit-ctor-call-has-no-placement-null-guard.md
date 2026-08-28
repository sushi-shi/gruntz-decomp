# Placement `new (&member) T` emits a null guard retail has not — use MSVC5's explicit ctor call
tags: cpp:ctor | asm:cmp | topic:codegen-idiom
symptoms: lea ecx,[this+N]; cmp ecx,0; je — placement new — sub-object construct — guard — addresses off ecx not this
confidence: 9/10

Constructing an EMBEDDED sub-object with placement new makes cl5 emit the standard-mandated
null check on the placement pointer AND then address the whole inlined ctor off that pointer
(`ecx`), where retail addresses everything off `this`. MSVC 5 accepts an **explicit
constructor call** (`obj.T::T();`) — a pre-standard extension — which inlines the same ctor
with NO guard and off `this`. Byte-exact.

```cpp
// NO - cl adds `lea ecx,[this+0x1c]; cmp ecx,0; je` and re-bases the ctor stores on ecx:
new (&m_node1c) CRezItmHashNode;

// YES - same ctor, no guard, stores addressed off `this`:
m_node1c.CRezItmHashNode::CRezItmHashNode();
```
```asm
; retail (no guard, everything off eax == this)
mov    eax,ecx
xor    ecx,ecx
mov    DWORD PTR [eax+0x1c],0x5ef740   ; the sub-object's vptr
mov    DWORD PTR [eax+0x30],ecx        ; the inlined ctor's own zero
mov    DWORD PTR [eax+0x34],ecx        ; ... the enclosing function continues off eax
```
STEERABLE. `CRezItm::Init` @0x1396f0 went **56.00% -> 100.00% EXACT** with this one
change (26 B, rezarchive). Same guard is visible at the `new (&r) CRect(...)` sites
(GruntCombat.cpp:263 notes it independently); the explicit-call spelling is the lever there too.

For a **fresh local** rather than a sub-object the spelling is simply a declaration - the
placement form buys nothing and costs the guard. The two CRect operands of an inlined
`CMapMgr::Clip` are `CRect full(0, 0, w, h);` (four inline stores) plus `RECT rc = CRect(0, 0,
w, h);` (an out-of-line `CRect::CRect` call whose return value is copied field-by-field); that
is exactly what retail emits, with no `test`/`je` anywhere. All 14 `new (&r) CRect(...)` sites
in `BattlezMapConfig.cpp` / `BattlezUnitStep.cpp` were converted 2026-08-08:
`CBattlezMapConfig::AdvanceToEnemyBase` 0x32060 79.42 -> 80.36,
`CBattlezMapConfig::TrackAssignedEnemy` 0x31ca0 78.20 -> 86.30,
`CBattlezMapConfig::StepRowUnits` 0x267c0 85.84 -> 86.65.
