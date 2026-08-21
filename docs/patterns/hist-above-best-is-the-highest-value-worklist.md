# `hist_pct > best_pct` is a recovered-exact worklist, not a statistic
tags: topic:method topic:tooling topic:wall | cpp:local cpp:pointer | asm:sub-esp
symptoms: a function that used to be exact; best RESET by a source edit; cleanliness pass cost a match; frame one dword short; invented NULL initializer; hist 100 best 90
confidence: 10/10

`config/match_baseline.tsv` carries THREE numbers per function and the third is
the one nobody reads. `best_pct` is scoped to the current implementation and
RESETS when the per-function source hash changes; `hist_pct` never resets. So

    hist_pct > best_pct

means: **an earlier source shape scored higher and a source edit took it away.**
That is not noise and not a wall - it is a bug report with a git bisect already
attached, because the ledger row records the `src_hash` that scored it.

## The screen

Join the lane's worklist against the baseline and print every row where
`hist > best + eps`, descending. Two extra reads make it decisive:

* `hist == 100` - the function was **EXACT**. Highest value in the campaign.
* `best == 100` with a low `cur` - the OPPOSITE. Already fixed; the dip is TU
  composition. Do NOT "improve" it: any source edit resets `best` off 100.

## Finding the culprit

The ledger is checked in, so the row's own history is the bisect:

    git log --oneline -G'<mangled name>\t100\.0000' -- config/match_baseline.tsv
    git show <commit> -- config/match_baseline.tsv | grep '<mangled name>'

A `-` row with the old number and a `+` row with the new one, and a CHANGED
`src_hash`, names the commit and the source edit. An UNCHANGED `src_hash` across
the drop means TU composition, not that commit's source - keep looking.

## What it finds, measured 2026-08-21 in one lane

Four functions, all from cleanliness passes that added source the retail
compiler never saw:

| function | was | commit's intent | cause | now |
|---|---|---|---|---|
| `StreamFeeder::FillBuffer` | 100 -> 86.66 | type audio buffers `void*`->`u8*` | `= NULL` seeds on Lock out-pointers | **100 EXACT** |
| `DirectSoundMgr::LockConvert` | 100 -> 90.09 | same | same | **100 EXACT** |
| `DirectSoundMgr::LoadFromFile` | 100 -> 97.73 | same | same | **100 EXACT** |
| `CDDrawWorkerMapSmall::LoadSizedPaletteFromSource` | 100 -> 91.28 | "model parse source length directly" | deleted a union pun's frame slot | **100 EXACT** |

The first three shared ONE cause: the typing pass seeded every
`IDirectSoundBuffer::Lock` out-pointer with `= NULL`, two stores retail does not
make. Nothing required the seed (`PtrOut` is a pure type pun and Lock writes
both slots unconditionally). **The typing itself was never the cost** - keep the
cleanliness, delete the invented initializer.

The fourth is the frame-slot reading: retail's frame was one dword LARGER
(`sub esp,0x54` against our `0x50`) because the era source reinterpreted a `u32`
length as the callee's signed size through a union local instead of converting
it. Restoring that pun LOCALLY (`AddrWord<char> h; h.m_uword = src->m_length;`
then read `h.m_word`) is exact. Re-adding the union MEMBER to the shared header
also reaches 100 but costs two other exact functions to the declaration ripple
(`CSBI_Image::Render` 100 -> 74.04, `CGruntzCmdMgr::BlitTileMarker` 100 -> 83.79)
- when a header and a local spelling both reach the same bytes, take the local.

## Discipline

* Not every gap is recoverable. `CSBI_ImageSet::SetupImage` (74.63 -> 68.31) and
  `CBattlezMapConfig::StepRowUnits` (88.22 -> 84.89) were *deliberately* traded
  for a correct model and their commits say so. Read the commit message before
  reversing anything.
* A row whose `src_hash` did NOT change across the drop is TU composition; its
  headroom is a bank-the-max opportunity, not a source bug.
* Never "improve" a row whose `best` is already 100 - the edit resets it. Two
  such rows were nudged +0.63 in this session and had to be reverted to restore
  the 100.00 bank (`CMotionState::ArrivalVelX`/`Y`).

## Related

[max-fuzzy-gate-eliminate-hacks](../../CLAUDE.md), [operand-level-adjudication](operand-level-adjudication.md)
