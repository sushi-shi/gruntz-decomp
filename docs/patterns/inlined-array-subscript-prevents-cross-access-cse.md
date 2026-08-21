# An inlined array subscript keeps two logical element accesses distinct

**Tags:** cpp:inline cpp:operator cpp:array cpp:member | asm:mov | topic:codegen-idiom topic:correctness

## Symptom

A small array search has the same instruction, call, branch, return, constant,
and referent counts as retail, but the raw-member reconstruction caches the
array base across both source-level element accesses:

```asm
; raw m_pData spelling                  ; retail
mov  edi,[ecx+8]                        mov  edx,[ecx+8]
mov  ecx,edi                            mov  ebx,[edx]
mov  ebx,[ecx]                          ...
...                                     mov  ecx,[ecx+8]  ; re-read on hit
mov  eax,[edi+eax*4]                    mov  eax,[ecx+eax*4]
```

`gruntz walls diagnose` reports REGALLOC/SCHEDULING because the skeleton is
identical. The important clue is semantic: retail preserves the owning `this`
in `ecx` so the hit path can perform a second array access, while the direct
member spelling destroys `this` and reuses the cached base.

## Cause and fix

The two accesses were written through the array class's inline subscript
operator, not by naming its storage member. Inlining removes the calls, but the
front end retains distinct expression identities long enough that VC5 does not
fold the hit access into the loop's cursor:

```cpp
struct ItemArray : CObject {
    Item** m_pData;
    i32 m_nSize;

    Item*& operator[](i32 index) {
        return m_pData[index];
    }
};

for (i32 i = 0; i < m_items.GetSize(); i++) {
    if (m_items[i]->m_key == key) {
        return m_items[i];
    }
}
```

This is a modeling fix only when the container already has independent evidence
for a typed array API. Do not invent an accessor around arbitrary storage as a
codegen device. `CShadeTableArray` has the canonical MFC array layout
(`m_pData`, size, capacity, grow-by), virtual serialization, and the complete
SetSize behavior, so `GetSize` and `operator[]` are part of the recovered
container surface rather than a probe.

## Controlled result

Measured 2026-08-21 on `CShadeTableCache::FindByKey` at `0x14fb40`:

- direct size plus direct `m_pData[i]`: 77.9688%, 32 instructions;
- `GetSize()` plus direct `m_pData[i]`: 77.9688%, byte-identical negative control;
- direct size plus `operator[]`: **100.0000% EXACT**;
- retained `GetSize()` plus `operator[]`: **100.0000% EXACT**, 0x3e bytes,
  32 instructions, 0 calls, 3 branches, 3 returns, and 0 relocations on both
  sides.

The isolation proves that the subscript boundary, not the size accessor or a
generic loop reordering, is the closing source fact.
