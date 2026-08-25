# A control that cannot express the failure is not a calibration

**The revelation:** every paired sieve we run was calibrated the same way — *"it returns zero
over the exact rows"* — and for most of what those sieves measure that sentence is **vacuously
true**. An exact row is byte-identical to retail, so every quantity derived FROM THE BYTES is
equal on both sides *by construction*. A frame offset, a slot number, a member displacement, a
loop span, an instruction count, a condition code: the comparison is `f(x) == f(x)`. It cannot
fail, whatever the code does, and a green cell there is not evidence.

Ask of each sieve instead: **is there a failure mode an exact row structurally cannot exhibit?**
Four independent defects fell out of asking it once, and the sieve that prompted the question
had its own census collapse from 12/13 rows to 2/0 when they were fixed.

---

## Worked example: the sieve whose calibration was structurally blind

`walls valuetemp` finds an inlined accessor's by-value struct temp by keying a dead store pair
on its **frame slot**. It was calibrated by running it over the 100.00% rows and reporting zero.

That calibration cannot fail. On a 100.00% row the two sides have the SAME FRAME, so a
frame-offset comparison agrees whatever the tracking does. Under it, four separate defects
survived: esp drift across calls, a frame level short by the saved registers, deadness
attributed to a SLOT rather than to a STORE, and — the one the calibration was structurally
incapable of reaching — **comparing a frame offset at all**. `CBattlezMapConfig::RouteToNearbyEnemy`
emits the identical eleven-instruction RECT copy on both sides, at `[esp+0x6c]` on ours and
`[esp+0x80]` on retail's, and keying on that offset reported it as an asymmetry in BOTH
directions at once.

The fix was not a better comparison, it was a different KEY: a member offset is comparable and a
frame offset is not, so member pairs are compared and local aggregate copies are COUNTED and
never compared. (Their count IS comparable, and it is reported as its own line.)

---

## The audit, applied to the rest (2026-08-23)

Every quantity a sieve keys on falls into one of three classes, and the class decides what a
control can say:

| key | comparable across two builds? | reachable by an exact-row control? |
| :-- | :-- | :-- |
| mnemonic / condition code / call referent / member offset / count | yes | **no** — equal by construction |
| frame offset, slot number, register name, instruction index, absolute address | **no** | **no** — equal by construction |
| relocation PRESENCE and SPELLING | yes, but the two objects are different artefacts | **yes** — this is the one live cell |

That last row is why reflexivity is not entirely vacuous. The base object is cl's own output and
the target is the delinker's; their relocation tables differ in presence and spelling even where
objdiff scores 100.00. Every paired sieve filters on `Line.ref`, so an asymmetric referent reads
as a residual on a function that is already byte-perfect.

Running it found one: `zPTree::Walk` (0x193340) and `CDDrawSubMgrLeaf::LoadFromTree` (0x152ad0) are
both recursive and both byte-identical to retail, and both read as a one-instruction `selection`
residual — because our object spells the recursive call as a `call rel32` with a relocation
naming the function, and the delinked target resolves it inside its own section with no
relocation at all. `walls residue` dropped the line on one side and kept it on the other.
`walls thisscan` had already met the same asymmetry from the other end; its own calibration
caught ten inverse "call sites" that were the caller calling ITSELF.

### What the audit found, per sieve

| sieve | key | verdict |
| :-- | :-- | :-- |
| `valuetemp` | member offset (was: frame offset) | **fixed earlier.** Its `--calibrate` is the vacuous half; `--control` carries the weight |
| `storescan` | member-store run offsets | **DEFECT.** Excluding the literal `esp` base is not enough — cl builds a local aggregate's address in an ordinary register (`lea esi,[esp+0x20]`) and fills through THAT. 66 of 595 todo rows carry such a run, 21 disagree on how many, and the positional run pairing shifted with them |
| `jccscan` | condition-code multiset | **DEFECT.** The region rule cut at the last `ret`, which discarded 5524/6147 real instructions (531/599 condition codes) in cl's cold blocks, asymmetrically on 41 rows |
| `framescan` | `sub esp,N` delta | **DEFECT (evidence column).** A fixed 24-instruction prologue window counted ARGUMENT pushes as callee-saves on 108/107 rows. The delta itself does not move on a single row; the push count beside it changes on 134 |
| `residue` | masked stream, member displacements | **DEFECT x2.** The recursive-call referent above, and `[ebp+-N]` treated as a member displacement in the 8 rows cl gives an ebp frame |
| `loopscan` | loop count, body span | **clean.** Jump-table payload produces 0 phantom loops (measured, not assumed). Its positional anchoring inside a replace block stays untested by construction |
| `retscan` | retail `ret N` vs our mangled arity | **sufficient, and now bounded on the other side.** One-sided, so there is no comparability question; the exact-row control bounds false POSITIVES only, and `--recall` was added to bound false negatives (2 of 3962 ours, both benign decode artifacts) |
| `thisscan` | callee name, dead ECX per side | **sufficient.** Its structural limit — a dropped receiver in an EXACT caller — is stated by the tool, and `--retail` is the one-sided screen that reaches it |
| `ehactions` | (slot, dtor) sequence | **sufficient once given a control.** Frame displacements were already quarantined as `slot-shift`. Funclet discovery is NAME-driven through two different paths, which IS reachable: `--calibrate` runs the census over the 710 parents whose whole band is exact; all read `equal` |

## The counter-example: when the exact rows ARE the control

`walls retscan` is one-sided. It reads retail's own `ret N` from the image and our stack-argument
bytes from the mangled name, and the byte side is read the same way whatever the row scores. Its
exact-row cell therefore exercises the same code path as every other row and it earned its keep —
it caught all four defects of its first run, two of them parameter-list parses. The limit is
narrower and different: an exact row cannot exhibit a MISSED disagreement, so the cell bounds
false positives and says nothing about the deliberate recall trade in its membership rule. That
needed its own measurement, not a better calibration.

## Detection signature

Read a sieve's comparison and ask what would have to differ for the control to fail. If the
answer is "nothing, because the inputs are identical", the control is measuring the absence of a
crash. Then look for the key that is not comparable — a frame offset, a slot index, a position, a
register name — and either change the key or stop comparing it and count it instead.

`gruntz walls calibrate` runs the reflexivity cell for the five sieves that had no control at
all, and prints this caveat under its own result so the number is never quoted without it.
