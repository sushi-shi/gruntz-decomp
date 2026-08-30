# An inlined mutating helper can pin an outer structure-return result

tags: cpp:inline cpp:struct cpp:pointer msvc5:sret | asm:mov asm:ret | topic:codegen-idiom topic:method
symptoms: a small by-value aggregate return has the exact calls, CFG, stores,
constants, displacements, and referents, but retail loads the hidden result
pointer near entry, keeps it in a scratch register, and copies it to `eax` at
the tail while base loads it only when the final stores are ready
confidence: 9/10 for the controlled cl 5.0 mechanism; the exact original helper
name and ownership remain source-lineage questions

## Production closure

`CGrunt::GetTilePos` at 0x031c70 returns an eight-byte `Coord`.  The direct
source was complete but compiled to 27 bytes / 9 instructions at 85.70%:
the object supplied both coordinates first, then C2 loaded the hidden result
pointer directly into `eax` for the two stores.  Retail is 29 bytes / 10
instructions: it loads that pointer as instruction two, keeps it in `edx`, and
ends with `mov eax,edx`.

Restoring the coordinate conversion as a pointer-mutating inline helper made
the complete function byte-exact:

```cpp
static inline void ScreenTile(Coord* pos) {
    pos->m_x >>= TILE_SHIFT_PX;
    pos->m_y >>= TILE_SHIFT_PX;
}

Coord CGrunt::GetTilePos() {
    Coord out;
    CWwdSpriteObject* object = m_object;
    out.m_x = object->m_screenX;
    out.m_y = object->m_screenY;
    ScreenTile(&out);
    return out;
}
```

The helper expansion is machine-arithmetic-equivalent to the direct spelling,
but it changes C1's entity/lifetime boundary.  C2 consequently parks the outer
sret pointer before loading either coordinate and emits retail's final result
copy.  A helper that instead owns the two raw-coordinate stores and receives
`&out` is independently byte-exact, proving that the causal input is the
inlined mutating-helper boundary rather than which half of the arithmetic it
contains.

## Negative controls

- direct field assignments, explicit scalar temporaries, and a named `&out`
  pointer local remain in the 27-byte island;
- aggregate initialization is byte-identical to that 85.70% island;
- `Coord::Set` return-pointer and void-Set forms do not select the retail home;
- the real out-of-line `CUserLogic::GetScreenPos` adds a call and falls to
  10.40%, so this is not permission to relabel a plain callee as inline;
- a pair-valued `ScreenPosition` helper creates an eight-byte stack temporary
  and grows the function to 39 bytes / 39.70%;
- a classified 32-island campaign executed 272 source/TU-state variants and
  found only the original 27-byte compiler island.

## Reverse-use rule

When this exact sret schedule appears, first prove the aggregate, return ABI,
stores, and semantic arithmetic.  Then look for an authentic coherent
operation that era source would reasonably express as a pointer-mutating
inline helper.  Test the complete helper boundary, not merely a pointer alias:
the alias is a negative control and does not create the C1 lifetime input.

Keep the helper only when it is humane source and its operation is independently
supported by the surrounding API or repeated source family.  Do not invent an
empty helper, fake escape, volatile carrier, or TU-state declaration to obtain
the register home.
