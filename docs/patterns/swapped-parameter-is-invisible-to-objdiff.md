# A swapped PARAMETER moves one displacement byte — objdiff cannot see it, so read the argument SLOT at every call

**Tags:** cpp:call | asm:call | topic:runtime-defect topic:scoring-artifact

## Symptom

A function scores in the low 90s, its `--diff` residue reads as ordinary
scheduling, every gate is green — and the feature it implements is completely
broken at runtime.

Measured 2026-08-11: `CDDSurface::SaveRle16` (0x144640) scored **91.82%** while
**every explicit save in the game failed**.

## Cause

Passing the wrong parameter changes ONE displacement byte:

```asm
mov  eax,DWORD PTR [esp+0x78]     ; retail: argument 0  (path)
mov  eax,DWORD PTR [esp+0x7c]     ; ours:   argument 1  (pal)
```

objdiff scores that as a single mismatched instruction inside a 702-byte
function. There is no structural signal at all: same block count, same branch
sequence, same call graph, same archive-slot sequence (`serial_io` passes),
identical `ret 0xN`. The bug is a *value* choice, and value choices are exactly
what the percentage is worst at seeing.

```cpp
// WRONG - and 91.8% "correct"
i32 CDDSurface::SaveRle16(void* path, void* pal, i32 flag) {
    if (path == NULL) return 0;                       // guards `path` ...
    file.Open(static_cast<char*>(pal), 0x2001, 0);    // ... and opens `pal`
```

The one caller chain passes `pal = 0`, so `CFile::Open(NULL, ...)` failed every
time, `SaveScreenshot` returned 0, and `CSaveGame::Save` returned 0 **after**
`SaveGame` had already written a complete snapshot. The `SlotN.sav` on disk is a
full, valid save; the index row keeps `type = 0`, so the save list shows
"(Empty)" and quicksave reports "ERROR - Cannot Save Game."

## Detection — read the slot, by hand

There is no cheap mechanical sieve for this (see below). The reliable move is:

1. `gruntz sema disasm <rva> --lite` and compute the frame depth from
   the **epilogue** (the pops plus the final `add esp,N`) — never the prologue,
   because MSVC hoists a callee-saved `push` past a branch and threads the /GX
   `mov fs:0,ecx` between the epilogue pops.
2. Return address is at `esp+frame`; argument *i* is at `esp+frame+4+4*i`,
   adjusted by any argument `push`es made since.
3. Compare, per call site, which argument index each side loads.

A truncated output FILE is the loudest instance of this class: diff the tail of a
failing artifact against a working one. Slot3.sav stopped exactly where the
230,458-byte preview should begin, with its whole 4,408-byte POSTSAVE tail intact
— which said "the snapshot succeeded, the step AFTER it failed" in one command.

## Why the obvious sieve does NOT work (do not rebuild it)

A tool that walks both instruction streams and compares which incoming argument
slots each side reads was built and **withdrawn**: MSVC recycles a **dead
parameter home as a spill slot**, so "touches argument N's slot" is not "uses
parameter N" — retail's own `SaveRle16` writes `pal`'s home to park a temp.
Filtering to "first access is a read" then makes the answer depend on linear
block order, and the detector stops firing on the very bug it was built for.
Separating a spill from a use needs real dataflow, not a linear scan.

Wall for tooling, hand-fixable in source: the fix is a one-token source change
and it raised SaveRle16 91.82 -> 91.90. Sibling writers `SaveBmp` and `SaveTga`
open `path` correctly — only this one slipped.

## Outgoing variant — read the pushed argument vector

Measured 2026-08-22: `CGrunt::FinishActiveAction` (0x6a6d0) had the same
runtime-defect signature on an outgoing call. Its knockback (`"O"`) cleanup
snapped the Grunt to `m_lastTilePx`, then called:

```cpp
// WRONG: rewires the transposed map cell.
m_tileMgr->WireTileSwitchLogic(this, m_lastTilePx.m_y, m_lastTilePx.m_x);
```

The multiset-style semantic diff reported no exclusive displacement, store,
constant, or referent difference: both sides still read `+0x17c` and `+0x180`
once and call the same function. The ordered argument vector was different.
For the three stack arguments of the `__thiscall` callee, retail emits:

```asm
mov  edx,[esi+0x180]       ; y
mov  eax,[esi+0x17c]       ; x
push edx                   ; rightmost argument: y
push eax                   ; x
push esi                   ; grunt
call WireTileSwitchLogic
```

The wrong source pushed `x`, then `y`, then the Grunt, making the callee see
`(grunt, y, x)`. Correcting the call to `(this, m_lastTilePx.m_x,
m_lastTilePx.m_y)` makes the rebuilt object push the same `(y, x, grunt)`
sequence as retail. Fuzzy moved slightly down, 89.2566 -> 89.2554, despite the
call protocol becoming correct.

This arm finalizes an interrupted knockback after the old cell has already been
released and the destination cell claimed. Rewiring `(y, x)` can therefore
leave the Grunt's logical cell and map ownership out of agreement with its
snapped screen position. The reverse-audit trigger is an in-game entity stuck
after an interrupted action, sometimes with ownership/colour damage, while the
responsible function has identical call, branch, displacement-multiset, and
referent counts. At every coordinate-bearing call, reconstruct the ordered
arguments from the final stack layout; do not accept a matching operand
multiset as proof.
