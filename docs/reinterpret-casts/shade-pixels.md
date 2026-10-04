# Shade-row byte buffers viewed as 16-bit pixels

All sixteen expressions are in
[DDrawShadeBlit.cpp](../../src/DDrawMgr/DDrawShadeBlit.cpp). Each row below
identifies one retained cast, including repeated expressions in different arms.
The current implementation uses typed word pointers to traverse byte buffers.

| Function and arm | Local | Expression |
| --- | --- | --- |
| `ConvertRow`, `SHADE_DST_BY_SRC_16` | `d` | `reinterpret_cast<u16*>(dst)` |
| same arm | `sc` | `reinterpret_cast<u16*>(g_scratch)` |
| `ConvertRow`, `SHADE_ALPHA_16` | `d` | `reinterpret_cast<u16*>(dst)` |
| same arm | `s` | `reinterpret_cast<u16*>(src)` |
| same arm | `sc` | `reinterpret_cast<u16*>(g_scratch)` |
| `ConvertRowFlip`, `SHADE_ALPHA_16` | `sc` | `reinterpret_cast<u16*>(&g_scratch[count * 2 - 2])` |
| same arm | `d` | `reinterpret_cast<u16*>(dst)` |
| same arm | `s` | `reinterpret_cast<u16*>(src)` |
| `ConvertRowDoubleFwd`, `SHADE_DST_BY_SRC_16` | `d` | `reinterpret_cast<u16*>(dst)` |
| same arm | `sc` | `reinterpret_cast<u16*>(g_scratch)` |
| `ConvertRowDoubleFwd`, `SHADE_ALPHA_16` | `d` | `reinterpret_cast<u16*>(dst)` |
| same arm | `s` | `reinterpret_cast<u16*>(src)` |
| same arm | `sc` | `reinterpret_cast<u16*>(g_scratch)` |
| `ConvertRowDouble`, `SHADE_ALPHA_16` | `d` | `reinterpret_cast<u16*>(dst)` |
| same arm | `s` | `reinterpret_cast<u16*>(src)` |
| same arm | `sc` | `reinterpret_cast<u16*>(&g_scratch[count * 2 - 2])` |

## Current accesses and actual storage

The row functions accept `u8* dst, u8* src, i32 count`; the double-row
variants also accept a byte displacement `rowDelta`. [Ints.h](../../include/Ints.h)
defines `u8` as `unsigned char` and `u16` as `unsigned short`. The shared
scratch object is explicitly `u8 g_scratch[1280]`. A typical arm is:

```cpp
memcpy(g_scratch, dst, count * 2);
u16* d = reinterpret_cast<u16*>(dst);
u16* s = reinterpret_cast<u16*>(src);
u16* sc = reinterpret_cast<u16*>(g_scratch);
```

The following loop reads `*s` and `*sc`, computes a palette/blend word,
and writes `*d`. These are actual typed accesses, not unused casts.
`dst` originates in a locked DirectDraw surface; pitch and pixel format
constrain it. `src` comes from `m_rleData`, a `new u8[]` allocation with
one-byte run tokens. `EncodeRle16` writes two pixel bytes after each token;
callers pass `&m_rleData[pos + 1]`. Even an aligned allocation can therefore
yield an odd pixel address. The shared scratch array's declared element
alignment is only that of `u8`; retail placement at RVA `0x2bed08` happens
to align it, but `DATA` is an identity annotation, not an alignment promise
for every new link.

## Safety assessment

The comments `byte-forced` describe a representation mismatch, not a safety
proof. A cast does not create an array of `u16`, establish suitable
alignment, or permit arbitrary typed access to byte-declared objects.
The [C++ alignment rule](https://eel.is/c++draft/basic.align) and
[type-access rules](https://eel.is/c++draft/basic.lval) must be considered
separately from x86's ability to execute unaligned word instructions.
Modern implicit-object-creation rules are also not evidence that VC5's
source model provides a portable lifetime guarantee for every buffer.

Additional preconditions are independent of cast spelling: count must be
nonnegative and small enough for `count * 2` and the 1280-byte scratch;
the mirrored last-word expression requires a positive count; destination
rows and `rowDelta` must describe valid surface storage. RLE run lengths
normally bound the count, but the helpers themselves enforce none of these
conditions. Their mirrored decrements can form a pointer before the start
of a word range after its last element. This review does not establish
malformed-resource reachability or prove all retail call paths violate a
precondition.

## Why the casts remain and what a repair requires

The current loops dereference and advance `u16*` cursors, while their public
inputs and scratch storage are `u8*` and `u8[]`. Those declarations force the
conversions in this implementation; the SDK does not require word-pointer
access to these buffers. Keeping the instruction shape does not establish
portable C++ safety.

A repair can index the byte buffers and use `memcpy` between each address
and a real `u16` object. It also needs valid count bounds and reverse indices
that never form a pointer before the array. Such a rewrite changes VC5's
code generation substantially and is not present in the current source.
The `Load16`/`Store16` helpers in [Pix16.h](../../include/Pix16.h) still use
union-based views; merely replacing a direct cast with those helpers would
not resolve the underlying typed-access issue.

The mirrored scratch-copy start addresses and unrelated eight-bit cursor
arithmetic require separate bounds analysis. No complete validation of
malformed RLE resources is claimed.

## Git provenance and GitHub comparison

[40b553b141](https://github.com/sushi-shi/gruntz-decomp/commit/40b553b1418cbcba2022bda41be94697f7312771)
introduced the reconstructed `ConvertRow` operations. The mechanical
[typedef-cast sweep](https://github.com/sushi-shi/gruntz-decomp/commit/6d500a7eab0cec44cf10b2e6e983867f4f5645bc)
changed earlier C-style spelling. Later helper/union passes hid the
conversions; that history did not establish portable aliasing safety.
The sixteen current direct expressions came from
[ba0a980744](https://github.com/sushi-shi/gruntz-decomp/commit/ba0a980744203a10bb208c6ecc27a8d4744843b1):
it restored inline row converters and word walkers to reproduce the retail
call/inline pattern. That is compiler and instruction evidence for the
reconstruction, not evidence of original source spelling or valid alignment
for every input.

[SDL's pixel-reading macros](https://github.com/libsdl-org/SDL/blob/6dc2b2c866ee926bed6cfda891dcdee62dd81bc4/src/video/SDL_blit.h#L155-L170)
also use a `Uint16*` view for two-byte pixels, and its
[pixel writer](https://github.com/libsdl-org/SDL/blob/6dc2b2c866ee926bed6cfda891dcdee62dd81bc4/src/video/SDL_blit.h#L309-L320)
stores through that view. This is a real analogous graphics implementation,
not proof of Gruntz's RLE alignment or a portable-C++ endorsement: SDL's
language, allocations, callers and compiler contract must be considered on
their own terms.
