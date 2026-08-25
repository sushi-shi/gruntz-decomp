# The vptr stamp's POSITION among a ctor's field stores tells you which fields were in the member-INIT LIST and which were in the BODY
tags: cpp:ctor cpp:vtable cpp:member | asm:mov | topic:codegen-idiom topic:regalloc
symptoms: an inlined `new T(...)` matches instruction-for-instruction except that `mov [p],??_7T@@6B@` sits FIRST in ours and Nth in retail; filed as a "vptr-scheduler wall" or "the scheduler won't sink the vptr past the member stores"
confidence: 9/10
variants: ctor-vptr-interleave-vs-spelled-out-init.md, ctor-scalar-seeds-interleaved-are-a-mem-init-list.md, ctor-handrolled-vptr-store-last.md

MSVC 5.0 emits an inlined constructor as **[base ctor] → [member-init list, in
declaration order] → [vptr stamp] → [ctor body]**. So the stamp is a *divider* you can
read straight off the disassembly: the fields stored **before** it were written by the
member-initializer list (or a base ctor), the fields **after** it by the body. The fix
is therefore not a scheduling trick — it is to split the ctor the way retail's was:

```cpp
// retail: [+4]=index, [+8]=0, [+0xc]=parent, THEN mov [p],??_7CImage, THEN [+0x10]...
CImage(i32 index, CDDrawSurfaceMgr* parent)
    : m_status(index), m_08(0), m_parent(parent) {   // <- above the stamp
    m_width = 0;                                     // <- below the stamp
    m_height = 0;
    m_surface = 0;
    m_owned = 0;
}
```
```asm
mov  ecx,[esi+0xc]
mov  [eax+0x4],edi         ; member-init list
mov  [eax+0x8],ebp
mov  [eax+0xc],ecx
mov  DWORD PTR [eax],0x5eaa2c   ; ??_7CImage@@6B@   <- the divider
mov  [eax+0x10],ebp        ; ctor body
mov  [eax+0x14],ebp
```
**A FULL init list is NOT the fix** and is why this reads as a dead end when tried: it
moves *every* field above the stamp. Only the pre-stamp fields belong in the list.

**When the pre-stamp fields are the BASE's**, use the base ctor instead (you cannot
mem-init an inherited member) — and cl then dead-store-eliminates the base's own vptr
stamp, leaving exactly retail's single stamp:
```cpp
CLoadable(i32 id, CDDrawSurfaceMgr* owner) { m_id = id; m_flags = 0; m_ownerCtx = owner; }
SoundCue(i32 count, CDDrawSurfaceMgr* h) : CLoadable(count, h) { m_10 = 0; m_18 = 0; m_14 = 0; }
```
STEERABLE, and it pays far beyond the ctor's own call sites. 2026-07-28: the partial
`CImage` init list took `CDDrawWorker::InsertFrame`/`CreateFrame24`/`CreateFrame28`/
`CreateFrame30` 99.4 → **100 EXACT** and, with no edit of their own,
`CDDrawWorker::GetMemoryUsage` (99.96) and `CDDrawWorkerHost::Save` (99.98) — both
filed as **commutative-imul operand walls** — and `CImage::BlitShadeNorm` (99.86). The
`CLoadable(id, owner)` delegation took `CreateEntry`/`CreateEntry2` 99.32 → **100
EXACT**, both filed as ecx/edx register-naming coin-flips. **So a commutative-operand or
register-naming mismatch can be a downstream readout of a wrong constructor shape — check
the ctors in the TU before filing one as a wall.**
