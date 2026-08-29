# `shr 0x10 / and 0xff` (never a byte load) + `mov dl,ah` = `GetBValue`/`GetGValue`

tags: cpp:expr cpp:int cpp:param | asm:shr asm:and asm:mov | topic:codegen-idiom

symptoms: a function that splits a packed `COLORREF`-shaped `int` parameter into
three channels sits 5-25 points low; the masked diff shows our BLUE channel as
`xor eax,eax / mov al,BYTE PTR [esp+param+2]` (a byte load straight out of the
parameter's home slot) where retail has `mov edx,eax / shr edx,0x10 /
and edx,0xff`, and our GREEN as `mov edx,eax / sar edx,0x8 / and edx,0xff` where
retail has `xor edx,edx / mov dl,ah / and edx,0xff`.

confidence: 10/10

## Two independent tells

1. **`shr`, not `sar`.** The parameter is `int` (`H` in the mangled name), so
   `(color >> 0x10) & 0xff` in source is an ARITHMETIC shift. Retail's logical
   shift says the shifted operand is UNSIGNED at the shift site.
2. **cl5 folds `(mem >> 16) & 0xff` into a byte load** off the memory operand
   when the value comes straight from a parameter slot. Retail does not fold,
   so its operand is a REGISTER copy.

Both fall out of `wingdi.h`:

```c
#define GetRValue(rgb)  ((BYTE)(rgb))
#define GetGValue(rgb)  ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb)  ((BYTE)((rgb)>>16))
```

`GetGValue`'s `(WORD)` cast is literally `ah`; the `(BYTE)` result widened back
to `int` is the otherwise-redundant `and reg,0xff` that a hand-written
`& 0xff` would have folded away. `GetBValue` on an unsigned `rgb` gives the
`shr`, and the register copy blocks the byte-load fold.

```cpp
u32 rgb = static_cast<u32>(packedColor);   // the param stays `int` - mangling unchanged
i32 cb = GetBValue(rgb);
i32 cg = GetGValue(rgb);
i32 cr = GetRValue(rgb);
```

## Declaration order is read from each site

Retail extracts in that order in both call sites (`[esp+0x48] <- blue` first),
independent of the order the channels are USED downstream. `RgbToHsv` in the
same TU already spelled the macros, which is the corroboration that this file's
author used them. This order is evidence for those two `CShadeTableCache` sites,
not a universal property of the macros: `FontRenderer::DrawGlyphRun` extracts
red, green, then blue and retail selects that order.

The unsignedness can also live in the owner. `DrawGlyphRun` initially declared
its packed member as `i32`, so `GetBValue(m_color)` still lowered with `sar`.
Restoring the layout-identical SDK type `COLORREF m_color` changes the shifted
operand itself to unsigned and produces retail's `shr`; no temporary cast or ABI
change is needed. The macros moved 91.30% to 93.10%, and the member type reached
93.54% before the independent clipping-local correction.

## Evidence

`CShadeTableCache::SubTable` 0x14f310 73.50 -> 79.32 on the reorder + macro
form alone (and again -> 80.47 with the loop-direction fix);
`CShadeTableCache::HueRampTable` 0x14e830 70.85 -> 83.37 on the same change,
then -> 99.29 once the conversions were left inline
([hoisting-an-invariant-by-hand-moves-it-out-of-the-loop-nest](hoisting-an-invariant-by-hand-moves-it-out-of-the-loop-nest.md)).

## Related

- [packed-color-is-the-rgb-macro](packed-color-is-the-rgb-macro.md) - the
  INVERSE direction (`RGB(r,g,b)` packing).
- [div-mul-lower-too-late-to-fold-with-a-mask](div-mul-lower-too-late-to-fold-with-a-mask.md)
  - the other "cl5's mask/shift peephole ran and retail's did not" family.
