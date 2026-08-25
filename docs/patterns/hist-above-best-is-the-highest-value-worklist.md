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
| `SoundBuffer::LockConvert` | 100 -> 90.09 | same | same | **100 EXACT** |
| `SoundBuffer::LoadFromFile` | 100 -> 97.73 | same | same | **100 EXACT** |
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
(`CSBI_Image::Render` 100 -> 74.04,
`CGruntzCmdMgr::EnqueuePlaceGruntAtScreenPoint` 100 -> 83.79)
- when a header and a local spelling both reach the same bytes, take the local.
The later negative control goes one step further: a plain signed local
(`i32 length = static_cast<i32>(src->m_length);` passed as the callee size)
produces the SAME exact function, so the union pun is not necessary source
structure at all - it was one of several spellings that reproduce the frame
slot. Prefer the signed local; it models the signed boundary directly.

## The second sweep, 2026-08-21: read the DROP COMMIT, then classify

Tracing all 49 rows against every revision of the ledger (one pass over
`git show <sha>:config/match_baseline.tsv`, recording each row's `best` and
`src_hash` per revision) splits them cleanly, and the split is what saves the
budget:

| class | tell | verdict |
|---|---|---|
| **an invented store** | the drop commit is a *typing/cleanliness* pass and its diff adds an initializer or deletes a copy | RECOVERABLE - the typing was never the cost |
| **a deliberate trade** | the drop commit's message OR the function's own source comment states the structural gain | leave it; re-deriving it wastes a build |
| **TU composition** | `src_hash` did NOT change across the drop | not a source bug |

Recovered in that sweep, both from the same commit ("type gameplay sound cue
lookups"): `_WinMain@16` 99.59 -> **100.00 EXACT** (a `= NULL` seed on
`VerQueryValueA`'s out-pointer), and `CBootyState::LeaveState` +
`CMultiBootyState::LeaveState` 77.27 -> **79.30**, each back to its peak - the
pass made `found` the lookup's own sink and deleted the era source's pointer
copy, so the escaped sink is reloaded at every use
([inlined-member-this-copy-survives-the-escaped-out-param](inlined-member-this-copy-survives-the-escaped-out-param.md)).

Classified as deliberate and left alone: `CRezImage::FlipVertical`,
`CSBI_ImageSet::SetupImage`, `CWwdGameObjectA::BltDirtyEx`,
`UpdateChipGrinderStatusBar`, `CMinimap::Draw`,
`ConvertRowDoubleFwd`, `CTileSecretTriggerLogic::Tick`,
`FontRenderer::DrawGlyphRun` (its commit message even prints the 71.74 -> 68.10
it accepted and why).

**CORRECTION to the first sweep's discipline note.** It listed
`CBattlezMapConfig::StepRowUnits` (88.22 -> 84.89) as a deliberate trade. It
was not: the commit traded the aggregate model in with the WRONG SPELLING of it
(see the Coord-aggregate pattern's probe table), and the target obj shows five
real `or eax,-1 / or ecx,-1` pairs in that body. Re-folding with the
whole-object-copy spelling took it to 85.12. "Deliberate" has to mean the
commit or comment states the STRUCTURAL gain, not merely that someone accepted
the number.

## Discipline

* A row whose `src_hash` did NOT change across the drop is TU composition; its
  headroom is a bank-the-max opportunity, not a source bug.
* Never "improve" a row whose `best` is already 100 - the edit resets it. Two
  such rows were nudged +0.63 in this session and had to be reverted to restore
  the 100.00 bank (`CMotionState::ArrivalVelX`/`Y`).
* Not every `hist > best` row is a SOURCE deviation at all.
  `CButeMgr::GetFloat` (99.77, hist 100) diagnoses as REFERENT with byte-
  identical code: our `butemgr` `.rdata` contribution is 0x18 bytes against the
  0x588-byte run the delinker carves for retail, so the content-hashed
  `$Sdata_rdata_<sha>` blob names differ. That is delinker granularity, not a
  body to fix. Diagnose before you bisect.

## Related

[max-fuzzy-gate-eliminate-hacks](../../CLAUDE.md), [operand-level-adjudication](operand-level-adjudication.md)
