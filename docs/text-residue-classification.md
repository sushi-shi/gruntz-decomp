# .text residue classification — how close source reconstruction can get to 100%

Measured 2026-08-13 by classifying every sub-100% function's objdiff instruction
rows (`objdiff-cli --format json`, DIFF_INSERT/DELETE = block-shape,
DIFF_ARG_MISMATCH = register/operand) and weighting by `size*(100-pct)/100`.
The class boundaries are the ones PROVEN this session (see the relevations).

## The partition (67,216 B weighted residue, 823 functions)

| class | weighted B | share | fns | source-reachable? |
|---|---|---|---|---|
| structural-CFG | 12,676 | 18.9% | 120 | YES — control-flow reconstruction lands gains |
| referent/other | 38,561 | 57.4% | 322 | PARTLY — caller-saved churn + referent/type fixes |
| register-canonicalized | 15,693 | 23.3% | 275 | NO — proven C2-anchored (3 controls) |
| tiny-residue (≤3 rows) | 286 | 0.4% | 106 | mostly NO — 1-2 byte operand/reloc canonicalization |

## What this means for the 100% goal

- **~23.8% of the weighted residue is C2-anchored** (register-canonicalized +
  most tiny-residue): the operand load-order canonicalization proven invariant
  to source across FindGruntAt / SumWeighted / ProbeHeadSoft. No source spelling
  closes these; 100% on them is gated on C2 codegen RE or accepting the bytes.
  This is a HARD CEILING on pure source reconstruction, first quantified here.
- **structural-CFG (18.9%)** is the reliable source-reachable work: the
  control-flow reconstructions, minus the switch-context cross-jump coin
  (PlaceObjectFull / BuildTabzDialog class — bounded, not source-steerable).
- **referent/other (57.4%)** is the largest and least-explored bucket: a mix of
  wrong referents/types (source-reachable, the classifier campaigns) and
  caller-saved (EAX/ECX/EDX) operand scheduling (partly C2-side). This is where
  the next investigation wave should focus — it dominates the weight.

## Sharpening (2026-08-13, later): REGISTER/SCHEDULE is C2-anchored at ANY %

CGruntPuddle::CGruntPuddle (0x40490, 55.87%) is `diagnose`-classified
REGISTER/SCHEDULE (call set + branches agree). Its entire 44% residue is
constant-pinning: retail pins 0 in EBX and 1 in EBP and reuses them across the
ctor's many `m_x = 0/1` member stores; we materialize each constant inline. The
constant COUNT is fixed by the ctor's semantics (m_pending=1, m_placed=0, flag
bits) - no legitimate source change alters it (forcing a pinning local is the
banned zero-register hack). Permute moved it 55.80 -> 55.87 (noise).

IMPLICATION: a `diagnose` REGISTER/SCHEDULE verdict = C2-anchored REGARDLESS of
percentage. Low-% register functions are not "more reconstructable" - they just
have denser C2 register/constant scheduling. So the ACTUAL source-reachable set
is ONLY the structural-CFG (branch-count-mismatch) functions; much of the
crude "referent/other" bucket is REGISTER/SCHEDULE the byte-classifier
mislabeled. The source-reachable share of the residue is therefore SMALLER than
the 18.9%+57.4% upper bound - closer to the structural-CFG 18.9% plus the
genuine wrong-type/wrong-referent subset of referent/other. Screen every
candidate with `diagnose` FIRST; only pursue structural-CFG verdicts by hand.

## Diagnose-screen of the heavy worklist (top-45 weighted sub-95%)

Screening the 45 heaviest sub-95% functions with `gruntz walls diagnose`:

| verdict | count | reachable? |
|---|---|---|
| structural (branch-count diff) | 14 | YES |
| structural (block-gap >=4) | 8 | YES |
| masked-truncation (blocks near-agree) | 12 | mostly C2 |
| register / C2-anchored | 11 | NO |

So ~49% (22/45) of the heavy worklist is source-reachable structural work - a
substantial, concrete backlog. The proven-productive loop: `diagnose`-screen,
discard REGISTER/register verdicts, hand-reconstruct the structural ones.
FindOrInsert (child-select) was closed this way (+0.32). Fresh structural
targets identified: LoadAttributes (switch-jump-structure, likely C2), Run@
CGruntzMgr (253v258 blocks), StepDefenderUnit / ExecuteCommand / ValidateLevelTiles
(dossiered). NOTE: switch-heavy overbuilds (LoadAttributes' PickA/B/C dispatch)
are usually C2 jump-table-structure choices, not source-reachable - verify the
divergence is NOT inside a switch before committing to a sitting.

## Method to reproduce / re-measure

The classifier is inline in the campaign notes (per-obj objdiff JSON, count
INSERT/DELETE vs ARG_MISMATCH, flag callee-saved density for the register
class). Re-run after any wave to track the partition; the C2-anchored share is
the floor that source work cannot cross.
