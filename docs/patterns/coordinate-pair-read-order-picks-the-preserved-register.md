# Retail reloads `+0x17c` where we reload `+0x180` — the pair's READ ORDER picks which half survives

tags: cpp:local cpp:member cpp:inline | asm:mov asm:sar | topic:codegen-idiom topic:regalloc
symptoms: retail and the base read the SAME two adjacent members of an x/y pair, but at a
LATER site the two sides reload opposite halves (`mov eax,[esi+0x17c]` vs
`mov ecx,[esi+0x180]`); the two loads that feed one inlined callee appear in swapped
order; a `mov <reg>,<reg>` copy of one half exists on both sides but of a different half
confidence: 9/10

When a coordinate pair (`Coord m_x/m_y`, `m_targetX/m_targetY`) feeds ONE inlined callee
that consumes both halves, cl loads both, copies ONE of them and shifts the OTHER in
place. The copied half is the FIRST one read, and because it is the half that survives the
region it is the one that gets the call-crossing callee-saved register — so every later use
site reloads the *other* half. Swapping the two locals' declaration order flips the whole
chain, including sites hundreds of bytes away.

```cpp
// retail loads m_targetY first, so declare it first
i32 tileY = m_targetY >> TILE_SHIFT_PX;
i32 tileX = m_targetX >> TILE_SHIFT_PX;
u32 flags = plane->CellFlagsAt(tileX, tileY);
```
```asm
; target                          ; base before the swap
mov  edi,[esi+0x180]              mov  edi,[esi+0x17c]
mov  eax,[esi+0x17c]              mov  edx,[esi+0x180]
mov  edx,edi        ; copy y      mov  ecx,edi        ; copy x
sar  eax,5          ; x in place  sar  ecx,5
...                               ...
mov  eax,[esi+0x17c] ; reload x   mov  ecx,[esi+0x180] ; reload y
```

STEERABLE, and NARROW. Read retail's FIRST load of the pair and match the declaration
order; the argument order at the call site is unchanged, so this is byte-neutral in
meaning. `CProjectile::AdvanceMotion` 0xdfd00 93.7714 -> 96.0192 (both duplicated
`CreateSprite` tail blocks fell into line at once); `CTriggerMgr::UseToyAt` 0x6e120
85.2268 -> 85.2354.

**THE READ-ORDER DIFFERENCE IS NOT THE WORKLIST — THE COPY SHAPE IS.** A tree-wide census
of every function's adjacent-offset pairs finds 10311 sites where both sides read the same
half first and 181 where they disagree (100 retail-low-first, 81 retail-high-first) — but
a disagreement is almost always a CONSEQUENCE of a different allocation, not a cause. Sieve
for the shape in the asm block above instead — both halves loaded close together, one
COPIED register-to-register and the other modified in place — and require base and target
to copy OPPOSITE halves. That population is 4 sites against 52 agreeing controls, i.e. the
vein is essentially drained by AdvanceMotion. A detector that fires on neither known-good
row is measuring nothing: check it against `AdvanceMotion` +0x17c before trusting a zero.

NEGATIVE controls, all paid for:
* the CONSUMER can pin the order — `CGrunt::RectContains` 0x51850 reads the pair for
  `r1.left += dx` before `r1.top += dy`, so declaring `dy` first is reordered straight
  back and the row DROPS 83.02 -> 78.86; `CGrunt::ConsiderArrival` 0x52f40 has the pure
  register-swap signature at `m_screenX`/`m_screenY` and six spellings (declaration order
  both ways, the condition inlined, no `Coord` copy, the pair declared before the tile
  read, a `Coord` destination, the member read without a local) compile byte-identically.
* the swap can move something ELSE — the four `CTileTriggerSwitchLogic` switch bodies read
  `m_tileY` before `m_tileX` where retail reads `m_tileX` first; swapping the two
  declarations left the pair order UNCHANGED and hoisted the `g_gameReg` load above the
  prologue instead, 93.60 -> 84.97/83.30 and 99.14 -> 71.00/71.00. Verify the pair order
  actually flipped before reading a score.
* a bare statement move is inert — `CBattlezMapConfig::IsCoordOccupied` 0x305b0 with
  `i32 i = 0;` after the loop-base computation is byte-identical.
