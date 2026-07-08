# Phantom tail: COMDAT-folded shared-vtable sibling families

Two phantom clusters that are **not** mechanical view-folds — they need a
hierarchy decision (deferred to a human; see the options at the bottom).

## The pattern

Several classes physically share **one** compiler-emitted `??_7` vtable in the
retail binary (the linker COMDAT-folded byte-identical vtables). To *tie* a
class's virtual-method declarations to real definitions you normally derive from
the class that owns the methods — but here the siblings have **type-distinct
fields at the same offset**, so a plain `: public <owner>` puts the layout wrong,
and `RELOC_VTBL` names the shared vtable *datum* without crediting the redeclared
*methods* (so the pure-phantom count doesn't drop).

## Family 1 — CImage (vtable `??_7CImage` @0x1eaa2c, `VTBL(CImage, 0x001eaa2c)`)

Members: `CImage` (real, matched), `CImageFrame` (include/Image/ImageSet.h),
`CFrameWorker` (src/Gruntz/SpriteResource.cpp).

- `CImage`'s vtable methods (FreeAll 0x153260, Create24 0x1530e0, LoadDispatch
  0x152fb0, Resolve 0x152f20, …) are **defined, byte-matched bodies** in
  `src/Image/CImage.cpp`.
- Fields align through +0x2c (width@+10, height@+14, surface@+2c) but **diverge
  by type at +0x30**: `CImage::m_owned` is `CDDrawShadeBlit*`; `CImageFrame::m_format`
  is `CImageFormat*`; `CFrameWorker` has its own +0x2c/+0x30.
- So they are genuine **siblings**, not one-derives-from-the-other.

## Family 2 — ANI record secondary facet (vtable @0x5f02d8)

Members: `CAniRecordBase2` (src/Gruntz/AniRecord.cpp) and `CDDrawMapWorker`
(src/DDrawMgr/DDrawWorkerMapSmall.cpp) model the **same** 14-slot vtable
(0x5f02d8) in two files. Slots 7/10/11/12 reloc-mask to `CAniRecordView`'s real
`FreeBuf_168fb0` / `Alloc168ee0` / `Alloc168ea0` / `Alloc168f60`.

## Decision needed (per family)

1. **Common base** — extract a `CImageBase` (shared virtuals + common fields
   through +0x2c); `CImage`/`CImageFrame`/`CFrameWorker` derive from it. This is
   the structurally-true shape, but moves `CImage`'s matched method bodies onto
   the base (mangled-name change ⇒ likely **de-matches** CImage's ~15 functions).
2. **Derive-and-reuse** — `CImageFrame`/`CFrameWorker : public CImage`, no own
   fields, reinterpret the inherited +0x30 slot via a cast at the few accesses.
   Keeps CImage matched; costs a small cast at +0x30.
3. **Leave as RELOC_VTBL siblings** — accept them as real distinct classes that
   share a folded vtable; their method redecls stay counted in the metric.

Verify against Ghidra RTTI (which class the `0x1eaa2c` / `0x5f02d8` `.rdata`
actually attributes) before committing to (1) vs (2).
