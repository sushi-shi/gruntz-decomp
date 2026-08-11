# A `switch`'s arm BODIES come out in SOURCE order — read the retail index table to recover it

tags: cpp:switch | asm:jmp asm:cmp | topic:codegen-idiom topic:tooling
symptoms: a switch-heavy function is stuck in the high 90s; `sema disasm --diff` shows a
  huge one-sided hunk starting at `jmp dword ptr [reg*4]` because the BASE side is truncated
  there; once the whole COMDAT is read, two arm bodies appear in the opposite order and every
  other instruction agrees; or the code agrees completely and only the byte index table differs
confidence: 10/10

cl5 lays out a switch's arm bodies in the order the `case` labels appear in the SOURCE,
not in case-value order. The dispatch is value-ordered (jump table indexed by
`value - min`, optionally through a byte index table), so a source-order permutation
is invisible in the dispatch and shows up only as two bodies swapped in `.text`.

## Why the diff hides it

`sema disasm --diff` (and `--blocks --diff`) stop the BASE side at an in-function
`jmp dword ptr [reg*4]`, so everything after the dispatch reads as a target-only
addition. `llvm-objdump -d` on the base obj is not enough either: it restarts its
listing at every `$L…` label, so a naive "grab the lines under `<sym>`" also stops at
the first table. Read the whole SECTION and stop at the next non-`$L` symbol.

## Recovering retail's arm order (and the exact case values)

The jump table and the byte index table are inside the function's own `.text`. Decode them:

1. find the `ff 24 8d/95/…` (`jmp [reg*4+disp32]`) and take `disp32` — the jump table;
2. find the `8a 80-bf disp32` (`mov r8,[reg+disp32]`) — the byte index table, if any;
3. jump-table entries, sorted by ADDRESS, are the arm order retail's source had;
4. index-table slot `k` holds the arm number for `value = k + min`, which is the case
   value map — no guessing.

On `CMulti::HandleControlMsg` @0xba1a0 this gave arms at 0xba1c5 / 0xba1ec / 0xba1fb /
0xba20c / 0xba21d for codes 5 / 3 / 0x101 / 0x31 / default. Two independent defects fell
out: `DESTROYPLAYERORGROUP` had to be written before `CREATEPLAYERORGROUP` (96.66 → 99.97),
and the last 0.03 was the index table — the source had `DPSYS_SESSIONLOST` and `DPSYS_HOST`
transposed on their two arms, which the byte table settles outright (99.97 → 100 EXACT).

## Corollary: `if (a == X || a == Y) { body }` is TWO arms in retail

The `||` form makes cl emit one body with a shared `cmp X; je <shared>`; retail's
`cmp X; jne` followed by a FULL second copy of the body means the source wrote the two
tests as separate `else if` arms with the body duplicated. `CPlay::LoadWarlordSprites`
@0xd65d0: splitting `d == PICKUP_TOYBOX || d == PICKUP_MEGAPHONE` into two arms (two sites)
took it 95.26 → 99.41.

## Corollary: an empty case run may need an explicit shared exit

Several adjacent case labels followed by `break;` do not necessarily remain one
IR arm when that break and the default path reach the same machine-code exit. cl5
can keep only the first case on the explicit arm and fold the remaining empty
labels into the default arm. The executable instructions and every jump-table
target then agree, but the byte index table exposes the lost source identity.

`CTileTriggerLogic::LoadBridgeMove` @0x110860 was otherwise exact. Its candidate
index table began `00 07 07 07`, assigning cases 15–18 as one explicit arm plus
three default-arm entries; retail began `00 00 00 00`. Replacing the grouped
`break;` with `goto done;` and spelling the shared final `return;` made all four
labels retain slot 0 and took **99.9887 → 100 EXACT**. This is not a semantic
change and it is not a register wall: the three one-byte index entries were the
entire objdiff residue.

## The table also settles WHICH BODY belongs to which case (a live-bug finder)

Reading it only for the ORDER leaves half its value on the table. The entry at slot
`k` names the body for `key == k + min`, so a reconstruction that attached the right
set of bodies to the wrong labels reads out immediately.

`CStatusBarMgr::ClearTabGroup` @0x100b00 (table 0x100c3c, `dec eax` so slot `k` is
`m_activeTab == k+1`) had all five arms permuted: our source cleared `m_tabSprite5..10`
on STATZ and `m_statObj` on GRUNTZ where retail clears `m_statObj` on STATZ,
`m_slotNotify`+gauges on GRUNTZ, `m_groupNotify`/`m_hlNotify` on RESOURCE,
`m_warlordHead` on MULTIPLAYER and `m_tabSprite5..10`+`m_modeNotify` on GAME — each
tab its own widgets. That is a behaviour bug the score barely moved on (75.85 →
75.94), so ONLY the table catches it.

Order alone, on the same TU: `CStatusBarMgr::UpdateStatusBarTabHighlight` @0xfe910,
seven arms, table 0x4ff4a0 sorting to tab 0,5,1,4,2,3,6 against our ascending
0..6 — **55.34 → 85.03 from the reorder alone**.

Trap that hides this whole family: `sema disasm --base` and `--diff` stop at the
indirect `jmp`, because llvm-objdump's `--disassemble-symbols` restarts at the `$L`
label the table lives under. A one-sided `@@ -1,15 +1,497 @@` hunk is that
truncation, not a 480-instruction divergence — dump the whole section and cut it at
the next non-`$L` symbol before believing any read of the arms.

related: rva-extent-must-include-switch-tables.md, switch-density-byte-index-table-vs-tree.md,
masked-diff-hides-branch-target.md
