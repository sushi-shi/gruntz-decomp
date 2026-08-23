# A switch whose arms don't all assign an output is a RUNTIME defect roulette, not a wall

tags: cpp:switch cpp:local | asm:mov asm:jmp | topic:runtime-defect topic:codegen-idiom
symptoms: a per-arm-assigned local read after the switch; our build reads a
never-written `[esp+N]` slot on some arms while retail's table entry lands
mid-block with the value still live in a register; in game: gruntz teleport
off-map / die on due-N or due-S steps while diagonal steps work
confidence: 9/10

Found live in `CGrunt::ClaimSwitchTile` (0x52c70). The original source assigns
BOTH output coordinates in EVERY switch arm from the same base term
(`nextX = tile.m_x + 0x20; nextY = tile.m_y - 0x20;` ...), and for the arms
where one coordinate is unchanged it still assigns it (`nextX = tile.m_x;`).
cl 5.0 CSEs the unchanged term onto the preloaded register and lands the
N/S jump-table entries MID-BLOCK, past the sibling arm's x-step (retail
N -> +0x4d, S -> +0x40): the emitted arm only adjusts y and x stays live in
ebx. Reading that shape back as a FALL-THROUGH spelling —

```cpp
case DIR_SOUTHWEST: nextX = tile.m_x - 0x20;  // fall through
case DIR_SOUTH:     nextY = tile.m_y + 0x20;  break;
```

— is the trap: it leaves `nextX` formally uninitialized in the pure-S arm, and
OUR cl homed `nextX` to a stack slot no path writes, so a due-south claim read
garbage. Depending on the residue that slot held, the claim either failed
spuriously (out-of-bounds `CellFlagsAt` -> the act-M caller kills the grunt via
`CellDispatch(DEATH_NORMAL)`) or committed `m_lastTilePx.m_x = garbage` and the
grunt walked off-map. Retail compiled the same VALUES but kept them in
registers, so retail never misbehaved: **identical-looking source, divergent
runtime, and objdiff cannot see it** (the read of the uninit slot is a
well-formed instruction that simply has no retail counterpart).

## Detection (tree-wide, minutes)

clang's dataflow warnings run fine over the MSVC5 tree via the clangd config:

```sh
# per entry of build/clangd/compile_commands.json, replace /c with:
clang-cl <args> -fsyntax-only -Wuninitialized -Wsometimes-uninitialized \
    -Wconditional-uninitialized
```

`-Wsometimes-uninitialized` hits are high-confidence; `-Wconditional-` hits
need triage (loop-entry proofs clang cannot do, and retail-faithful UB). On the
2026-08-24 sweep of the movement TUs the only LIVE divergence was
ClaimSwitchTile; `CTriggerMgr::LoadCameraSprite` (dock-position default arm) and
`CTriggerMgr::EnqueueGroupCells` (`x` latched outside the owner test) are EXACT
matches, i.e. retail's own quirks — model them as-is.

## Disposition

- Reconstructing: spell every arm with full assignments (the per-arm CSE form
  above); never the fall-through form that leaves an output unwritten.
- Reviewing an `@early-stop`: an extra `mov [esp+N],reg` per arm plus a
  register/stack home difference against retail is this pattern, and the fix is
  the spelling, not a regalloc lever.
- The no-default switch over a closed enum domain (dir 1..8) is faithful:
  retail's own out-of-range path reads uninitialized slots too. Do not invent a
  default arm for it.
