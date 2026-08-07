# `rep stosd` with an ELEMENT count and no byte tail is a LOOP, not `memset`

tags: cpp:loop cpp:expr | asm:rep asm:shr | topic:codegen-idiom

symptoms: retail fills an array tail with a bare `mov ecx,<n>; sub ecx,<m>; xor eax,eax;
rep stos DWORD` while the recompile's `memset(&p[m], 0, (n - m) * sizeof(T*))` emits five
extra instructions around it - `shl ecx,2 / mov edx,ecx / shr ecx,2 / rep stosd / mov
ecx,edx / and ecx,3 / rep stosb`. Reads as instruction-selection noise; it is a wrong
source construct and `insn_count` sees it as +5 or +6 per site.

confidence: 9/10

cl5's inline `memset` with a VARIABLE size always splits into a dword body plus a byte
tail, and it never folds the `*4`/`>>2` pair away - measured over ten spellings
(`n * sizeof(T*)`, `sizeof(T*) * n`, `n * 4`, `n << 2`, `(unsigned)`, `(size_t)`, a
pre-computed local, a `(void*)` destination). **So a retail fill whose `ecx` holds the
ELEMENT count was not written as `memset`.** What produces it is a plain store loop -
MSVC 5.0 does recognise the memset idiom:

```cpp
CShadeTable** pTail = &m_pData[m_nSize];
for (i32 nNew = nNewSize - m_nSize; nNew > 0; nNew--) {
    *pTail++ = NULL;
}
```

`c > 0` gives retail's `jle` guard, `c != 0` gives `je`; both are one guard. The
`while (c--)` and `do/while` forms do NOT convert - they stay a scalar store loop - so
the loop must be pre-tested.

**The guard is the source's own `if`, so delete it.** Writing `if (n > m_nSize) { for
(...) }` emits BOTH tests. Drop the `if` and let the loop's own test be the guard: cl
rewrites `(nNewSize - m_nSize) > 0` back into `cmp nNewSize, m_nSize`, which is exactly
what retail has.

**One `memset` in the same function can still be a real `memset`.** In
`CShadeTableArray::SetSizeGrow` the whole-array fill DOES have retail's byte tail, because
its byte count `nNewSize * 4` is CSE'd with the `operator new[]` argument - that shared
temp is the thing being shifted. Only the two partial fills are loops. Read each site.

Residue after the fix: the unguarded retail fill (MFC's release-dead `ASSERT(nNewSize >
m_nSize)` site) keeps our loop's one `test/jle`, +2 instructions. No source loop form
found that cl emits unguarded.

Measured (shadetablecache, three inlined copies of the same array-grow body):
`CShadeTableCache::AddFromArray` 85.49 -> 92.42, `CShadeTableArray::SetSizeGrow`
86.57 -> 90.03, `AddFromFile` -> 91.81; per-site delta +13 -> +1/+2.

related: [instruction-count-mismatch-finds-the-real-bug.md](instruction-count-mismatch-finds-the-real-bug.md)
(how the +13 was found in the first place)
