# The tile shift is written BACK into the Coord, not applied at the use site
tags: cpp:local cpp:method | asm:sar asm:mov | topic:codegen-idiom
symptoms: `insn_seq --hist` shows retail with many more `sar` AND many more `mov` than us; the retail disasm has `sar reg,0x5` immediately followed by `mov [esp+N],reg` where N is inside the `Coord` that `GetScreenPos` just filled; our call site reads `f(c.m_x >> TILE_SHIFT_PX, c.m_y >> TILE_SHIFT_PX, ...)`
confidence: 9/10

`CUserLogic::GetScreenPos(&c)` fills a pixel `Coord`; the callers want tile coords. There
are two spellings and they are NOT byte-equivalent:

```cpp
// ours - shift at the use site.  The Coord's slots keep the PIXEL values and cl
// never writes the shifted ones anywhere; with several uses it also CSEs the shift.
f(c.m_x >> TILE_SHIFT_PX, c.m_y >> TILE_SHIFT_PX, ...);

// retail - shift IN PLACE, then pass the members.  Two extra `mov`s that home the
// shifted values into c's own two dwords, and one `sar` per component per site.
c.m_x = c.m_x >> TILE_SHIFT_PX;
c.m_y = c.m_y >> TILE_SHIFT_PX;
f(c.m_x, c.m_y, ...);
```

The tell is unambiguous in the prologue - the store target is the same slot the
`lea`/`call GetScreenPos` pair just used:

```asm
lea  eax,[esp+0x18]           ; &tp
call <GetScreenPos>
mov  ecx,DWORD PTR [esp+0x1c] ; tp.m_y
mov  eax,DWORD PTR [esp+0x18] ; tp.m_x
sar  ecx,0x5
sar  eax,0x5
mov  DWORD PTR [esp+0x24],ecx ; -> tp.m_y   (esp is 8 lower here: 0x24-8 = 0x1c)
mov  DWORD PTR [esp+0x28],eax ; -> tp.m_x   (esp is 16 lower: 0x28-16 = 0x18)
```

## The by-value helper is the same bug, amplified

A `static __inline Coord ScreenTile(CUserLogic*)` that does the shifts and `return c;`
looks like the in-place form but is not: cl applies NRV to the by-value return, writes
straight into the destination and then folds the shifts across the expansions. Retail
homes BOTH the temp and the destination. Expand the helper at every site and delete it
(the usual [`shared-tail-helper-is-our-invention-expand-it`](shared-tail-helper-is-our-invention-expand-it.md)
rule; a by-value `Coord` return is our invention, not a period idiom).

## Declaration order is observable inside a rectangle cluster

When four `Coord`s supply one field each to a rectangle, recover their construction order
from retail's calls and write-backs rather than declaring them in rectangle field order.
`StepDefenderUnit` constructs the bottom, right, top, and left sources (`b3`, `b2`, `b1`,
`b0`) in that order. Each `GetScreenPos` call is immediately followed by the shifts whose
results survive: both components for the first three objects, but only `m_x` for the final
object because its `m_y` is dead. The same order repeats for the second rectangle cluster.
Moving all four calls before all shifts changes both entity lifetimes and register creation
order even though the resulting `RECT` has the same values.

**Evidence.** `CBattlezMapConfig::StepDefenderUnit` @0x33520 first read `sarl 22 -> 34`
and `movl 314 -> 368`. Deleting the invented `ScreenTile` helper, expanding its eight sites,
and converting the remaining use-site shifts took it **66.96 -> 70.81**. Scoping completed
`Coord` groups then reduced the candidate frame from `0x8c` to `0x60` against retail's
`0x58`. Finally, restoring the retail-observed `b3, b2, b1, b0` and `d3, d2, d1, d0`
construction/write-back order restored the complete `sar` census and raised the full-build
current-source result to **77.60%**. The candidate has 947 instructions against retail's
955 and preserves all 81 ordered referents. The remaining residue is not evidence for
keeping all eight objects live together.
