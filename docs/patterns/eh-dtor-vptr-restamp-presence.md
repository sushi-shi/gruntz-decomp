# Extra `mov [esi],&??_7Class` in a dtor = THE CLASS ISN'T POLYMORPHIC (not a codegen wall)
tags: cpp:dtor cpp:eh cpp:virtual cpp:layout | asm:mov | topic:mis-model topic:phantom-wall
symptoms: ~Class body byte-identical except ONE extra `mov [esi],&??_7Class` re-stamp right after `mov [esp+N],this`, before the trylevel write; recompile caps ~92-93% with the /GX frame + base-dtor call already exact
confidence: 9/10 (was 6/10 while the diagnosis was wrong — see HISTORY)

**Symptom.** Your `~Class` matches retail except for one extra most-derived vptr re-stamp
that retail simply does not have:
```asm
6ff: mov esi,ecx
701: mov [esp+4],esi          ; retail goes straight to the trylevel write...
705: mov [esp+0x10],1         ;   (NO mov [esi],&??_7Class here)
70d: call Reset
; our recompile inserts `mov [esi],&??_7Class@@6B@` between 701 and 705.
```

**CAUSE: retail's class is not polymorphic — you declared it so.** cl emits that store because
*your* model says the class has its own vtable. Retail has no store because retail's class has
**no vtable at all**. It is a MIS-MODEL, not a compiler quirk, and it is fully fixable.

**Diagnose before you defer** — check the binary, not the codegen:
1. Does `Class` have a vtable in the RTTI/vtable scan (`gruntz.analysis.vtable_scan`)? Any RTTI name?
2. Does retail's `~Class` restamp at all? (If it never does, it isn't polymorphic.)
If both say no: **the "base" is really a MEMBER at +0x00.**

**Why it hides:** a polymorphic base at +0x00 and that same class as a *member* at +0x00 have
**identical layout AND identical codegen** (`mov ecx,esi; call ~CPtrList` either way). Nothing in
the bytes distinguishes them — only the vtable's existence does.

**FIX:** de-inherit; hold the base as a member; drop `virtual` from the dtor.
```cpp
// WRONG - forces a vtable, a restamping dtor, and a vtable-dispatched `delete`
class CFontConfig : public CPtrList { virtual ~CFontConfig() OVERRIDE; };
// RIGHT - retail has no CFontConfig vtable/RTTI and its dtor never restamps
class CFontConfig {
    CPtrList m_list;   // +0x00 (0x1c) - same layout, same codegen
    ~CFontConfig();    // non-virtual -> `delete p` binds DIRECT, as retail does
};
```
**The protected-access tell.** If de-inheriting breaks on `error C2248: cannot access protected
member` (`m_pNodeHead`, `CNode`, `m_nCount`), that open-coded node-walk is *itself* the mis-model
— it is what forced the fake inheritance. MFC's public accessors are inline and emit the same
loads, so they are byte-identical AND legal:
`GetHeadPosition()` → `return m_pNodeHead;` · `GetNext(pos)` → `n=pos; pos=n->pNext; return n->data;`
· `GetCount()` → `return m_nCount;`

**Modeling the base subobject is still required** for the EH frame + base-dtor call (else you fall
to the ~50-60% plateau of eh-dtor-needs-base-subobject.md) — a *member* supplies both identically.

**Evidence.** `CFontConfig::~CFontConfig` (0x85f40): 92.7% → **100.0000%** on de-inheriting;
`FreeNodes` → **100%** on swapping the protected node-walk for the public API. It also let
`delete m_chatLog` bind direct instead of through a vtable (see the delete-fold work).

**HISTORY — why this doc was wrong (2026-07-17).** It previously claimed *"cl elided it because the
dtor body makes no virtual call on `this`, so the vptr value is dead"*, concluded *"the polymorphic
model is correct; only this one re-stamp is unreachable"*, and told readers to **defer to the final
sweep**. That invented a compiler behaviour to explain evidence whose real meaning was "this class
has no vtable". A wall that survives only because a doc says it's unfixable is a phantom: when the
bytes and your model disagree, suspect the MODEL first.
related: eh-ctor-vptr-restamp-position.md, eh-dtor-needs-base-subobject.md, inline-base-dtor-folds-into-leaves.md
