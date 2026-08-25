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
  byte stores may alias it; `CMinimap::Draw` 0xa3820 reads
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
DOWN. **This is not one phenomenon** - a first reading filed three rows under a
single "callee-saved rebind evicts `this`" cause, and a controlled re-measure
found three different causes, one of which is not a colour question at all:

| function | bank | with the folded local | frame ours/retail | actual cause |
|---|---|---|---|---|
| `CMinimap::Draw` 0xa3820 | 76.38 @162 | **70.12** @158 | 0x14/**0x14** | callee-saved re-colour (below) |
| `CBoomerang::AdvanceMotion` 0xe08b0 | 86.25 @128 | **71.52** @138 | 0x28/0x20 | x87 spill temps - **no GPR is involved**, see x87-spill-slots-are-compiler-temps.md |
| `CRezImage::FlipVertical` 0x176840 | 71.07 @89 | 61.86 @88 | 0x14/0x18 | misfiled - it is a CFG/IV question (below) |

So: before spending a lever on "the colour", check whether the general-purpose
registers already pair with retail. In `AdvanceMotion` they do, in every state
tested, and the entire residue is x87 slot displacements.

### Where it IS the colour: exactly one of {receiver, local} keeps the register

`ComputeRect` reaches retail's instruction count AND retail's frame size only
with `CDDSurface* surf = m_surface;` (retail loads `+0x10` once; without the
local cl must reload it after the `m_srcRect = *src` store, which may alias).
But cl gives the callee-saved register to `surf` and homes `this`, where retail
has `this` in ESI and `surf` in ECX - a scratch register, legal because `surf`
dies at the `BltEx` push. Three levers, all measured:

* declaration position of the local (before/after the guard and the copy):
  **byte-identical**;
* `m_dstRect.left = ...` direct member stores (which retail provably uses -
  four `movl %reg, 0x34..0x40(%esi)`) instead of the `dstRect` pointer: flips
  ESI back to `this` and homes `surf` instead, 68.87, and the frame grows to
  0x18;
* shortening the local's range (spell `m_surface` at the `BltEx` argument):
  `this` returns to a callee-saved register - EBX, not retail's ESI - 70.71.

The pair is never reproduced. Same asymmetry, same direction, in two more rows:
`CBattlezMapConfig::RepathAroundBlockedTiles` 0x2a570 (retail homes `this` at
`[esp+0x14]` and `unit` at its parameter slot, re-reading `m_board` at three
sites; we keep `unit` in a callee-saved register) and `WarpTextureBlit`
0x146a20 (retail homes `minY` at `[ebp-0x8]` and gives ESI to the `src`
parameter; we enregister `minY`). **Retail spends its callee-saved registers on
the long-lived receiver/parameter and homes the derived local; our cl does the
reverse.** That is the detection signature, and no source lever found moves it.

### FlipVertical is not this pattern

Retail is 101 instructions to our 89 with TWO extra frame dwords: it reloads
`m_pixels` inside all three inner loops (the byte stores may alias the member)
and maintains `m_height - i` as a decrementing induction variable, from which
the middle loop derives its destination as `botOff + (i - counter + 1) * wid`.
Spelling the three loops as direct `m_pixels[off + x]` indexing does produce the
in-loop reloads, but flips loops 1 and 3 from retail's down-counted pointer walk
to an up-counted index loop (71.07 -> 61.86). It is a CFG/IV-selection row.

## What DOES convert

The dead-pair half converts when the site is a real by-value accessor call.
`CTriggerMgr::ApplyTriggerB` 0x6e120: both `LoadTileArrivalFx` argument pairs
plus the two `m_screenX`/`m_screenY` guards, spelled `cell->MoveTile().m_x` and
`cell->LastTilePx().m_x` instead of the direct member, took it 87.70 -> 91.05
and cleared its `walls valuetemp` TARGET-ONLY row. The sibling `ApplyTriggerA`
0x6dae0 looks identical in source and LOSES 0.27 the same way - retail compares
`cmp [esi+0x17c],ecx` there, straight off the member - so the accessor is a
per-site fact the sieve decides, never a spelling to apply by name.
