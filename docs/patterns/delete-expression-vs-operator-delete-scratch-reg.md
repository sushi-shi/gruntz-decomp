# `delete p;` and `::operator delete(p)` pick DIFFERENT scratch registers
tags: cpp:dtor cpp:member | asm:mov asm:push | topic:codegen-idiom topic:regalloc
symptoms: one instruction pair differs, `mov ecx,[esi+N]; push ecx` where retail has `mov eax,[esi+N]; push eax`, whole function otherwise byte-exact, ~99.8%
confidence: 8/10
variants: guarded-delete-null-store-elision.md

For a non-class pointer (`char*`) the two spellings emit the identical
`push p; call ??3@YAXPAX@Z; add esp,4`, but they materialise the operand into a
different scratch register: the **delete-expression** reuses `eax`, the explicit
`::operator delete(...)` call takes the next free register (`ecx`). When the only
residue in a function is that register pick on a free, switch the spelling.

```cpp
// NO - ::operator delete takes ecx
if (m_reader->Read(m_base, 0, m_length, m_buffer) != (i32)m_length) {
    ::operator delete(m_buffer);
    m_buffer = 0;
}

// YES - the delete-expression reuses eax (and cl elides the null guard because
// flow already proved m_buffer != 0)
if (m_reader->Read(m_base, 0, m_length, m_buffer) != (i32)m_length) {
    delete m_buffer;
    m_buffer = 0;
}
```
```asm
mov  eax,[esi+0x38]        ; retail: delete-expression -> eax
push eax
call ??3@YAXPAX@Z
add  esp,0x4
mov  [esi+0x38],0x0
```
STEERABLE. `CRezItm::Load` 99.79 -> **100.00 EXACT**. The reverse case
exists too, so treat it as a two-way lever rather than "always use `delete`" —
`CRezItm::UnLoad` in the same TU stays 100% with `::operator delete`.
