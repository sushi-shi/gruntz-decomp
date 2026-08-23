# A dead stack store in retail is NOT an era anomaly — two cl 5.0 SP3 mechanisms produce them

tags: cpp:local cpp:struct cpp:member cpp:temporary | asm:mov | topic:codegen-idiom topic:wall
symptoms: retail writes an aggregate's fields from members and overwrites them a few
instructions later with no read in between, the base emits nothing there, and the row's
whole residue is exactly those extra loads + stores; the reflex is to call it the
RTM-provenance family
confidence: 10/10
variants: dead-second-field-load-is-a-struct-copy.md

Retail keeping a provably dead store looks like the era-compiler family, so lanes park it.
cl 5.0 SP3 emits them from ordinary source, by two mechanisms that are both steerable.
Neither is a liveness rule: **a plain local's dead field store IS eliminated**, whether the
store group is fully dead, half dead, killed field-wise or killed by an aggregate assign.

**1. The unread half of a by-value struct return.** An inlined accessor returning a
two-field struct materialises a frame temp. The half whose field is READ gets folded into
its consumer (cl re-loads the source member instead), and the UNREAD half's store is
emitted and left dead. Two calls therefore leave two dead stores, high slot first, in
whatever slots the temp is later given.

```cpp
Coord LastTilePx() { return m_lastTilePx; }   // in the class, like the real EntrancePx

i32 dx = LastTilePx().m_x >> TILE_SHIFT_PX;   // call 1: .m_y's store is dead
i32 dy = LastTilePx().m_y >> TILE_SHIFT_PX;   // call 2: .m_x's store is dead
```
```asm
mov  edx,[ecx+0x180]      ; m_y, for call 1's unread half
...
mov  [esp+0x14],edx       ; temp+4 = m_y     DEAD, later r1.top
mov  edx,[ecx+0x17c]      ; m_x, for call 2's unread half
mov  [esp+0x10],edx       ; temp+0 = m_x     DEAD, later r1.left
mov  edx,[ecx+0x180]      ; m_y again, this one feeds dy
```

**2. A load through a pointer read from a GLOBAL blocks the elimination.** cl 5.0
disambiguates a stack local against a load through a POINTER PARAMETER and deletes the
earlier store; through a pointer whose value came out of a global it does not, so every
field store issued before that load survives. This is what keeps `SaveScreenshot`'s
`srcRect.right = 0;` alive: `mgr = g_gameReg`.

STEERABLE both ways. `CGrunt::RectContains` 0x51850 83.02 -> **100.00 EXACT** and
`CGrunt::RectContainsGated` 0x51a20 82.53 -> **100.00 EXACT** on mechanism 1 (plus reading
the tile delta before normalising the arguments, which is what puts x in esi and y in ebp).
Controls, all with the local's address escaping to a real call: one rect / two rects,
killed field-wise / by aggregate assign, zeros / member values — dead stores ELIMINATED in
every case (so "a partially-live store group is retained whole" is REFUTED: `r.left=0;
r.top=0; r.right=0; r.bottom=0; r.right=w; r.bottom=h;` keeps four stores, not six).
Same source with the killing value loaded through a pointer from a global — RETAINED, one
rect and two. Corrects [[dead-global-read-spill-dce]] and this file's own earlier claim
that a fully dead store group is simply unreachable.
