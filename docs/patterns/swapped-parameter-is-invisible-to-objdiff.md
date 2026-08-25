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

The reliable manual move is:

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

## Why the obvious linear sieve does NOT work (do not rebuild it)

A tool that walks both instruction streams and compares which incoming argument
slots each side reads was built and **withdrawn**: MSVC recycles a **dead
parameter home as a spill slot**, so "touches argument N's slot" is not "uses
parameter N" — retail's own `SaveRle16` writes `pal`'s home to park a temp.
Filtering to "first access is a read" then makes the answer depend on linear
block order, and the detector stops firing on the very bug it was built for.
Separating a spill from a use needs real dataflow, not a linear scan.

The replacement is the project-neutral `decomp-param-roles` tool in the sibling
`decomp-generic-tools` repository. Its CFG interpreter versions stack memory:
an incoming home starts with its parameter identity, and a store replaces that
identity before any later reload. It traces the surviving value into member
paths, method receivers, comparisons, returns, scaled dereferences, and ordered
outgoing arguments. Repeated pointer/reference parameters form one default ABI
group and repeated four-byte scalars another; adapters may narrow the groups to
exact canonical types. Whole-role matching searches cycles of three or more as
well as pair swaps.

Wall for tooling, hand-fixable in source: the fix is a one-token source change
and it raised SaveRle16 91.82 -> 91.90. Sibling writers `SaveBmp` and `SaveTga`
open `path` correctly — only this one slipped.

## Outgoing variant — read the pushed argument vector

Measured 2026-08-22: `CGrunt::FinishActiveAction` (0x6a6d0) had the same
runtime-defect signature on an outgoing call. Its knockback (`"O"`) cleanup
snapped the Grunt to `m_lastTilePx`, then called:

