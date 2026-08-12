# Three x87 divides prove the attribute delta is divided before the clip distance is multiplied
tags: cpp:float cpp:expr | asm:fdiv asm:fmul asm:fxch | topic:codegen-idiom topic:wall
symptoms: a clipping/interpolation block has one axis denominator and three attribute numerators; retail emits three `fdiv st,st(N)` instructions before loading the clip-plane distance, while base emits one division for a shared ratio and multiplies each numerator by it
confidence: 10/10

Algebra does not identify the original expression tree. These two spellings
compute the same intersection component, but MSVC 5.0 gives them different x87
DAGs:

```cpp
// One shared clip ratio: cl can divide once and multiply every numerator by it.
prevAttr + attrDelta * (clipDistance / axisDelta)

// Retail shape: each numerator is divided independently, then scaled.
prevAttr + (attrDelta / axisDelta) * clipDistance
```

The retail signature is decisive when several attributes share the same axis:
load the denominator and all numerators, issue one `fdiv` per numerator, then
load/store the common clip distance and issue the `fmul`s. A single `fdiv`
followed by several numerator multiplications is the first spelling instead.

Do not introduce named quotient or weighted-delta locals just to force the
schedule. In the calibration case they became real stack storage, added a frame,
and destroyed the otherwise-correct block. Preserve the expression tree directly.

Evidence: `RotateRasterize` at `0x00146550` has three interpolated attributes in
each of four clipping passes. Reassociating all twelve expressions from the
shared-ratio spelling to divide-before-scale raised current-source MAX from
66.1540% to 81.3884% and reproduced retail's three-divide skeleton. Subsequent
traversal and lifetime corrections reached 95.5313%; the remaining `fxch`
choreography is a separate scheduling wall.

This is deliberately not a universal preference. `ImagePolyClipRect` has only
one interpolated attribute and its own retail bytes support the opposite
plane-first tree. Count and order the actual `fdiv` instructions at each site.
