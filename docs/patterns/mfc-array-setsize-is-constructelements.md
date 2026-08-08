# A hand-written NULL-fill loop is MFC's `ConstructElements<T>` — and the fill guard is the branch you are missing
tags: cpp:loop cpp:branch mfc:carray | asm:rep-stos asm:test asm:jle | topic:codegen-idiom
symptoms: an inlined `CArray`/`CObArray` `SetSize` expansion is one branch OVER retail per fill site; the base has `test ecx,ecx / jle` immediately before a `rep stosd` that retail reaches with no guard at all; a plain `memset(p, 0, n * sizeof(T*))` instead grows a `shl $2 / shr $2 / and $3 / rep stosb` byte tail retail does not have
confidence: 10/10
variants: redundant-test-elimination-is-syntactic.md, inlined-mfc-accessors-transcribed-as-raw-offsets.md

## Symptom

Every hand-transcribed MFC array `SetSize` in the tree grew the same shape:

```cpp
CShadeTable** pTail = &m_pData[m_nSize];
for (i32 nNew = nNewSize - m_nSize; nNew > 0; nNew--) {
    *pTail++ = NULL;
}
```

cl5 *does* recognise that loop and emits `rep stosd` — but it guards the entry:

```asm
; BASE                                    ; TARGET
  mov  edx,[ebx+0x8]                        mov  edx,[ebx+0x8]
  mov  ecx,ebp                              mov  ecx,ebp
  sub  ecx,edx                              sub  ecx,edx
  test ecx,ecx            ; <- the two      lea  edi,[eax+edx*4]
  lea  edi,[eax+edx*4]    ;    extra        xor  eax,eax
  jle  <skip>             ;    bytes        rep stos DWORD PTR es:[edi],eax
  xor  eax,eax
  rep stos DWORD PTR es:[edi],eax
```

so the function reads one branch over retail per fill site, and the whole
`CShadeTableArray` / `CFaderArray` family sat at 83-92% for it.

## The fix is the real MFC helper

`SetSize` never wrote a loop. It calls `ConstructElements<TYPE>` from
`<afxtempl.h>`, which is

```cpp
memset((void*)pElements, 0, nCount * sizeof(TYPE));
for (; nCount--; pElements++) ::new((void*)pElements) TYPE;   // no-op for a scalar
```

and for a pointer element type that lowers to a **bare `rep stosd` with the
ELEMENT count** — no entry guard and no byte tail. Writing the `memset` by hand
does **not** reproduce it: cl5's inline `memset` never folds a variable byte
count, so `memset(p, 0, n * sizeof(T*))` emits `shl $2 / mov / shr $2 / rep stosd
/ and $3 / rep stosb`, which is 6 instructions worse. Only the helper works.

```cpp
// WRONG - one branch over retail
for (i32 nNew = nNewSize - m_nSize; nNew > 0; nNew--) { *pTail++ = NULL; }
// ALSO WRONG - grows a dword+byte-tail memset retail does not have
memset(&m_pData[m_nSize], 0, (nNewSize - m_nSize) * sizeof(CShadeTable*));
// RIGHT
ConstructElements<CShadeTable*>(&m_pData[m_nSize], nNewSize - m_nSize);
```

Transcribe the rest of `CArray<TYPE, ARG_TYPE>::SetSize` literally too — the
"it fits" arm keeps its `if (nNewSize > m_nSize)` guard (that IS retail's
`cmp ebp,eax / jle`), the grow arm has **no** guard, and `nNewMax` is an if/else,
not an assign-then-fix:

```cpp
i32 nNewMax;                                  // NOT: i32 newMax = m_nMaxSize + grow;
if (nNewSize < m_nMaxSize + grow) {           //      if (nNewSize >= newMax) newMax = nNewSize;
    nNewMax = m_nMaxSize + grow;
} else {
    nNewMax = nNewSize;
}
```

`Serialize`'s tail is `SerializeElements<TYPE>(ar, m_pData, m_nSize)`, not an
open-coded `if (ar.IsStoring()) ar.Write(m_pData, m_nSize * 4); else ...`: the
helper takes the pointer and the count as PARAMETERS, so retail loads both once
*before* the `IsStoring` test and the two arms share `eax` (`shl eax,2` in one,
`lea ecx,[eax*4]` in the other). The open-coded form reloads inside each arm.

## How to find the class

`CShadeTableArray` and `CFaderArray` are not bespoke classes: they are
`CArray<CShadeTable*, CShadeTable*>` and `CArray<CFader*, CFader*>`. The proof is
byte-level — `src/Gruntz/ArraySerialize.cpp` instantiates
`CArray<PLAYLISTINFOSTRUCT*, PLAYLISTINFOSTRUCT*>` explicitly and scores 100%, and
its `Serialize` at 0x39fa0 is byte-identical to `CShadeTableArray::Serialize`
0x14fe90 and `CFaderArray::Serialize` 0x17e2a0 apart from 16-24 relocated bytes
(0x188 all three); the dtors (0x51) and `??_G` thunks (0x1e) pair off the same
way. Whenever a suspected container method has the SAME SIZE as a known template
COMDAT, compare the retail bytes before reconstructing it by hand.

    # the sweep that found it: an unguarded `rep stosl` inside an already-EXACT function
    llvm-objdump -d build/objdiff/base/<unit>.o | rg -B4 'rep.*stosl'

Measured 2026-08-08 (`ShadeTableCache.cpp`, `FaderMgr.cpp`):
`CShadeTableArray::SetSizeGrow` 0x150040 90.03 -> **100.00 EXACT**,
`CShadeTableCache::AddFromArray` 0x14f6c0 92.42 -> **100.00 EXACT**,
`CShadeTableCache::AddFromFile` 0x14f8b0 91.81 -> **100.00 EXACT**,
`CShadeTableArray::Serialize` 0x14fe90 83.83 -> **100.00 EXACT**,
`CFaderArray::Serialize` 0x17e2a0 83.83 -> **100.00 EXACT**,
`CShadeTableCache::HueRampTable` 0x14e830 99.29 -> **100.00 EXACT**.
