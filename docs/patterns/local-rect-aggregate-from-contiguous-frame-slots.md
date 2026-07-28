# Four bbox scalars at CONTIGUOUS ascending frame slots are ONE local `RECT`
tags: cpp:local cpp:struct | asm:mov asm:cmp | topic:codegen-idiom topic:regalloc
symptoms: min/max bbox spilled to the stack, esp+N/N+4/N+8/N+0xc used as left/top/right/bottom, "spill-slot permutation" wall, frame size off by 4, compiler temps sit ABOVE the user scalars
confidence: 9/10
variants: struct-copy-block-store-base-reg.md, sib-base-index-follows-local-decl-order.md

A bounding-box fold written as four scalars (`minX/minY/maxX/maxY`) spills to four
frame slots in whatever order the allocator picks, and the compiler temps land at
the TOP of the frame. Retail instead shows the four values at **contiguous,
ascending** offsets in exactly `left, top, right, bottom` order, with the temps
BELOW them — that is the signature of a named `RECT` local, not four scalars.
Read the slot numbers off the min/max compares and the midpoint arithmetic; if
they are `N, N+4, N+8, N+0xc` and the min-x one is lowest, it is a `RECT`.

```cpp
// NO - four scalars: the allocator permutes the slots and puts them under the temps
i32 maxX = 0, maxY = 0;
i32 minX = grid->m_wrapW - 1, minY = grid->m_wrapH - 1;

// YES - one named aggregate: cl homes it as a block, temps go below it
RECT bbox;
bbox.right  = 0;
bbox.bottom = 0;
bbox.left   = grid->m_wrapW - 1;
bbox.top    = grid->m_wrapH - 1;
...
if (x < bbox.left)   bbox.left   = x;
if (x > bbox.right)  bbox.right  = x;
if (y < bbox.top)    bbox.top    = y;
if (y > bbox.bottom) bbox.bottom = y;
ResetGoals(bbox.left + (bbox.right - bbox.left) / 2,
           bbox.top  + (bbox.bottom - bbox.top) / 2);
```
```asm
; the tell - ascending, 4 apart, min-x lowest, and the temps are BELOW at 0x10/0x14
mov  [esp+0x20],edi        ; bbox.right  = 0
mov  [esp+0x24],edi        ; bbox.bottom = 0
mov  [esp+0x18],ecx        ; bbox.left   = w-1
mov  [esp+0x1c],eax        ; bbox.top    = h-1
...
cmp  eax,[esp+0x20]        ; x > bbox.right
```
STEERABLE. Members of the aggregate can still be enregistered individually (in
`CenterOnGroup` three of the four colour into edi/ebp/eax and only `right` stays
in memory — the slot is allocated anyway, which is what makes the frame the right
size). `CTriggerMgr::CenterSelectionGroup` 99.85 -> **100.00 EXACT** (was filed as
an "esi<->ebp regalloc wall"), `CTriggerMgr::CenterOnGroup` 83 -> 98.8.
