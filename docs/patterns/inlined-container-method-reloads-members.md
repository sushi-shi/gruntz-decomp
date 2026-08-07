# Member re-loads that look like a missed CSE mean the code came from an INLINED container method

tags: cpp:mfc cpp:method cpp:cse | asm:mov asm:rep | topic:codegen-idiom
symptoms: retail re-reads `[this+N]` in a block where the recompile keeps the same member in a callee-saved register CSE'd from earlier in the function; retail's prologue pushes one FEWER register than the recompile
confidence: 8/10

A function that finds an element and then erases it typically reads the array's
`m_pData`/`m_nSize` twice — once for the search bounds, once for the erase. If you write the
erase **open-coded in the caller**, cl CSEs both members out of the search into callee-saved
registers and the erase reuses them:

```cpp
i32 last = m_arr.m_nSize - 1;                 // esi = m_nSize   (kept)
CFader** w = m_arr.m_pData;                   // edi = m_pData   (kept)
while (*w != p) { ... }
i32 cnt = m_arr.m_nSize - i - 1;              // sub esi,eax     <- CSE, no re-load
CFader** dst = &m_arr.m_pData[i];             // lea edi,[edi+eax*4]
```

Retail instead re-loads **both**, and re-loads `m_nSize` a *third* time for the decrement:

```
mov ecx,[ebx+0x18]      ; m_nSize
mov edx,[ebx+0x14]      ; m_pData
sub ecx,eax
dec ecx                 ; nMoveCount = m_nSize - (index + 1)
lea edi,[edx+eax*4]     ; m_pData + index          <- computed BEFORE the branch
je  skip
lea esi,[edx+eax*4+4]   ; m_pData + (index + 1)    <- re-derived from m_pData, not edi+4
rep movsd
skip:
mov ecx,[ebx+0x18]
dec ecx
mov [ebx+0x18],ecx      ; m_nSize -= 1
```

That is `MFC CArray::RemoveAt` inlined verbatim — cl does not CSE a member load across the
inline-expansion boundary of a method called on a *sub-object* (`m_arr.RemoveAt(i)`, `this` =
`&m_arr`). The three loads, the destination `lea` sitting **above** the `if (nMoveCount)` branch
(it is `DestructElements(m_pData + nIndex, nCount)`'s argument, whose loop cl deletes), and the
source `lea` re-derived from `m_pData` are all signatures of the real MFC body:

```cpp
void RemoveAt(int nIndex, int nCount = 1) {
    int nMoveCount = m_nSize - (nIndex + nCount);
    TYPE* dst = m_pData + nIndex;                       // DestructElements' argument
    if (nMoveCount)
        memcpy(dst, m_pData + (nIndex + nCount), nMoveCount * sizeof(TYPE));
    m_nSize -= nCount;
}
```

Model the container method on the array class and call it. Two related tells in the same
function:

- **`lea edx,[ecx-1]` + `test edx,edx` instead of `dec edx` + `js`** — the bound came from a
  *separate* local (`i32 count = m_arr.GetSize(); i32 last = count - 1;`), which forces the load
  into its own register even though `count` is then dead. `m_arr.GetUpperBound()` (or the direct
  `m_arr.m_nSize - 1`) folds to `dec` + `js`.
- The search loop must be `while (i <= last) { if (data[i] == x) { erase; return; } i++; }`.
  The equivalent `while (data[i] != x) { i++; if (i > last) return; }` peels the first compare
  (see [mfc-map-walk-while-not-guard-dowhile](mfc-map-walk-while-not-guard-dowhile.md)).

`CFaderMgr::Remove` 0x17e170 67.11% → 96.33%, prologue and both loop shapes exact.
