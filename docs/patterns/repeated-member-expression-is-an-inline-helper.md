# A member expression written out twice CSEs; retail reloads it, so it was an inline helper
tags: cpp:inline cpp:member cpp:cse cpp:local | asm:mov asm:imul asm:cmp | topic:codegen-idiom topic:regalloc
symptoms: retail reloads `[reg+disp]` at every use where the base holds it in a callee-saved register; base frame ONE slot larger than retail; whole-function register renaming (`this` in a different register); base `cmp reg,reg` where retail has `mov reg,[m]` + `cmp`
confidence: 9/10
variants: retail-recomputes-a-shift-we-cse.md, shared-reference-local-cses-an-inlined-helpers-address-math.md, struct-copy-defeats-the-field-load-cse.md

A function repeats the same member-derived expression at two or more sites - a
bounds-checked grid read, a pixel offset, any `obj->a * obj->b + ...`. Written
out inline at each site it is ONE expression tree, so cl CSEs the loads and
parks the value in a callee-saved register for the whole body. That costs a
register, which pushes a real local to the stack, which re-colours **every**
instruction in the function: the diff looks like diffuse regalloc, not a bug.
Retail reloads at each site because the expression was behind a **call** - cl
expands each inline call as its own tree and does not merge them.

The tells, in order of usefulness:

1. retail's frame is one dword SMALLER (`sub esp,8` vs `sub esp,0xc`) - the
   mirror-direction frame rule says the base has an EXTRA spilled local;
2. retail re-emits `mov <reg>,[<obj>+<disp>]` at a site where the base compares
   against a register it filled once;
3. the expression appears verbatim 2+ times in the source.

```cpp
// NO - one tree, cl CSEs `g->m_width` from the loop test into the guard and
// holds it in ebp for the whole nest:
for (u32 x = 0; x < m_tileGrid->m_width; x++) {
    i32 tile;
    if (x < m_tileGrid->m_width && y < m_tileGrid->m_height) {
        tile = m_tileGrid->m_rows[y][x].m_occupantId;
    } else {
        tile = -1;
    }

// YES - the expansion is its own tree; cl reloads [ecx+0xc], as retail does:
static inline i32 OccupantAt(const CGruntzMapMgr* g, u32 x, u32 y) {
    if (x < g->m_width && y < g->m_height) {
        return g->m_rows[y][x].m_occupantId;
    }
    return -1;
}
...
i32 tile = OccupantAt(m_tileGrid, x, y);
```
```asm
; retail - reloaded per site
a3534: mov eax,DWORD PTR [ecx+0xc]      ; loop entry test
a3563: mov eax,DWORD PTR [ecx+0xc]      ; the guard, loaded AGAIN
a365f: cmp esi,DWORD PTR [ecx+0xc]      ; the back-edge, memory operand
; base before the fold - one load, ebp burnt for the whole nest
      mov ebp,DWORD PTR [ecx+0xc]
      cmp edx,ebp
```

Steerable. The codebase already carries the shape as `CMapMgr::CellFlagsAt`
(a real out-of-line COMDAT at 0x75a40) and `TileScan.cpp`'s `GridLookup`, so
prefer an existing accessor before inventing one. Evidence: `CLightFxRender::Resize`
59.75 -> 69.22 (prologue plus the first 68 loop instructions become byte-exact,
frame 0xc -> 0x8); `CLightFxRender::DrawBorderRaw` 72.07 -> 92.01 via a
`PixOffset(surf, x, y)` helper for `y * m_pitch + x * m_bytesPerPixel`;
`CGrunt::StepEntranceReinit` 87.60 -> 90.18 on the same fold onto `CellFlagsAt`.
**Measure per function**: the neighbour `CLightFxRender::DrawBorder` is banked at
MAX 100.00 with the expression spelled OUT at all four of its sites, which is the
byte evidence that retail wrote the two differently.