```cpp
// WRONG: rewires the transposed map cell.
m_triggerMgr->WireTileSwitchLogic(this, m_lastTilePx.m_y, m_lastTilePx.m_x);
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

## Incoming whole-body variant — two same-typed object parameters trade roles

Measured 2026-08-24: `CBattlezMapConfig::HandleUnitContact` (0x2ae00) had two
`CGrunt*` parameters and the same calls, CFG, referent sequence, and aligned
member offsets as retail. The source nevertheless gave the two parameters the
opposite identities throughout the body. After `sub esp,0x34; push ebx; push
ebp`, retail loaded the second argument from `[esp+0x44]`; the candidate loaded
the first from `[esp+0x40]`. After the later `push esi; push edi`, retail loaded
the first argument from `[esp+0x48]`; the candidate loaded the second from
`[esp+0x4c]`.

That inversion made the final call semantically
`second->CommitNeighbor(first identity, first position)` where retail does
`first->CommitNeighbor(second identity, second position)`. `StepRowUnits`
passes `(unit, other)`: `unit` is on its saved tile pixels, while `other` can be
partway through a diagonal move. Rotating `other` to a cardinal direction in
the middle of that move zeroes one still-unfinished axis, producing a Grunt
that remains permanently off-axis. Retail rotates the stationary `unit`, so it
does not corrupt motion.

Binding the source roles as `(actor, other)` made both decisive loads and the
complete `CommitNeighbor` setup instruction-identical to retail. Fuzzy barely
moved, to 86.17%; the proof is the incoming stack slots and receiver dataflow,
not the score. This is outside the outgoing call-order sieve below: every
caller already passed `(unit, other)` in retail order, and the callee assigned
the meanings incorrectly. When a function takes repeated same-typed pointer
parameters, compare which incoming slot feeds each field-dereferencing receiver
on both sides; a clean outgoing argument-vector audit does not cover it.

## Generalized incoming-role audit, 2026-08-24

The portable Gruntz adapter parsed all 293 source translation units with zero
errors, joined 1,145 of 1,175 repeated-role definitions to normalized functions
(30 were unpaired or ambiguous), decoded all 1,145 pairs, and compared 1,358
ABI-compatible role groups. The final result was **zero whole-role
permutations and zero uniquely migrated semantic events**. The population
includes 866 functions already at 100.00%, so the negative result also has a
large reflexivity control. This closes the distinguishable incoming-role
population after `HandleUnitContact`; it does not cover the 30 unpaired helpers
or roles whose observed event fingerprints are identical.

The first generic run produced three apparent migrations, and all three were
detector defects rather than source defects:

- `AddSwitchActionEvent` linearized a cdecl `delete` cleanup immediately before the
  shared epilogue. Summing both `add esp,N` instructions overstated the base
  frame by four bytes and renamed `playerSlot` as `cellKey`. Frame inference now
  reconciles the epilogue release against the forward /GX frame candidate.
- `CGruntzMgr::Run` scheduled `xor ebx,ebx` between `operator new` and its
  `add esp,4`. Caller cleanup identity now survives intervening register-only
  instructions instead of requiring adjacency.
- `CMapMgr::Expand` used `lea reg,[dx+col]` on one side and
  `lea reg,[dy+row]` on the other while the complementary sum used `add`.
  `lea` is address arithmetic, not a memory read, and no longer enters the
  scaled-dereference event channel.

Permanent controls cover those three cases plus the corrected
`HandleUnitContact` negative, an injected entry-role permutation of its real
401-instruction base body as the positive, `ResolveTileClaim`'s delayed
callee-save push, `MoveClimbing`'s `push ecx` local, `SaveRle16`'s dead-home
reuse, ordinary EBP frames, and a synthetic three-role cycle. The generic tool
keeps indirect switch edges, ambiguous CFG merges, and arguments parked across
nested calls as explicit false-negative boundaries rather than guessing.

## Full-corpus reverse audit
A 2026-08-22 reverse audit covered all 641 primary functions whose historical
MAX remained below 100, with 10,911 equal-callee call sites paired between the
rebuilt and retail objects. Comparing symbolic pushed-value vectors found three
more source defects:
- two pursuit calls in `CBattlezMapConfig::Step` put `0xd87` in
  `arrivalPhase`; retail puts it in `maskA`;
- `CGrunt::ScanNearestTarget` put `m_arrivalFlags` in `arrivalPhase`; retail
  puts it in `maskA`;
- a source-level coordinate-pair pass found `CGrunt::UpdateArrival` passing
  one `Coord` as `(y, x)` where retail passes `(x, y)`.
The vector screen also produced useful negative controls. Cross-jumped branch
arms can leave two unrelated call setups in one linear pre-call window, and cl
5.0 freely rotates local homes or reuses dead parameter homes. A raw
`[esp+N]` operand is therefore not an argument identity. Normalize it by the
epilogue-derived fixed frame and the current push depth, then trace the value's
definition. `CTriggerMgr::ClearCell` is the compact control: rebuilt and retail
use different frame sizes, but after depth normalization both pass the rounded
world coordinates as `(x, y)`.

## Independent re-sieve, 2026-08-23: outgoing call-order swaps are drained

After the five fixes (FinishActiveAction, the two arrivalPhase/maskA pursuit
calls, GruntScanTarget, GruntArrivalUpdate's TileSwitch) a second, independently
written sieve was run over **all 4,426 paired functions** and found **0**
remaining swapped-order call sites. Script: `scratchpad/argswap.py` - it walks
each `call`'s argument run backwards, resolving every `push <reg>` through the
last `mov <reg>,<memref>` that defined it, then flags calls where both sides
push the same operand multiset in a different order.

**The one methodological trap, learned by hitting it.** The first run flagged
`CBattlezMapConfig::ValidateUnitPath` calling `PathToNearestGoal` with
`[esp+0x18]`/`[esp+0x1c]` reversed. That is a FALSE POSITIVE: the two builds
assign frame slots independently, so the same `[esp+N]` names a different
variable on each side and their order carries no information. Only
`this`-relative (`[esi+0xNN]`) and global operands are decidable across builds -
which is exactly why FinishActiveAction was provable (`[esi+0x17c]` /
`[esi+0x180]` mean the same member on both sides). The sieve now drops every
esp-relative operand before comparing; with that filter the tree is clean.

Re-run 2026-08-24 after correcting the incoming `HandleUnitContact` roles: the
independent assembly pass checked all 4,427 paired functions and again found
zero swapped outgoing call vectors. A separate clang AST pass parsed 293 source
translation units with zero errors and found zero definite Y,X arguments passed
to X,Y parameters. These are complementary negative controls; neither result
would have detected the callee-wide same-typed pointer inversion above.
