# Reserved-but-NEVER-TOUCHED frame slots at a regular stride prove a local AGGREGATE (its other members enregistered)
tags: cpp:local cpp:struct | asm:sub asm:mov | topic:codegen-idiom topic:regalloc
symptoms: retail `sub esp,N` is LARGER than ours by a multiple of 4 and the extra dwords are never read or written; the slots retail DOES use are spaced 8 (or 16) apart with the gaps dead; every other byte matches
confidence: 8/10
variants: local-rect-aggregate-from-contiguous-frame-slots.md, stack-buffer-size-drives-frame.md

MSVC 5.0 reserves a local **aggregate as one block** even when the optimizer promotes
some of its members into registers — so an aggregate leaves *holes* in the frame. Four
scalars can never do that (a scalar that enregisters costs no slot at all). Therefore:
**dead slots interleaved at a regular stride ARE an aggregate**, and the stride tells you
its member layout. Map the used offsets onto the members whose values you can identify;
the ones in the holes are the members that got enregistered.

```cpp
// used: +0x00 and +0x08; +0x04 / +0x0c reserved and never touched -> ONE 16-byte
// rect, X members spilled, Y members living in eax/edx
WwdRect cell;                                            // the rect in CELL space
cell.m_minY = (y0 - m_bounds.m_minY) >> m_shiftX;         // +0x04  (enregistered)
cell.m_minX = (x0 - m_bounds.m_minX) >> m_shiftY;         // +0x00  (spilled)
cell.m_maxY = (y1 - m_bounds.m_minY) >> m_shiftX;         // +0x0c  (enregistered)
cell.m_maxX = (x1 - m_bounds.m_minX) >> m_shiftY;         // +0x08  (spilled)
```
```asm
sub    esp,0x20          ; 8 dwords, but only 6 are ever referenced
mov    DWORD PTR [esp+0x20],edx   ; cell.m_minX   (aggregate +0x00)
mov    DWORD PTR [esp+0x28],esi   ; cell.m_maxX   (aggregate +0x08)
;      [esp+0x24] / [esp+0x2c]    never read, never written  <- the tell
```
STEERABLE. `CWwdGrid::Query` @0x1918c0 99.72 -> **99.92**: four `i32 colA/rowA/colB/rowB`
scalars gave `sub esp,0x18` with all six slots live; the one `WwdRect` gives retail's
`sub esp,0x20` **and every slot number**, leaving only one scheduling pair. Corollary of
[[local-rect-aggregate-from-contiguous-frame-slots]]: there the tell is contiguous
ascending USED slots, here it is the dead ones between them.
