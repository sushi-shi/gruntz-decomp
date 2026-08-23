# A folded local: name the frame slot ROLES, not the frame SIZE — and expect the register re-colour
tags: cpp:local cpp:scope | asm:sub-esp asm:mov asm:fild | topic:regalloc topic:codegen-idiom topic:wall
symptoms: `gruntz walls framescan --todo --folded` puts the row in the "retail
reserved more than we did" list; retail's `sub esp,N` is 4-20 bytes larger and
its instruction count is HIGHER; retail loads a member once where we load it
three times, or homes a value we keep in a register
confidence: 8/10 (the method), 4/10 (that fixing it raises the score)
variants: frame-size-sieve-and-its-false-positives.md,
dead-eight-byte-coord-temp-is-unreproduced.md

Retail's frame being LARGER than ours means retail HOMED something we never
declared. The sieve ranks by the size delta, but the size is not the evidence -
**the slot ROLES are**. Read what each side's slots hold, in order, and the
missing local names itself.

```cpp
// CGruntzMgr::RecomputeViewScale 0x8f7f0, 85.11 -> 90.08.  Retail homes BOTH
// integer spans before the first __ftol; the lazily-declared `fh` let cl sink
// `ih`, which kept `top` and `bottom` in two extra callee-saved registers
// across the call - four pushes where retail has three.
i32 iw = ext.right - ext.left + 1;
i32 ih = ext.bottom - ext.top + 1;
float fw = static_cast<float>(iw);
float fh = static_cast<float>(ih);          // NOT after the first store
view->m_rectA.w = static_cast<i32>(fw * 1.4f);
view->m_rectA.h = static_cast<i32>(fh * 1.4f);
```
```asm
; retail: two spans homed in two slots, then one conversion each
mov  [esp+0x10],eax      ; iw
sub  ecx,ebx
fild DWORD PTR [esp+0x10]
inc  ecx
mov  [esp+0x14],ecx      ; ih  <- the slot the sunk source never allocates
fst  DWORD PTR [esp+0xc] ; fw stays in st(0) for the multiply
```

## The screen that names the local

Two read-side signals, both keyed on the member displacement (a base register
is not comparable across sides, a displacement is):

* **a member we read MORE times than retail** - retail cached it in a source
  local. `CRezImage::FlipVertical` 0x176840 reads `m_height` (+0x43c) three
  times to retail's two, and the third read is INSIDE the row loop where the
  byte stores may alias it; `CLightFxRender::ComputeRect` 0xa3820 reads
  `m_surface` (+0x10) FIVE times to retail's one. `gruntz walls reloadscan
  --loop` already fires on the first (`+0x43c ours 1 retail 0`); the
  straight-line case has no shipped channel.
* **a frame slot retail WRITES and never READS** - the by-value accessor temp.
  That one IS shipped, as `gruntz walls valuetemp`, and its TARGET-ONLY list is
  the actionable form; a hand-rolled adjacency screen re-derives it with two
  false-positive classes the shipped verb does not have (outgoing-argument
  staging written at two esp depths reads as an adjacent pair, and two
  consecutive scalar initialisations that cl also keeps in registers read as a
  Coord).

## The trap: the fix is right and the score still falls

Adding the proven local converges the INSTRUCTION MULTISET and moves the score
DOWN, because cl then binds the new local to a callee-saved register and evicts
`this` (or the loop cursor) into the frame, which shifts every operand byte.
Measured three times in one session:

| function | bank | with the folded local | instructions (ours/retail) |
|---|---|---|---|
| `CLightFxRender::ComputeRect` 0xa3820 | 76.38 @160 | **70.12** @156 | 156/156 - EXACT |
| `CBoomerang::AdvanceMotion` 0xe08b0 | 86.25 @128 | **71.52** @132 | 132/132, every `walls semdiff` key equal, byte length equal |
| `CRezImage::FlipVertical` 0x176840 | 71.07 @87 | **56.78** @93 | 93/100 |

`AdvanceMotion`'s is the sharpest: with `m_phase` updated after both position
stores (retail's store order is `m_posX`, `m_posY`, `m_phase`) and the rotation
parenthesised so the subtraction precedes the origin add, the object is
retail's byte length with retail's exact instruction multiset and a pure x87
schedule permutation - and it scores 15 points lower than the shape that is
four instructions short. **So a converging multiset is not a converging score,
and the MAX gate will refuse the correct shape.** Bank the number the tree can
reach, record the shape, and do not read the drop as a falsification.

## What DOES convert

The dead-pair half converts when the site is a real by-value accessor call.
`CTriggerMgr::ApplyTriggerB` 0x6e120: both `LoadTileArrivalFx` argument pairs
plus the two `m_screenX`/`m_screenY` guards, spelled `cell->MoveTile().m_x` and
`cell->LastTilePx().m_x` instead of the direct member, took it 87.70 -> 91.05
and cleared its `walls valuetemp` TARGET-ONLY row. The sibling `ApplyTriggerA`
0x6dae0 looks identical in source and LOSES 0.27 the same way - retail compares
`cmp [esi+0x17c],ecx` there, straight off the member - so the accessor is a
per-site fact the sieve decides, never a spelling to apply by name.
