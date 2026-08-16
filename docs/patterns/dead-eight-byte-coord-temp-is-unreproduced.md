# The "dead 8-byte `Coord` temp" is a REGISTER-ALLOCATOR SPILL, not a source local

tags: cpp:local cpp:struct | asm:sub asm:mov | topic:wall topic:scoring-artifact
symptoms: retail's whole frame is `sub esp,0x8`, written by exactly two `mov [esp+k],reg`
stores and **never read**; our base has no frame at all, so every `[esp+N]`, the register
assignment and one callee-saved `push`/`pop` all differ
confidence: 9/10 (the diagnosis), 0/10 (a source-level fix - there is none)

**This file previously said the source "has (or clearly had) a `Coord` local" and asked
for a spelling that reproduces it. That framing was wrong and it cost three lanes.** The
two stores are a **spill pair the allocator emitted and then never needed**, because the
same two values also stay live in registers and every use reads them from there.

| fn | rva | the two dead stores |
|---|---|---|
| `CTriggerMgr::NotifyCell` | 0x79fb0 | `mov [esp+0x10],edx` / `mov [esp+0x14],edx` |
| `CTriggerMgr::ToggleRegionA` | 0x7d450 | `mov [esp+0x20],ecx` / `mov [esp+0x28],edx` |
| `CGrunt::ResolveArrivalNeighbor` | 0xf26f0 | `mov [esp+0x8],eax` / `mov [esp+0x1c],edx` |
| `CTriggerMgr::TriggerCell` | 0x7b1b0 | `mov [esp+0x28],ecx` / `mov [esp+0x30],edx` |
| `CTriggerMgr::ApplyTriggerA` | 0x6dae0 | `mov [esp+0x20],ecx` / `mov [esp+0x1c],eax` |
| `CTriggerMgr::ApplyTriggerB` | 0x6e120 | `mov [esp+0x24],ecx` / `mov [esp+0x20],edx` |
| `CTriggerMgr::ClearCell` | 0x6e800 | `mov [esp+0x10],edx` / `mov [esp+0x18],edx` (m_moveTile, not m_lastTilePx) |

In all three the slots are `[S-8]` and `[S-4]` (S = esp at entry), i.e. exactly the
`sub esp,0x8` area, and in all three they hold `x` then `y` out of `m_lastTilePx` - which
is what made a `Coord` local look obvious.

## The three facts that kill the `Coord`-local reading

**1. The "copy" is also in registers, and that is where every use reads it.**

```asm
; NotifyCell - x and y are spilled AND kept live, simultaneously
  mov  edx,[esi+0x17c]     ; x
  mov  eax,[esi+0x180]     ; y
  mov  [esp+0x10],edx      ; spill x        <- dead
  mov  ecx,edx             ; ...and keep x in ecx
  mov  edx,eax             ; ...and keep y in edx
  sar  eax,0x5             ; y>>5   (from the REGISTER)
  mov  [esp+0x14],edx      ; spill y        <- dead
  ...
  sar  ecx,0x5             ; x>>5   (from the REGISTER)
```

A source-level struct copy produces a store *or* a register copy, not both of each. Same
in `ToggleRegionA` (`mov [esp+0x20],ecx` then `push ecx`) and in
`ResolveArrivalNeighbor` (`mov [esp+0x8],eax` beside a second, un-CSE'd
`mov ecx,[esi+0x17c]` for the push). The un-CSE'd reload is the giveaway: that is
rematerialisation, which is an allocator decision, not an expression.

**2. cl uses `sub esp,0x8` for ordinary scalar spills all over the tree, and we already
match 39 of them.** Of the 75 retail functions whose entire frame is `sub esp,0x8`, **39
are 100% EXACT for us and 64 have the same `sub esp,0x8` in our base** - none of those
sources declares an 8-byte local. `CTriggerMgr::CycleMoveIcons` 0x7a690 is in the same
file, 100% EXACT, and its 8 bytes are the spill area for loop scalars.

**3. Nothing in the C++ language reaches it.** 38 probe cells, cl 5.0 `/O2 /MT /GX /GR`:
the nine originally documented (copy-init, field-wise, address escaped via `this`, user
copy ctor, by-value inline param, user `operator=`, and all three again with the local
READ) plus 29 more (2026-08-08): whole-struct copy-init tried **live** in `NotifyCell`;
by-value return from an inlined accessor for a POD, for a struct with a user ctor and for
one with a user copy ctor; in-place construction; a local array; writing through a
pointer to itself; a union; aggregate init; `*(double*)&p = ...`; declare-then-assign;
an inline out-parameter accessor (`GetPos(&p)`), the same returning the pointer, and free
inline fillers; a by-value struct argument to an out-of-line callee in first and middle
position, with and without the fields read afterwards; and passing by value to an inline
that uses it. **cl deleted the local in every one.**
**TU STATE IS ALSO RULED OUT (2026-08-08).** The one axis the source probes could not
reach - the TU declaration count that re-colours other functions
([declaration-count-window-steers-regalloc.md](declaration-count-window-steers-regalloc.md))
- is dead flat here. 50 `tu_state_*` island cells per function
(the retired permuter), across all ten families:

