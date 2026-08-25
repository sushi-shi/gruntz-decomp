# A run of adjacent same-value `buf[k]=c` stores is a LOOP — cl's fill expansion merges it to DWORDs

tags: cpp:loop cpp:array | asm:mov asm:stos asm:shl | topic:codegen-idiom
symptoms: retail writes a 16-bit array with `mov DWORD PTR [esi+N],eax` at a 2-byte-misaligned
offset, preceded by the word-duplication idiom `mov cx,ax / shl ecx,0x10 / mov cx,ax`; the
recompile emits one `mov WORD PTR [esi+N],bx` per element; long runs already agree (`rep stos`)
confidence: 10/10

`for (i=a; i<b; i++) buf[i] = c;` over a `u16*` is recognised by cl5 /O2 as a **fill**: it
duplicates the 16-bit value into a dword (`mov cx,ax / shl ecx,0x10 / mov cx,ax`) and then
writes the range with **DWORD** stores — `rep stos DWORD` for a long run, a short unrolled
sequence of `mov DWORD PTR [dst+N],eax` below the rep-stos threshold, and a trailing
`stos WORD` when the element count is odd. **Alignment is irrelevant** — cl happily emits
`mov DWORD PTR [esi+0x4e],eax` at `≡2 (mod 4)`.

Writing the same range as *individual assignments* does NOT get the fill expansion: cl emits
one 16-bit store per element and never builds the duplicated dword. That is the whole
difference, and it is invisible in a `--diff` that only says "N word stores vs M dword stores".

```cpp
// NO - eight 16-bit stores, no dword duplication:
buf[1] = c00; buf[2] = c00; buf[3] = c00; buf[4] = c00;
buf[5] = c00; buf[6] = c00; buf[7] = c00; buf[8] = c00;

// YES - cl duplicates c00 into a dword and writes four DWORDs:
for (i = 1; i < 9; i++) { buf[i] = c00; }
```
```asm
mov ax,bx | mov cx,ax | shl ecx,0x10 | mov cx,ax | mov eax,ecx   ; c00:c00
mov DWORD PTR [esi+0x4e],eax | ... [0x52] | ... [0x56] | ... [0x5a]
```

**Read the run length straight off the fill.** `lea edi,[esi+X]` + `mov ecx,N` + `rep stos DWORD`
is `2*N` words at `(X - bufOffset)/2`; a following `stos WORD PTR es:[edi],ax` adds **one more**.
That trailing `stos word` is the tell for an odd-length run and is easy to drop when
transcribing by hand.

Even a **2-element** run is a fill (one `mov DWORD PTR [dst],eax`), so `buf[195]=c; buf[196]=c;`
must be written as a 2-iteration loop too.

Evidence (2026-07-28, `src/Gruntz/Minimap.cpp`): `CMinimap::Shape1..Shape8` had all
67 short runs spelled as individual assignments. Converting them to loops took the eight from
**82.7–85.4% to 98.5–99.5%** in one change — and dropped the frame size from 20 to 21 dwords,
which incidentally dissolved the "retail enregisters `g_rDown` in ebx / we give ebx to colour #0"
allocator split that had been filed as the family's residual wall (see
[cse-partial-term-is-not-a-separate-constant.md](cse-partial-term-is-not-a-separate-constant.md)).
The same pass found a real **runtime pixel bug**: `Shape5..Shape8`'s `buf[124..138]` run is 15
words (`mov ecx,0x7` + `rep stos DWORD` + `stos WORD`) and had been transcribed as 14.
