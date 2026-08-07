# The 16-bit pixel pack is an inline helper taking three u8 BY VALUE

- **tags**: `cpp:inline` `cpp:cast` `asm:shr` `asm:shl` `asm:and` `topic:codegen-idiom`
- **confidence**: 9/10

## Symptom

A `dst = (u16)((r >> g_rDown << g_rUp) | (g >> g_gDown << g_gUp) | (b >> g_bDown))`
transcription plateaus in the 60s. Two shapes differ from retail:

1. retail masks **once**, `and eax,0xffff` immediately before the store; the
   recompile distributes the truncation into the OR operands
   (`and edx,0xffff` + `and ebx,0xffff`, an extra register pushed/popped);
2. retail shifts the *blue* channel 8-bit (`mov dl,[..+2]; shr dl,cl`); the
   recompile zero-extends it to 32 bits first and shifts 32-bit.

A third, subtler one: retail reads **peRed first**, the recompile reads
**peGreen first** — and *swapping the operands in the source does not change
it*, because cl canonicalises the commutative `|` tree.

## Cause

The devs wrote a file-static inline helper whose parameters are `u8` **by
value** and whose return type is `u16`:

```cpp
static inline u16 PackPalEntry16(u8 r, u8 g, u8 b) {
    return static_cast<u16>(
        (static_cast<u8>(r >> g_rDown) << g_rUp)
        | (static_cast<u8>(g >> g_gDown) << g_gUp)
        | static_cast<u8>(b >> g_bDown)
    );
}
...
u16 packed = PackPalEntry16(pal[idx].peRed, pal[idx].peGreen, pal[idx].peBlue);
m_bltFx.dwFillColor = packed;
```

Three things are each load-bearing and each was measured:

- **by-value `u8` params** fix the channel evaluation order to r, g, b. Reading
  `pe.peRed`/`pe.peGreen` off a `const PALETTEENTRY&` *inside* the helper lets
  cl pick its own order (green first) and no source reordering recovers it.
- **the `u16` return** puts the truncation at the inlined return, so cl emits
  ONE `and eax,0xffff`; spelling it `dst = (u16)(a|b|c)` (or via a `u16` local
  holding the same expression) lets cl distribute the mask into the operands.
- **the shift COUNT stays `i32`** — `>> g_rDown`, *not* `>> (u8)g_rDown`.
  Casting the count is what stops cl demoting the blue shift to 8-bit. cl loads
  the low byte of the `i32` global into `cl` on its own.

## Evidence

`CDDrawWorkerHost::ResolveColorKey` 0x163670: 64.75 -> 100.00 EXACT.
A 22-cell axes sweep over orders, associativity, `u16`/`u8` locals and
`(u8)`-cast shift counts scored 62.5 / 97.25 / 100.0 respectively — the three
levers only close the diff together, which is why a sequential edit ladder
misses it.

Sibling call sites already in this shape: `src/Gruntz/TriggerMgr.cpp`
`PackRgb16`, `src/Gruntz/LightFxRender.cpp` `Pack`.
