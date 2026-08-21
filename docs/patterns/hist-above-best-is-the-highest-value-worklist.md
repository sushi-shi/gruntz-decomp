# `hist_pct > best_pct` is a recovered-exact worklist, not a statistic
tags: topic:method topic:tooling topic:wall | cpp:local cpp:pointer | asm:sub-esp
symptoms: a function used to be exact; best reset after a source edit; a cleanup pass added stores; frame one dword short
confidence: 10/10

`config/match_baseline.tsv` carries three score readings per function.
`best_pct` is scoped to the current source fingerprint and resets when that
fingerprint changes; `hist_pct` is the highest score observed across all source
fingerprints. Therefore:

    hist_pct > best_pct

means an earlier source shape scored higher. This is a derived recovery queue,
not an invitation to keep a second hand-written worklist.

## Reading the row

- `hist == 100` proves that an earlier source fingerprint was exact.
- `best == 100` with a lower current score means the current fingerprint has
  already reached exact under another compiler state. Do not edit the function
  merely to improve its current score and reset that bank.
- A changed source hash at the score drop points to a source edit. An unchanged
  hash points to TU composition or compiler state instead.

The checked-in ledger makes the row's history searchable:

    git log --oneline -G'<mangled name>\t100\.0000' -- config/match_baseline.tsv
    git show <commit> -- config/match_baseline.tsv

Read the source commit as well as the number. Some historical peaks belong to a
model that retail evidence later disproved; correct structure outranks that
score.

## Measured recovery examples

Four exact functions were recovered in one 2026-08-21 pass:

| function | source debt | recovered shape |
|---|---|---|
| `StreamFeeder::FillBuffer` | invented `= NULL` stores on `Lock` out-pointers | leave the typed pointers uninitialized; `Lock` writes them |
| `DirectSoundMgr::LockConvert` | same | same |
| `DirectSoundMgr::LoadFromFile` | same | same |
| `CDDrawWorkerMapSmall::LoadSizedPaletteFromSource` | signed callee size passed directly from a `u32` member, losing retail's frame slot | bind `i32 length = static_cast<i32>(src->m_length)` and pass `length` |

The first three prove that the typed `u8*` model was not the cost; only the
invented stores were. The palette function is the important negative control
against a codegen-shaped union: an `AddrWord` local also reproduced the bytes,
but a normal signed local produces the same exact function and models the
source-level signed boundary directly. Prefer the signed local.

## Discipline

- Work this queue in descending historical loss, but keep the normal retail
  operand, referent, caller, and ownership audit.
- Never restore an old shape solely because its score was higher.
- Never edit a current-hash `best == 100` row to chase its transient score.
- Bank an unchanged fingerprint while exact; source edits start a new proof.

## Related

[MAX fuzzy divergence](../max-fuzzy-divergence.md),
[operand-level adjudication](operand-level-adjudication.md)
