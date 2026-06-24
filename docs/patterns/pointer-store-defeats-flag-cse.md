# Writing an array member through a pointer local defeats flag-CSE — re-reads the flag field and reuses the byte mask (`mov al,1; test al,cl`)
tags: cpp:member cpp:branch cpp:local | asm:test asm:mov | topic:codegen-idiom
symptoms: two same-condition `if (m_flag & 1)` blocks — retail re-reads the flag field for the SECOND test (`mov al,1; ... test BYTE [field],al`, byte ops, mask kept in a reg) but your recompile CSE's the first `& 1` (one `and eax,1` / `andb $1,%al`, then `test eax,eax`/`testb %al,%al` on the cached result)
confidence: 7/10

Two consecutive `if (m_flag & 1) {…} else {…}` blocks whose bodies write a member ARRAY: if the
writes go through the TYPED array member (`m_tbl[i] = …`), cl's alias analysis proves they can't
touch `m_flag`, so it CSE's the first `(m_flag & 1)` and tests the cached result for the second
block. Retail instead RE-READS `m_flag` per test and keeps the byte mask `1` live in a register —
because its store goes through a POINTER that may alias the flag. Reproduce by taking a pointer
local to the array and writing index 0 (the one the rep-stos pointer already aliases) through it; cl
then can't prove non-aliasing, re-reads the field, and emits `mov cl,BYTE [field]; mov al,1; test
al,cl` (first) + `test BYTE [field],al` (second, mask reused).

```cpp
unsigned long* tbl = m_2b4;          // pointer local over the array member
for (int i = 0; i < 0x20; i++) tbl[i] = 0;
if (m_334 & 1) { tbl[0] = 0x20; m_2b4[1] = 0x11; /* … */ }   // tbl[0] via the pointer
else           { tbl[0] = 0x39; m_2b4[1] = 0x1d; /* … */ }
if (m_334 & 1) { /* second block — re-reads m_334 */ }
```
```asm
8a 8a 34 03 00 00   mov cl,BYTE [edx+0x334]
b0 01               mov al,1
84 c8               test al,cl          ; first if
; …block…
84 82 34 03 00 00   test BYTE [edx+0x334],al   ; second if, mask al=1 reused, field re-read
```
STEERABLE. Evidence: CInputDevice::SetupKeyTable (@0x133c30) 88.5% → 94.6% — the typed `m_2b4[0]=…`
CSE'd the flag (`and eax,1`/`andb $1,%al`); routing index 0 through a `tbl` pointer local restored
the per-test re-read + `mov al,1; test al,cl`. Residual is epilogue pop-scheduling only.
related: int-to-bool-normalize.md.
