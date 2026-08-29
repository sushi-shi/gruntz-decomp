# Nested inline pixel helpers recover the front-end tree

tags: cpp:inline cpp:expr cpp:local cpp:int | asm:mov asm:imul asm:shr | topic:codegen-idiom topic:regalloc
symptoms: a hand-expanded pixel blend has the right arithmetic but a large allocation
residue; calls and loops are already present, while branch and relocation counts close
only after packing is nested inside blending
confidence: 9/10

Equivalent arithmetic does not give cl 5.0 the same front-end tree. A 16-bit blend is
naturally layered as three inline operations:

```cpp
static inline u8 BlendChannel(u8 dest, i32 source, u8 cover);
static inline u16 PackPixel16(u8 red, u8 green, u8 blue);

static inline u16 BlendPixel16(u16 pixel, u8 cover, i32 red, i32 green, i32 blue) {
    // unpack the destination channels
    return PackPixel16(
        BlendChannel(dr, red, cover),
        BlendChannel(dg, green, cover),
        BlendChannel(db, blue, cover)
    );
}
```

The nesting is the evidence-bearing part. In `FontRenderer::DrawGlyphRun` at
`0x179e70`, extracting only `BlendPixel16` moved 75.7199% to 77.87%. Nesting the
sequential `PackPixel16` helper inside it recovered retail's exact 40 branches and 25
ordered relocations and reached 90.2593%. Making the cover inverse part of
`BlendChannel`, rather than computing and passing it outside, let C1 represent three
parallel channel operations and reached 91.30%.

The surrounding source still matters. Restoring `GetRValue`/`GetGValue`/`GetBValue`
and the owning `COLORREF` field reached 93.54%; reusing `startChar` as the left-clipping
scan index reached 94.4792%. The final body has retail's exact calls, branch skeleton,
returns, relocations, and semantic operand multisets. Its remaining residue is one
extra copy and a larger frame, so it is a local-census/allocation wall rather than
missing blend behavior.

## Negative controls

- Calling `PackPixel16` for the separate solid-colour initialization fell from 90.2593%
  to 89.31%. One authentic helper expansion does not prove that every equivalent
  expression used that helper.
- Passing the cover inverse into `BlendChannel` was flat; recomputing it inside the
  helper enabled CSE at the caller and improved the allocation island.
- Byte RGB parameters, an `i32` cover, and a destination-pointer mutating blend helper
  were lower or emitted the channel work in the wrong order.
- A 413-candidate campaign over seven source shapes and 59 TU states found 14 compiler
  islands but no exact result. The helper/type/local composition raised the source
  family beyond every state-only result.

## Reverse use

When repeated pixel arithmetic is behaviorally complete but far below retail, recover
the semantic helper tree before permuting the expanded expression. Test each nesting
edge separately, compare branch and ordered-relocation topology, and keep an inline
boundary when direct expansion is byte-flat. Do not infer that a helper was used at an
unrelated occurrence merely because its arithmetic recurs.

related: [packed-color-unpack-is-the-getrgbvalue-macros](packed-color-unpack-is-the-getrgbvalue-macros.md), [cumulative-numerator-is-an-authored-strength-reduction](cumulative-numerator-is-an-authored-strength-reduction.md), [escaped-object-blocks-licm-not-the-copy](escaped-object-blocks-licm-not-the-copy.md)
