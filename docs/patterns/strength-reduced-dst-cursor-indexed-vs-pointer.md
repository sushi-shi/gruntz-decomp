# Expand-loop cursors: `dst[i].field` vs a walking `d` pointer picks a DIFFERENT strength-reduced anchor

**Tags:** cpp:loop cpp:array cpp:local | asm:lea asm:mov | topic:codegen-idiom

## Symptom

A byte-expansion loop (RGB triples -> `PALETTEENTRY[256]`, etc.) is logically
correct, the counter/prologue/tail all match, but the destination cursor is
biased by one field and every store displacement is off by the same amount:

```
retail:  lea edx,[esp+0x1]        ; cursor centred on pal[i].peGreen
         ...  mov [edx-0x1],cl / mov [edx],cl / mov [edx+0x1],cl
base:    lea edx,[esp+0x2]        ; cursor centred on pal[i].peBlue
         ...  mov [edx-0x2],cl / mov [edx-0x5],cl / mov [edx-0x4],cl
```

Same instructions, same order, same `add edx,4` placement — only the anchor.

At stride 4 the divergence is much bigger: one spelling gives a single clean
induction register, the other splits into three addressing modes
(`[edx]`, `[esi+eax-4]`, `[edi+eax-4]` with `esi = pal - src`).

## Fix

Two independent levers; both matter.

**1. Index the destination, never walk it.**

```cpp
// walking pointer -> cursor anchored on the LAST field written (peBlue)
PALETTEENTRY* d = pal;
for (i32 i = 0; i < 256; i++) { d->peRed = ...; d->peGreen = ...; d->peBlue = ...; d++; }

// indexed -> cursor anchored on the MIDDLE field (peGreen), which is retail
for (i32 i = 0; i < 256; i++) { pal[i].peRed = ...; pal[i].peGreen = ...; pal[i].peBlue = ...; }
```

**2. Derive the SOURCE cursor inside the loop too, when retail orders the two
setup instructions dst-first.**

```cpp
// hoisted `s` -> cl schedules its `inc eax` bias BEFORE the dst `lea`
u8* s = static_cast<u8*>(bgr);
for (i32 i = 0; i < 256; i++) { pal[i].peRed = s[2]; ...; s += 3; }

// derived in-loop -> `lea edx,[esp+1]` then `inc eax`, matching retail; cl
// strength-reduces `base + i*3` straight back to the same walking pointer
for (i32 i = 0; i < 256; i++) {
    u8* s = static_cast<u8*>(bgr) + i * 3;
    pal[i].peRed = s[2]; pal[i].peGreen = s[1]; pal[i].peBlue = s[0];
}
```

The loop body emitted is *identical* either way — this only moves the two
prologue instructions relative to each other.

## Why

cl's strength reducer creates one induction variable per accessed object and
picks its anchor from how the object is *addressed* in the source: a walking
`d` pointer is a live value that must survive to the bottom of the body (so the
anchor lands on the last field), while `pal[i]` is a fresh address expression
per statement and the reducer centres the shared cursor. At stride 4 (dst
stride == src stride) the reducer can additionally express two of the three dst
writes as `src_cursor + (pal - src)` constants, which is what produces retail's
three-addressing-mode loop — and only the indexed spelling lets it.

## Evidence

`ApiCallerStubs::CImagePaletteNode`, all three expanders, one edit each
(2026-07-28):

| function | before | after |
|---|---|---|
| `CreateFromRgb` 0x176e70 (stride 3, RGB) | 99.86% (`lea edx,[esp+2]`) | **100% EXACT** |
| `CreateFromBgr` 0x176f30 (stride 3, BGR) | 95.6% (anchor bias) | anchor fixed by lever 1, prologue order by lever 2 |
| `CreateFromBgrx` 0x176ec0 (stride 4, BGR) | 62.7% (clean 1-IV, retail is 3-mode) | 3-mode split reproduced exactly by levers 1+2 |

Both `CreateFromBgr` and `CreateFromBgrx` had been filed as
"stride-4 strength-reduction wall / not source-steerable (pal[i] indexing tried)".
The reason `pal[i]` alone looked like it failed is that lever 2 was missing: with a
hoisted `s`, indexing fixes the anchor but leaves the `inc eax` / `lea edx`
transposition, which objdiff scores *worse* than the systematic bias it replaced.

## Third case: TWO fields per record - the walking pointer buys a SECOND induction variable

The same lever runs on a plain serializer walk, and the tell there is a spare register
plus a spilled counter:

```asm
; retail - ONE cursor, the second field re-lea'd inline, counter in a register
  lea ebx,[edi+0x378]        mov ebp,0x4
  push 4 ; push ebx ; call Write
  lea eax,[ebx+0x4] ; push 4 ; push eax ; call Write
  add ebx,0x18 ; dec ebp ; jne

; base - `nb->m_value` became a SECOND running pointer, so the counter spills
  lea ebx,[edi+0x378]        lea ebp,[ebx+0x4]        mov [esp+0x14],0x4
  ...  add ebx,0x18 ; add ebp,0x18 ; mov eax,[esp+0x14] ; dec eax ; mov [esp+0x14],eax ; jne
```

```cpp
// before - two induction variables
for (i32 m = 0; m < 4; m++) { s->Write(&nb->m_state, 4); s->Write(&nb->m_value, 4); nb += 1; }

// after - one cursor + an inline lea, and the counter stays in a register
for (i32 m = 0; m < 4; m++) { s->Write(&nb[m].m_state, 4); s->Write(&nb[m].m_value, 4); }
nb += 4;
```

`CStatusBarMgr::Serialize` 0x1090a0 97.76 -> 98.69 and `Deserialize` 0x109520
94.73 -> 95.02 on this edit alone (2026-07-28); both had been filed
"trailing 3x4 loop induction-variable form, not steerable (a countdown do-while is
byte-identical to the `for`)" - and that is true, because the countdown is not the lever:
**indexing the record is.**

## Related

- [sib-base-index-follows-local-decl-order](sib-base-index-follows-local-decl-order.md)
  — the other half of the family: which of two live pointers becomes the SIB base.
