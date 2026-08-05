# Diff the ORDERED reloc-symbol sequence - the masked diff cannot show a wrong callee

**Tags:** data:objdiff cpp:call cpp:switch | asm:call asm:push | topic:tooling topic:mis-model

## Symptom

A function sits well below 100% with no obvious instruction-shape difference, or the
shapes differ everywhere and the diff is too noisy to read. `--diff` and
`--blocks --diff` mask address operands, and `--branches --diff` only covers branch
targets - so a **wrong callee**, a **wrong global**, a **rotated set of string arms**,
a **missing call** or a **wrong argument count** is invisible in all three views.

## Mechanism

Every `call rel32`, every `push <string>` and every global access carries a COFF
relocation naming its referent. `llvm-objdump -dr` prints those names on both sides:
the base obj gets cl's own symbols, and the delinked target obj gets whatever symbol
the delinker resolved retail's address to. Comparing the two **ordered sequences**
(not by offset - the code shifts as soon as anything diverges) is a position-
independent statement of "does this function reference exactly what retail's does, in
the same order".

Extract, per function, the reloc symbol at each instruction; canonicalise the two
structural classes that always differ, then `difflib` the lists:

- a reloc to a `$L…` label or to the function's own symbol is the **switch/byte
  table** - fold both to `<TABLE>`
- `__except_list`, `___CxxFrameHandler` and `Unwind@…` are the **EH frame** - the
  target carve does not include the unwind funclets, so the base always has extras

## The trap: pooled string literals

Retail is `/Gf` and the linker folded identical literals, so a target reloc is often
named by the **nearest owned symbol plus an addend** - `_s_IMPACTMM4$S34310+0x24`
is `s_IMPACTMM3`, and `??_C@_07HCON@ToyPeek?$AA@-20` is whatever static sits 20 bytes
earlier. Resolve the addend through `build/gen/symbol_names.csv` before believing a
string mismatch; a bare name difference between a `??_C@…` on one side and a `_s_…`
on the other is nearly always this artifact, not a bug.

## Measured (2026-08-05, matcher lane B3)

Every one of these was invisible to the masked diff and fell out of the sequence diff
in minutes:

| function | found | before -> after |
|---|---|---|
| `CTriggerMgr::LoadPowerupIconSprites` | three powerup arms rotated by one | 98.93 -> **100 EXACT** |
| `CGrunt::StepCompassMove` | `GetIntDef` should be `GetDwordDef`; the empty-bag `return` belongs inside the `CByteArray` scope (two dtor calls); the random-slot pick keeps its degenerate `count == 0` arm (two `rand()` calls) | 37.48 -> 53.25 |
| `CTriggerMgr::ReinitGroup` | `GetInt` not `GetIntDef` and its two arguments swapped; `RefreshState` not `Reset`; `SetAtGrow(size, colour)` not `InsertAt(size, 0, 0)`; `Format` had one variadic argument too many; `EnsureSub(outR, outC, colour)` | 75.50 -> 83.06 |
| `CTriggerMgr::CombatCue` | a missing `CGrunt::StepArrivalCommit()` call; `GetDwordDef` | 79.58 -> 85.54 |
| `CTriggerMgr::PlaceObjectFull` | a whole missing case arm (GOOBER walks `m_baseList`) plus the spy's hidden-object lookup | 31.94 -> 45.30 |
| `CGruntzMgr::TransitionState` | `CMulti`'s ctor is inline in retail | 82.60 -> 86.44 |
| `CRollingBall::Update` | `VtblResolve` is inlined; and the floor/ceil arms were **inverted** (a live behaviour bug) | 83.14 -> 83.96 |
| `CTriggerMgr::WireTileSwitchLogic` | the arrow-current inner switch has no `default` arm | 88.37 -> 90.00 |
| `CTriggerMgr::NotifyCell` | `CWarlord::RaiseBattleAlert`, not `ResolveDeathAnimation` | referent set now identical |
| `CTriggerMgr::DestroyAllAnims` | the notify slot is compared against `CreateProjectile`, not a `CGrunt` member pointer | referent set now identical |

Two of these (the RollingBall rounding, the ReinitGroup bound compares) were live
behaviour bugs, not just byte differences.

## When the counts match

If the two sequences are equal apart from `<TABLE>`/`<EH>`, the referent set is
correct and the residue is pure codegen - stop reading relocations and go back to
`--rich` / the register allocation.

## A second, related read: the jump table names the arm order

For a `switch`, the target obj's jump-table entries are DIR32 relocs to the function
itself, and the **addend of each entry is the arm's offset**. Reading the byte table
(slot per case) and the jump table (offset per slot) gives retail's arm addresses,
and since cl emits arms in SOURCE order, that is retail's source order directly. It
also exposes arms that do not exist in the reconstruction at all -
`PlaceObjectFull`'s GOOBER case was found this way.

## Second trap: the delinked obj's string names disagree with the raw disasm

The reloc NAMES on the target side come from whatever `symbol_names.csv` resolved
retail's address to, and for one-character literals that resolution is often a
different literal at a nearby address. A sequence diff that reports retail
comparing against `"J"` and `"R"` where you compare against `"L"` and `"P"` is
usually this artifact, not a wrong letter. **Confirm every string finding against
`gruntz sema disasm <rva> --target`**, whose reloc table prints the raw retail
address - then map addresses to letters once (they are consecutive in `.rdata`)
and reuse that map. `CTriggerMgr::ApplyTriggerB` looked like three wrong letters
and was in fact three missing re-resolutions of the same name.

## Confidence

c9 - reproduced across ten functions in one session; the only false positives are the
pooled-string naming and the EH-funclet extras, both of which canonicalise away.

related: masked-diff-hides-branch-target.md, rva-extent-must-include-switch-tables.md,
pooled-string-literals-one-owner
