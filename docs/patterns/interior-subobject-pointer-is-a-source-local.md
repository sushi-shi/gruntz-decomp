# A subobject offset retail ADDS into a register was a source pointer local, not a folded displacement
tags: cpp:local cpp:pointer cpp:member | asm:add asm:mov | topic:codegen-idiom topic:correctness
symptoms: add reg,0x1a0, immediate multiset differs, base has [reg+0x1c8] where retail has [reg+0x28], instruction count one or two low
confidence: 9/10

Retail materializes the ADDRESS of a nested subobject once (`mov r,[this+outer]`
then `add r,K`) and reaches its fields as `[r+field]`; the reconstruction writes
the full chain `m_outer->m_sub.m_field` at each use, and cl folds `K` into every
displacement as `[r+K+field]`. The tell is an immediate present in retail's
multiset and absent in ours whose value equals a subobject offset, with our
member displacements exactly `K` larger than retail's. Distinct from
[member-not-reread-after-a-call-names-a-source-local.md](member-not-reread-after-a-call-names-a-source-local.md):
nothing here is re-read across a call, and the offsets — not the loads — differ.

```cpp
// folds K into every displacement:
if (m_wwdObject->m_animationCursor.m_finished != 0
    && m_wwdObject->m_animationCursor.m_frameTicksLeft == 0) {

// retail: the interior pointer is its own local
CAniAdvanceCursor* cur = &m_wwdObject->m_animationCursor;
if (cur->m_finished != 0 && cur->m_frameTicksLeft == 0) {
```
```asm
; retail — K materialized once, fields at their true offsets
mov eax,DWORD PTR [esi+0x154]
add eax,0x1a0
mov ecx,DWORD PTR [eax+0x28]
mov ecx,DWORD PTR [eax+0x20]
; base — K folded away, and the `add` immediate is simply missing
mov eax,DWORD PTR [esi+0x154]
mov ecx,DWORD PTR [eax+0x1c8]
mov ecx,DWORD PTR [eax+0x1c0]
```
STEERABLE. `CGrunt::RearmEntranceDrop` 0x68370 97.16 -> 100.0000 EXACT by one
declaration; the sibling `CGrunt::ResetEntranceDropIfIdle` directly above it in
`GruntEntranceMove.cpp` already carried the same `CAniAdvanceCursor* cur = ...`
spelling and was already exact, so a matched neighbour in the same file is the
cheapest place to read the intended idiom off. The immediate-multiset screen
(base vs target) finds these mechanically: a retail-only immediate equal to a
known member offset is this pattern almost every time.
