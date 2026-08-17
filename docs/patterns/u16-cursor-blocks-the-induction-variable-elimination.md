# A `u16*` scratch cursor blocks cl's IV elimination - retail walked the row with `u8*` + `Load16`

- **confidence** c9
- **tags** `cpp:local` `cpp:loop` `cpp:cast` | `asm:lea` `asm:sub` `asm:mov` | `topic:codegen-idiom`

## Symptom

A pixel-row loop that steps three buffers in lockstep is 8-40 instructions *longer* than
retail and spills two of the three cursors to the stack every iteration. Retail's loop keeps
**one** cursor register and reaches the other two through a constant bias computed in the
preheader:

```
 base (3 live cursors)                  target (1 cursor + 2 biases)
 preheader:                             preheader:
   mov esi,<g_scratch>                    mov edi,<g_scratch>
   ...                                    mov eax,[esp+0x14]      ; dst
                                          sub eax,edi             ; bias(dst)
                                          mov [esp+0x14],eax
                                          mov eax,[esp+0x18]      ; src
                                          sub eax,edi             ; bias(src)
                                          mov [esp+0x1c],eax
 loop:                                  loop:
   mov ax,word ptr [esi]                  mov cx,WORD PTR [edi]           ; scratch
   mov cx,word ptr [edi]                  mov si,WORD PTR [eax+edi]       ; src  = bias + cursor
   ...                                    lea eax,[edx+edi]               ; dst  = bias + cursor
   add esi,2                              add edi,2
   add ebx,2
   mov [esp+0x1c],ecx                     (no per-iteration spills)
   mov [esp+0x20],esi
```

The extra cursors eat the registers that retail spends on loop-invariant LUT pointers, so
those get spilled too and the frame grows (`sub esp,8` where retail has none).

## Cause

The reconstruction typed the scratch/source cursors as `u16*` (`u16* sc = Scratch16();`,
`u16* ss = Pix16(src);`) while the destination stayed the `u8*` the signature forces
(`void ConvertRowDouble(u8* dst, u8* src, i32 count, i32 rowDelta)`). Written as **byte
cursors with an explicit 16-bit load**, cl relates all three and eliminates two:

```cpp
// blocks the reduction
u16* sc = Scratch16();
u16* ss = Pix16(src);
while (count-- > 0) { u32 d = *sc++; u32 a = *ss++; ...; dst += 2; }

// reproduces retail
u8* sc = g_scratch;
u8* ss = src;
while (count-- > 0) { u32 d = Load16(sc); u32 a = Load16(ss); ...; dst += 2; sc += 2; ss += 2; }
```

`Load16`/`Store16` are the existing `Pix16Ptr`-union inlines in `DDrawShadeBlit.cpp`; a
reverse cursor becomes `u8* sc = &g_scratch[count * 2 - 2];` with `sc -= 2;`.

**CORRECTED 2026-08-18 — the cause is NOT pointer-type identity.** The earlier reading
("cl relates IVs only of the same pointer type, so a mixed `u16*`/`u8*` set is never
coalesced") is falsified: three cursors written all-`u8*`, all-`u16*`, and **mixed
`u8*`/`u16*` with identical byte strides compile to byte-identical objects**, while two
same-type `u8*` cursors at strides 1 and 2 are NOT coalesced. The real conditions are
**equal constant BYTE stride** and **a loop-invariant base** — walking the *parameter*
(`dst += 2` on the incoming pointer) is what blocks the reduction, not the pointee type.
Which cursor survives as the IV is set by declaration order. Full evidence, the
stride/type matrix and the declaration-order lever:
[docs/relevations/wall-reasons-globalopt.md](../relevations/wall-reasons-globalopt.md)
§7-§9. The measured score movements below stand; only the explanation changes, and the
`u8*` + `Load16` rewrite remains the right spelling because it also drops the typed
`Scratch16()`/`Pix16()` indirection.

## Measured

`src/DDrawMgr/DDrawShadeBlit.cpp`, all 47 cursor sites converted:

| function | before | after |
|---|---|---|
| `CDDrawShadeBlit::ConvertRowDouble` 0x14d950 | 62.78 | **72.33** |
| `CDDrawShadeBlit::ConvertRowDoubleFwd` 0x14d5e0 | 64.02 | **71.78** |
| `CDDrawShadeBlit::ConvertRowFlip` 0x14bd50 | 65.99 | **71.17** |
| `CDDrawShadeBlit::BlitShadedMirrored` 0x14b770 | 50.84 | **53.80** |
| `CDDrawShadeBlit::ConvertRow` | 76.56 | 76.90 |
| `CDDrawShadeBlit::BlitShadedForward` 0x14a200 | 54.57 | 54.63 |

Instruction counts moved the right way in every case (`ConvertRowDoubleFwd` 335 -> 325 vs
retail's 327; `BlitShadedForward` 1785 -> 1822 vs retail's 1855).

## How to spot it

`gruntz walls diagnose <rva> --asm` and look in the loop *preheader* for a `sub <reg>,<reg>`
whose operands are two different buffer bases: that subtraction IS a bias, and it means retail
reduced an IV you still carry. The residue after the fix is ordinary register rotation.

related: [2d-array-codegen-signature] (the sibling read of a shifted index),
[member-array-is-a-container-object]
