# Declare the near bound first and leave its inputs inline to get retail's CSE live ranges

tags: cpp:local cpp:member cpp:expr | asm:lea asm:sub asm:mov | topic:codegen-idiom topic:regalloc
symptoms: a four-bound overlap test has the exact branch skeleton and comparisons, but the
base is four bytes short and rotates the registers holding left/right/top/bottom; source
caches the coordinate and extent components in scalar locals before deriving the bounds
confidence: 10/10

For symmetric bounds derived from one coordinate and one extent, the source order that
produces the retail live ranges is not necessarily the order in which C2 emits the
arithmetic.  Declare the near bound before the far bound and leave the member expressions
at the two use sites:

```cpp
i32 left = m_screenX - m_frameImage->m_anchorX;
i32 right = m_screenX + m_frameImage->m_anchorX;
i32 top = m_screenY - m_frameImage->m_anchorY;
i32 bottom = m_screenY + m_frameImage->m_anchorY;
```

VC5 CSEs each repeated member read, emits the far-edge `lea` before the near-edge `sub`,
and gives the four derived values retail's registers.  Hoisting `screenX`, `anchorX`,
`screenY`, and `anchorY` into scalar locals is value-equivalent but gives the expression
trees explicit source live ranges; the object shrinks from retail's 171 bytes to 167 and
the four values rotate.

Measured on `CWwdSpriteObject::IntersectsViewport` 0x1509c0: a 48-cell Cartesian product
crossed eight bound spellings, two equivalent flag tests, and three surface-extent
spellings.  All eight exact cells shared these properties:

- `left` was declared before `right`, and `top` before `bottom`;
- the coordinate and anchor components were left inline; binding only `m_frameImage` to
  a pointer local was neutral;
- surface width was declared before height.

The flag spelling and `const` on width/height were neutral.  Far-before-near stayed at
83.07%, bottom-before-top topped out at 90.43%, and height-before-width topped out at
94.15%.  The retained direct-member form moved 83.07% to **100.00% exact**.

This is not a blanket instruction to duplicate expensive expressions.  Use it when the
retail and base have the same four comparisons and branch skeleton and differ in the
register roles established by the bound-construction prelude.  Cross the two axis orders
instead of assuming rectangle field order is also the compiler's preferred live-range
order.
