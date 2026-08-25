# Base-class fields assigned in the ctor BODY put the vptr stamp in the wrong place — delegate to the base ctor

tags: cpp:ctor cpp:vtable | asm:mov | topic:codegen-idiom topic:mis-model
symptoms: a `__thiscall` ctor whose instruction multiset matches retail exactly, but the
`mov DWORD PTR [this],??_7…` stamp sits BEFORE the base-class header stores where retail has it
after them (or, with a member sub-object, retail stores the header fields before the member's
ctor call and we store them after it); the arg loads sit at the very top of retail's prologue and
in the middle of ours; 60-98%
confidence: 10/10
variants: vptr-stamp-splits-meminit-from-body.md, ctor-vptr-interleave-vs-spelled-out-init.md

MSVC 5.0 emits a constructor as **base ctors → this class's vptr stamp → member ctors →
body**. Assigning inherited fields (`m_id`/`m_flags`/`m_ownerCtx` in this tree's `CLoadable`
family) inside the ctor BODY therefore moves them *after* the stamp and *after* every member
sub-object ctor — which is exactly the wrong side of both. Delegating to the base's inline ctor
puts them back, and the stamp lands where retail has it with no other edit:

```cpp
// WRONG - the trio become body statements; the ??_7 stamp is emitted first
CLogicRecord::CLogicRecord(CDDrawSurfaceMgr* owner, i32 id, i32 flags) {
    m_id = id; m_flags = flags; m_ownerCtx = owner;   // CLoadable's fields
    m_notify = 0; /* … */
}
// RIGHT
CLogicRecord::CLogicRecord(CDDrawSurfaceMgr* owner, i32 id, i32 flags)
    : CLoadable(id, flags, owner) {
    m_notify = 0; /* … */
}
```

Two corollaries:

- **The base's own vptr store is elided, not duplicated.** Only one `mov [this],??_7` survives —
  the derived one — so "retail has a single stamp" is not evidence against a base ctor call.
- **A member sub-object's ctor CALL is the marker that separates the two halves.** If retail
  stores the header fields *before* `call ??0CObArray` and you store them after, the fields are
  in your body and belong in the mem-init list. That is the whole diagnosis for
  `CDDrawWorkerHost::CDDrawWorkerHost` @0x1615a0 (67.7 → **100.00 EXACT**).

**Argument ORDER of the base-ctor call is observable when the ctor is inlined.** cl5 materializes
actual arguments RIGHT-TO-LEFT even for an inlined callee, so the rightmost argument's load is
emitted first. `CLogicRecord`'s inline `(owner, id)` ctor inlined into
`CGameObject::EnsureWorker80/88/90` loads `this->m_id` before `this->m_ownerCtx`, which only
happens with `: CLoadable(owner, id)` — the `(id, owner)` overload emits them the other way round
and costs the last 0.8%. The order at the OUTER `new T(...)` call site does not matter; the base
ctor call's does.

Evidence (2026-07-28, all filed as different "walls" before):
`CLogicRecord::CLogicRecord` @0x15b300 98.18 → **100** ("vptr-last wall … a real-virtual class
forces cl's implicit vptr-first store"), `CAniAdvanceCursor::CAniAdvanceCursor` @0x15b730 97.62 →
**100** ("the 100% spelling needs the base ctor inline, which contradicts its proven out-of-line
0x156cb0 body" — wrong: the inline overloads and the out-of-line `??0CLoadable` are *different
ctors*), `CDDrawWorkerHost::CDDrawWorkerHost` @0x1615a0 67.71 → **100**, and
`CGameObject::EnsureWorker80/88/90` @0x150eb0/0x150f90/0x151070 90.82 → **100** each (there the
whole inline ctor had been hand-spelled with the vptr store dropped — 6 bytes short of the
annotated size, which `gruntz walls diagnose <rva>` shows and objdiff cannot).
