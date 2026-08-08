# Retail keeps a DEAD 8-byte `Coord` temp that no source spelling reproduces (OPEN)

tags: cpp:local cpp:struct | asm:sub asm:mov | topic:wall
symptoms: retail's whole frame is `sub esp,0x8`, written by exactly two `mov [esp+k],reg`
stores and **never read**; our base has no frame at all, so every `[esp+N]`, the register
assignment and one callee-saved `push`/`pop` all differ
confidence: 8/10 (the observation), 0/10 (a fix)

Three independent functions show the identical shape. In each, the source has (or clearly
had) a `Coord` local copied out of `X->m_lastTilePx`, and retail stores both halves into
an 8-byte frame it then never loads:

| fn | rva | the two dead stores |
|---|---|---|
| `CTriggerMgr::NotifyCell` | 0x79fb0 | `mov [esp+0x10],edx` / `mov [esp+0x14],edx` |
| `CTriggerMgr::ToggleRegionA` | 0x7d450 | `mov [esp+0x20],ecx` / `mov [esp+0x28],edx` |
| `CGrunt::ResolveArrivalNeighbor` | 0xf26f0 | `mov [esp+0x8],eax` / `mov [esp+0x1c],edx` |

`gruntz sema disasm <rva> --target --lite | grep esp` confirms each: `sub esp,0x8`, two
stores, one or more `add esp,0x8`, and no load.

## What has been ruled out (fast-probe matrix, cl 5.0 /O2 /MT /GX, 2026-08-08)

Nine spellings of an 8-byte struct local, all compiled in one probe TU; **cl deleted the
local in every one**:

1. `Pt t = o->p;` (copy-init)  2. field-by-field  3. a member call on the local so its
address escapes as `this`  4. a struct with a user-declared copy ctor  5. an inline free
helper taking the struct BY VALUE  6. a user `operator=`  7-9. the same three but with the
local actually READ afterwards (cl forwards the stores to the reads and deletes both).

Also ruled out on the live tree: an inlined by-value accessor
(`Coord GetLastTilePos() { Coord out; out.m_x = …; return out; }` -> `Coord pt = cell->GetLastTilePos();`)
changed nothing in `ToggleRegionA`.

So the temp is not any ordinary dead local. Whatever holds it alive in retail escapes its
address into something our reconstruction does not have, or retail's build ran a pass
ordering ours does not. **Do not fabricate a helper to force it** - a lane already tried
and got zero.

## Cost, so you can budget

It is not a rounding error: it moves one callee-saved register and every `[esp+N]` in the
function. `ToggleRegionA` is parked at 79.13 (from 75.40) and `NotifyCell` at 85.89
(MAX 87.15) with this as the only remaining mechanism, both otherwise block-exact.

If you crack it, say so loudly and update this file - it is worth at least three functions.

related: [shrink-wrapped-prologue-needs-one-tail-return.md](shrink-wrapped-prologue-needs-one-tail-return.md)
