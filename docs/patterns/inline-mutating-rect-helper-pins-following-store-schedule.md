# An inline mutating rectangle helper pins the following store schedule

tags: cpp:inline cpp:pointer cpp:struct cpp:statement | asm:mov asm:dec | topic:codegen-idiom topic:regalloc
confidence: 10/10
symptoms: base and retail have identical extent, topology, stores, constants and
referents, but base schedules the first independent store from the following
block into the middle of four rectangle-field assignments

## Finding

Preserve a source-level inline helper around the rectangle mutation. VC5 does
not treat an inlined function and a macro-expanded copy as the same C1 input,
even when both disappear completely:

```cpp
static inline void SetLevelViewport(LevelCoordRect* rect, i32 w, i32 h) {
    rect->left = 0;
    rect->top = 0;
    rect->right = w - 1;
    rect->bottom = h - 1;
}

SetLevelViewport(&m_viewportRect, w, h);
SetSpatialDefaults();
```

The mutating helper keeps all four viewport assignments ahead of the expanded
spatial-default stores. Hand-expanding the same body lets C2 move the first
independent default store between the parameter loads and decrements.

## Evidence

`CGameLevel::SetViewportSize` at 0x15d030 started at 84.28% with equal 0x92-byte
extents, 33 instructions, no calls or branches, one return, no relocations, and
identical store/displacement/immediate/mnemonic multisets. The only difference
was schedule: base emitted `m_defaultActiveGridCellSize[0] = 500` before the two
decrements; retail completed both decrements and viewport stores, emitted the
two 1000-valued large-cell stores, and only then stored 500.

The controlled abstraction matrix isolated the boundary:

- direct field assignments: 84.28%;
- a macro containing those assignments: the same 84.28% object;
- inline helper taking `LevelCoordRect*`: 100.000000% exact;
- inline helper taking `LevelCoordRect&`: the same exact object;
- existing by-value `MakeRect` and a returned `RECT`: 55.20% with a larger
  aggregate temporary;
- modeling the member as `CRect` and calling `CRect::SetRect`: retail-refuted by
  an out-of-line `__imp__SetRect@20` call and broad assignment-family changes.

This was not compiler-state luck. A 462-cell source/TU-state campaign found one
84.28% island. A separate complete permutation of the four leading grid-default
statements found six islands; its 84.32% maximum only reversed two equal-valued
large-cell stores and did not move the premature 500 store. The pointer/helper
source then compiled identically to retail with exact extent, topology, decoded
instructions, and relocation stream.

Reverse-use rule: when an independent store from the next semantic operation
crosses into a preceding group whose arithmetic is otherwise exact, test an
attested or semantically natural inline mutating helper before permuting stores
or declarations. A macro is not a control for the helper boundary. Pointer and
reference forms can be byte-indistinguishable; choose between them from source
lineage and API conventions, not score.
