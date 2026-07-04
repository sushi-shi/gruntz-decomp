# A shared header's forward-declaration COUNT re-colors an unrelated includer's /O2 schedule
tags: cpp:struct cpp:header | asm:mov | topic:wall topic:regalloc topic:tu-layout topic:codegen-idiom
symptoms: an unrelated function craters (e.g. 100→74%) after a header edit that only renamed
  members / swapped `struct X;` forward decls; the diff is two loads of an `a+b` arg swapped
  (retail loads the `this`-side operand first, recompile loads the pointee-side first); the
  edited header is pulled TRANSITIVELY (via another shared header) into the cratered TU
confidence: 8/10

The number of file-scope forward declarations (`struct X;` / `class X;`) in a shared header is
part of MSVC 5.0's per-TU type-table state. Crossing a count threshold re-colors the register
allocation of an UNRELATED /O2 function in EVERY TU that transitively includes the header — even
when the header change is otherwise codegen-neutral (renames, signature typing). Observed: a
GameLevel.h forward-decl block growing 1→3 structs (total file-scope fwd decls 2→4, pulled into
the CSBI_MenuItem TU via GruntzMgr.h) flipped `CSBI_MenuItem::DecCounter`'s RenderFrame arg block
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
