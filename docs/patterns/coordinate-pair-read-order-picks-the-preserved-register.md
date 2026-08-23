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

STEERABLE. Read retail's FIRST load of the pair and match the declaration order; the
argument order at the call site is unchanged, so this is byte-neutral in meaning.
`CProjectile::AdvanceMotion` 0xdfd00 93.7714 -> 96.0192 (both duplicated `CreateSprite`
tail blocks fell into line at once); `CTriggerMgr::ApplyTriggerB` 0x6e120 85.2268 ->
85.2354. NEGATIVE control: it does NOT apply when the CONSUMER fixes the order —
`CGrunt::RectContains` 0x51850 reads the pair for `r1.left += dx` before `r1.top += dy`,
so declaring `dy` first is reordered straight back and the row DROPS 83.02 -> 78.86.
Check that the consumer does not already pin the order before spending a build.
