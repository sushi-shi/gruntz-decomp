# A spilled dword whose BYTES are read back is a 4-byte STRUCT local, not shift/mask arithmetic
tags: cpp:struct cpp:local cpp:loop | asm:mov asm:movzx asm:shr | topic:codegen-idiom topic:mis-model
symptoms: a loop body stores a just-loaded dword to a stack slot it never reloads as a dword
(`mov [esp+N],ecx`), then reads `mov bl,cl` / `movzx cx,ch` from the REGISTER and
`mov cl,BYTE PTR [esp+N+2]` from the SLOT; no `shr`/`and 0xff` pair anywhere
confidence: 9/10

## Symptom

`CRezImage::Convert8To16` @0x00175b80 (8bpp -> RGB555) was transcribed as shift/mask
arithmetic on a `u32`:

```cpp
u32 c = palette[*sp];
u32 r = c & 0xff;
u32 g = (c >> 8) & 0xff;
u32 b = (c >> 16) & 0xff;
```

Retail has no shifts at all:

```asm
mov  ecx,DWORD PTR [ebp+ecx*4]   ; the palette entry
mov  DWORD PTR [esp+0x14],ecx    ; spill it -- never reloaded as a dword
mov  bl,cl                       ; channel 0, from the register
movzx cx,ch                      ; channel 1, from the register
...
mov  cl,BYTE PTR [esp+0x16]      ; channel 2, from the SLOT (+2)
```

## Mechanism

x86 can address bytes 0 and 1 of a register (`cl`, `ch`) but **not byte 2**. So when a
4-byte struct local is read field-by-field, cl5 keeps the value in a register for the first
two fields and spills it once so the third field has an address. The spill with no dword
reload is the signature; a `u32` + `>> 16` would emit a shift, and MSVC5 does not turn a
shift into a byte reload.

Therefore the palette element type is a real 4-byte struct read by name:

```cpp
PALETTEENTRY c = palette[*sp];   // ScanlinePalette::m_colors is PALETTEENTRY[256]
u16 r = c.peRed;                 // byte 0  -> mov bl,cl   (after xor bx,bx)
u16 g = c.peGreen;               // byte 1  -> movzx cx,ch
u8  b = c.peBlue;                // byte 2  -> mov cl,[esp+0x16]
```

## The channel WIDTHS come with it

`movzx dx,dl` / `xor cx,cx; mov cl,bl` are 16-bit widenings and `shr cl,0x3` is an 8-bit
shift — so the intermediates are `u16`/`u8` locals, not `u32`. That also fixes the mask
constant: `r &= ~7` on a `u16` is retail's `and ebx,0xfff8`, which a `u32 & 0xf8` cannot
produce.

The pack itself is then a statement pipeline, one operation per retail instruction:

```cpp
r &= ~7;  g &= ~7;  r <<= 5;  r |= g;  b >>= 3;  r <<= 2;  r |= b;  *dp++ = r;
```

## Evidence

`CRezImage::Convert8To16` 68.61 -> **100.00 EXACT** (with the row cursor written as
`row.m_words + y * m_stride` so the `*2` is pointer scaling, and `sp++` as the loop body's
last statement). Filed `@early-stop` before this.

## Corollary for pack/unpack loops generally

Where the channels ARE genuinely shift/mask (`g_rDown`/`g_rUp` style), the opposite rule
applies: keep the per-channel terms INSIDE the store expression rather than binding each to
a local, so cl folds each term as it is produced instead of computing all of them into
separate registers first — `CDDrawShadeBlit::EncodeRle16` 65.43 -> 79.20.
