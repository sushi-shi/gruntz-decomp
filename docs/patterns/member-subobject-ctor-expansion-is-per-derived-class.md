# Which member sub-object ctors a factory expands is decided PER DERIVED CLASS — tag-dispatch it
tags: cpp:ctor cpp:inline cpp:class | asm:call asm:mov | topic:codegen-idiom
symptoms: a family of `new CDerivedX(...)` factories that are byte-identical in source shape scores 100% for one member and 87-97% for the siblings; the diff is a target-only run of 3-5 `mov [<sub>+N],<zeroreg>` stores next to a `call` we do emit, or the reverse
confidence: 9/10

## The claim

`cl 5.0 /O2 /Ob1` inlines a header-defined constructor **until its expansion budget runs
out**, and that budget is spent differently in each leaf of a class family because each
leaf contributes different members. So the *same* member sub-object constructor is
`call`ed in one factory and expanded in another — and both are the retail bytes.

`CDDrawChildGroup`'s four object factories, all `new CWwdGameObject*(owner,id,flags)`:

| factory | `CGameObject` base ctor | `WwdRegion m_region` | `CAniAdvanceCursor m_animCursor` |
|---|---|---|---|
| `CreateSpriteObject` 0x159600 (`CWwdGameObjectA`) | EXPANDED | `call ??0WwdRegion` 0x15b2b0 | `call ??0CAniAdvanceCursor` 0x15b730 |
| `CreateDotObject` 0x159250 (`CWwdGameObjectC`) | EXPANDED | EXPANDED: `call ??0WwdGridNode` 0x15b2a0 + one `m_object` store | n/a |
| `CreateDeferredObject` 0x159440 (`CWwdGameObjectF`) | EXPANDED | EXPANDED, same shape | n/a |
| `CreateContainerObject` 0x1598d0 (`CWwdGameObject`) | `call` 0x15b390 | (inside the base call) | EXPANDED: `call ??0CLoadable` + vptr + three NULLs |

`0x15b730` has exactly **one** retail caller (`sema xref`), and `0x15b2b0` exactly one —
so "the ctor is out of line" is false for every other site even though the out-of-line
COMDAT exists. An `inline` function cl declines to expand at one site still gets its
COMDAT emitted; a single caller is the tell, not the absence of one.

## What to write

The codebase already has the device — a tag enum that names the expanded sibling
(`CGameObject::EInlineBase`). Extend it one level at a time, driven by the byte evidence,
never by symmetry:

```cpp
struct WwdRegion : WwdGridNode {
    WwdRegion();                                  // out of line, 0x15b2b0
    enum EBaseCall { BASE_CALL };
    WwdRegion(EBaseCall) : WwdGridNode() { m_object = NULL; }   // call 0x15b2a0 + 1 store
};

struct CGameObject : CResolveNode {
    enum EInlineBase { INLINE_BASE };                        // CWwdGameObjectA takes this
    enum EInlineBaseAndRegion { INLINE_BASE_AND_REGION };    // CWwdGameObjectC/F take this
    CGameObject(CDDrawSurfaceMgr*, i32, i32, EInlineBaseAndRegion);
};
inline CGameObject::CGameObject(CDDrawSurfaceMgr* o, i32 id, i32 f, EInlineBaseAndRegion)
    : CResolveNode(o, id, f), m_region(WwdRegion::BASE_CALL) { AttachToOwner(o, id); }
```

Measured: `CreateDotObject` 96.89 -> **100.00 EXACT** and `CreateDeferredObject` 96.17 ->
**100.00 EXACT** on the region tag alone (both were also `sub esp,0x8` where retail is
`0xc` — the frame delta was a consequence of the missing expansion, not a missing local).
The cursor tag took `CDDrawWorkerHost::ReadPlaneObjects` 82.19 -> 83.05.

## The limit, and the thing that does NOT work

The tag only chooses at the level you place it. `CreateContainerObject` needs
`CAniAdvanceCursor`'s body expanded **but its own `CLoadable` base kept as a `call`** —
a depth-4 decision inside the expansion. There is no tag for that (a second `CLoadable`
overload would need a second symbol for retail's single 0x156cb0 body), and
**`#pragma inline_depth(2)` / `(3)` are ignored by cl 5.0** — measured, byte-identical
output. That site is parked at 85.39 (down from 87.20 with the *wrong* shape); the MAX
ledger keeps the old number because the function's own source never changed, which is the
right trade: model the expansion retail actually has and take the current-% dip.