| fn | rva | cells | result |
|---|---|---|---|
| `CTriggerMgr::NotifyCell` | 0x79fb0 | 50 | 50/50 identical at 85.892 |
| `CTriggerMgr::ToggleRegionA` | 0x7d450 | 50 | 50/50 identical at 79.133 |
| `CGrunt::ResolveArrivalNeighbor` | 0xf26f0 | 50 | 50/50 identical at 86.608 |

A user-declared `Coord` copy ctor was re-tried on the live tree and does not even build -
it makes `Coord` non-POD and `GruntWanderStep.cpp:257` then fails C2362 (`goto ph1` skips
the initialization of `cp`), which is itself weak evidence that retail's `Coord` was a POD.

One real gain came out of the re-audit: in `ResolveArrivalNeighbor` the local was copied
and then IGNORED, with `CommitNeighbor` re-reading `occ->m_lastTilePx` - passing
`tile.m_x, tile.m_y` instead took it 81.10 -> 86.61 (size 222 -> 237, retail 262). The
local is still elided; the remaining 25 bytes are the frame and the two stores.

## Cost, so you can budget

Exactly three things keep the frame, and each is refuted by retail's own bytes:

| construct | keeps the 8 bytes? | why it is not the answer |
|---|---|---|
| a real out-of-line call taking `&p` | yes | emits a `call` retail does not have |
| a user-declared destructor | yes | drags in the `/GX` EH frame (`push -1`), which retail does not have |
| **`volatile` on the local** | yes - **byte-for-byte retail's idiom**, stores interleaved into the argument pushes | forcing a store is the definition of `volatile`; it proves the residue is a DSE/allocation decision, not a missing declaration |

The `volatile` cell is the useful one *as a diagnostic*: it reproduces the shape exactly,
which means what retail has is "a store that was not eliminated" - not "a local we failed
to declare". **Do not put `volatile` in the tree to buy the bytes.**

## What to do with these three functions

Treat them as an ordinary **regalloc wall** and hand them to the permuter, not to another
source-construct hunt. They are otherwise block-exact; the residue is one callee-saved
register and every `[esp+N]`. If a variants run flips the allocation, it flips all of it
at once.

`NotifyCell` is parked at 85.89 (MAX 87.15).

## 2026-08-10 additions

**Two more probe families died** (they are not in the 38 above): a by-value inline
accessor `CGrunt::LastTilePx()` added to `Grunt.h` for exactly this purpose, and an
explicit `Coord() {}` default constructor on `Coord` itself (tried live in `NotifyCell`).
cl elided the local in both, and the `Coord()` cell does now build - the C2362 that
blocked the copy-ctor cell does not apply to a default ctor - so that avenue is closed on
evidence rather than on a build error.

**The TU-declaration-count axis is dead for the whole unit, not just per function.** A
16-cell sweep of throwaway prototypes above `TriggerMgr.cpp`'s first include moved
**none** of the unit's 21 sub-100 functions (only `PlaceObjectFull` wobbled 72.27/72.33
on parity). The same sweep on `TriggerMgrHitTest.cpp` left `TmDeflectStep` 0x6f2f0 flat at
99.4577 across 13 cells.

**But the ARGUMENT-EVALUATION ORDER around one of these sites is reachable, and it is
worth more than the two dead stores.** `ToggleRegionA` 0x7d450 went **79.13 -> 86.98** by
spelling the ResetGroup call's coordinate source as a two-argument `Coord::Set` instead of
a struct copy:

```cpp
// 79.13 - a struct copy loads m_x then m_y, retail loads m_y first
Coord pt = cell->m_lastTilePx;
// 86.98 - Set(x, y) evaluates right-to-left, which is retail's load order
Coord pt;
pt.Set(cell->m_lastTilePx.m_x, cell->m_lastTilePx.m_y);
```

The 8-byte frame and its two stores are still missing; everything else in the call setup
now matches. So the rule stands - the stores are allocator residue - but **check the
member LOAD ORDER before parking one of these**: `m_y` ahead of `m_x` means a two-argument
call, not an aggregate copy. The same rewrite in `TriggerCell` 0x7b1b0 fixed the load
order but scored 91.19 -> 89.73 and was reverted, so adjudicate per site.

related: [shrink-wrapped-prologue-needs-one-tail-return.md](shrink-wrapped-prologue-needs-one-tail-return.md)
