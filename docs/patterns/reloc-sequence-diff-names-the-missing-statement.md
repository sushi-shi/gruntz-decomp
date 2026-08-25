# The ordered RELOC SEQUENCE names the missing STATEMENT in a -200-instruction function

- **confidence** c10
- **tags** `topic:method` `topic:triage` | `asm:call` | `topic:codegen-idiom`
- **measured** `CGrunt::TryTeleportToCell` @0x52fb0 **42.21 -> 92.99** (deficit -292 -> +3);
  `CGrunt::AdvanceMotion` @0x5f310 **64.14 -> 79.03** (-192 -> -71);
  `CGrunt::StepSmartChaserBehavior` @0xf42f0 **51.30 -> 65.71** (-212 -> -54).

## The method

`gruntz walls diagnose <rva>` classifies the wall and, for the REFERENT class,
prints the ordered relocation pairs that differ. For the general case build the
sequence yourself: `gruntz sema dump <rva>` lists retail's relocation targets in
program order, and the normalized base object carries the same list for our side.

Keep only the instructions that CARRY a relocation, as `(mnemonic, symbol)` pairs
in program order, and run difflib over the two sides. That list is the function's
**calls, global reads and string references** — its statements — and unlike the
instruction stream it survives register allocation and block scheduling (the
optimizer reorders *within* a block, not across a call).

The instruction count says a bug exists; the mnemonic histogram says which
*idiom* is wrong; this says which *statement* is missing, extra or in the wrong place.

## Reading it

- **A long unbroken `+` run** = a block retail has and you do not. This is the
  `-200` case and it is a reconstruction, not a tune. `TryTeleportToCell`'s run
  was 40 relocs long: two more act-key gates, the wormhole spawn, and the two
  ignored parameters.
- **A `-`/`+` pair on one row** = a wrong callee or a wrong global. Cheap and
  always a real bug (`GetNameRecord` vs `ScratchResolve`; `CoordHead` vs
  `CoordTail`; `??1CString` vs `??0CString`).
- **A group moved as a unit** = block PLACEMENT, i.e. a source-shape idiom
  ([negated-condition-far-block](negated-condition-far-block.md),
  [if-body-owns-the-fallthrough](if-body-owns-the-fallthrough.md)). Retail
  emitting the big `else` before the small `if` body means the source negates the
  test.
- **Per-symbol COUNTS** (`Counter` over the sequence) find a whole missing arm
  faster than the diff does — one cluster each short by exactly one is a deleted
  `case` ([always-returning-gate-dce-kills-a-later-switch-arm](always-returning-gate-dce-kills-a-later-switch-arm.md)).

## Two false alarms — do NOT chase them

1. **Symbol NAMES differ between the two objs for the same address.** The base obj
   names a pooled 1-char literal `??_C@_01PFH@A?$AA@`; the delinked target may name
   the same byte `_s_codeA$S41355` or, worse, an unrelated neighbour
   (`??_C@_0BE@MAOF@GAME_ACTIONAREA_RED?$AA@` for the "A" at 0x60a454). Resolve the
   ADDRESS before believing a string mismatch: `gruntz sema disasm <rva>` prints the
   real reloc table, and reading the retail bytes at that VA settles it.
2. **`$T…` vs a named double** on the FP side is the anonymous-constant pool, not a
   missing global.

related:
[instruction-count-mismatch-finds-the-real-bug.md](instruction-count-mismatch-finds-the-real-bug.md),
[mnemonic-histogram-diff-finds-the-wrong-idiom.md](mnemonic-histogram-diff-finds-the-wrong-idiom.md),
[reloc-sequence-diff-finds-wrong-referents.md](reloc-sequence-diff-finds-wrong-referents.md)

## USE THE SHARED TOOL — do not rebuild the technique

`gruntz walls diagnose <rva>` is the maintained entry point. A lane that
re-derived the same idea by hand shipped three false-positive bugs and acted on one,
regressing `CGameLevel::ResolveFloorCollision` 94.65 -> 34.58 before catching it:

1. same-unit direct calls discarded as "intra-function jumps" by an address-window
   heuristic (this is what faked the missing `AxisProbe`) — gate on the function's real
   size, never a window;
2. an llvm-objdump byte-column regex that required a trailing space, so 10-byte
   instructions (`mov dword ptr [0x0],0x1`) were dropped — that faked a
   `g_logicTypesRegistered` 1-vs-2 delta on `CObjectDropper`;
3. only the FIRST relocation per instruction collected.

The shared tool has none of these (verified against both functions above: `AxisProbe`
reports 2-vs-2, `CObjectDropper` reports no `g_logicTypesRegistered` delta). If you think
you need your own copy, fix these three first — or better, extend `insn_seq.py`.
