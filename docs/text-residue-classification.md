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

## Method to reproduce / re-measure

The classifier is inline in the campaign notes (per-obj objdiff JSON, count
INSERT/DELETE vs ARG_MISMATCH, flag callee-saved density for the register
class). Re-run after any wave to track the partition; the C2-anchored share is
the floor that source work cannot cross.
