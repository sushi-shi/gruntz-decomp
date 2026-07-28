# Replay verdicts — FROZEN RECORD (the tool that produced these is gone)

**The `recomp/replay` harness was removed on 2026-07-28** by user ruling: it launched real
`GRUNTZ.EXE` windows repeatedly, which is disruptive, and the campaign's return per unit of
effort is far better in ordinary byte-matching. Do not rebuild it. The `harness/` half —
which fabricates its inputs and never launches anything — stays.

These verdicts are kept because they are **evidence, not tooling**. Each row cost a real
capture and none of it is reproducible now, so treat the file as read-only history. Two
things it is still good for:

- **An AGREE row retires a function.** Five of them are `@early-stop`-parked bodies whose
  correctness was previously unknown; `CMapMgr::ComputeCellFlags` sits at 0.00% and its
  parked note *claimed* the zero was a delinker artefact — that is now confirmed on 19
  inputs rather than asserted. Do not spend a lane re-litigating these.
- **The DISAGREE row is a live lead, and it is chaseable statically** — see the decoded
  `buf[]` table at the end of this file. It is a *specification*: retail's exact colour for
  each index range, checkable against our store program and against the disasm with nothing
  running.

  A first reading of that row blamed `g_clut` per-channel sub-table addressing (the sibling
  `CDDrawShadeBlit` uses three planes at `+0x20002`/`+0x2`/`+0x10002`). **That was wrong.**
  Decoding the failure to `buf[]` indices exonerates the packer: `Pack` is a variable-shift
  packer reading `g_rUp`/`g_rDown`/`g_gUp`/`g_gDown`/`g_bDown`, both sides read identical
  shifts, and `buf[1..8]` (`c00 = Pack(0x4e,0x78,0x1c) = 4bc3`) agrees — which exercises it
  end to end. The defect is in WHICH CONSTANT is painted over WHICH RANGE, i.e. the
  run-length / case-selection structure of the store program.

One caveat that limits the Shape rows specifically: they were measured against
`lightfxrender.obj` at commit `2bebf202f`, which **predates** the palette rebuild merged at
`605e35287`. Whether that fix moved them was never re-measured and now cannot be.

---

One row per function that was run through the removed replay harness. The column that
matters is **VERDICT**, not the match %: a function at 54% that is behaviourally identical
to retail and one that computes the wrong answer look the same on the scoreboard, and this
was the only thing that told them apart.

Read a row as: *our compiled bytes, run on the state the real game produced, wrote the
same bytes and returned the same value as retail's own machine code did, over the whole
restored address space.* The rules the comparison declines to apply — the callee's frame,
the incoming argument area, other modules' data, the observer's own footprint, and `eax`
for a void return — were stated and bounded in the removed `replay/README.md`.

Every AGREE row was taken with `--control`, which re-runs the same snapshot with a
one-sided perturbation and **fails the row** if the comparison does not go red.

## Ledger

`effect` is how many bytes the recorded call changed, i.e. how much there was to agree
about. `fuzz` is how many additional mutated inputs were run through
`[3] OURS vs RETAIL` with no game launch.

| function | fuzzy % | effect | fuzz | verdict |
|---|---|---|---|---|
| `CMapMgr::ComputeCellFlags` | 0.00 | 1 B / 1 region | +18 inputs | **AGREES** |
| `CMapMgr::UpdateDiagonals` | 54.43 | 1 B / 1 region | — | **AGREES** |
| `CChatBoxOwner::HitTest` | 63.06 | 0 B (pure predicate) | +18 inputs | **AGREES** |
| `CPlay::StepGridWalk` | 66.64 | 0 B, 1–2 B under mutation | +10 inputs | **AGREES** |
| `CDDrawShadeBlit::Select` | 68.75 | 1 B / 1 region | — | **AGREES** |
| `CLightFxRender::Shape3` | 72.11 | **524 B / 1 region** | — | **DISAGREES — 257 B wrong** |
| `CLightFxRender::Shape1,2,4..8` | 67–77 | 524 B | — | **DISAGREE** (cross-run, below) |
| `StateMgrBZ::Flush` | 74.39 | 0 B (return value only) | — | **AGREES** |
| `CMapMgr::Expand` | 78.83 | 19 B / 3 regions | +12 inputs | **AGREES** |
| `_ParseWaveChunks` | 100.00 | 9 B / 1 region | +3 inputs | **AGREES** |

Seven AGREE, one family DISAGREE. The AGREE rows are worth having — five of them are
`@early-stop`-parked bodies whose correctness was previously unknown — but the DISAGREE
is the point of the exercise.

## `CLightFxRender::Shape3` — 72.11% and computing the wrong pixels

The recorded call writes **524 bytes** into a 16-bit surface at `0x03ab9176..0x03ab93fb`
(20 contiguous spans). Our build gets **257 of them wrong**, and the harness self-test
`[1]` is IDENTICAL on the same state, so the difference is ours and not the harness's.

Decode the 16-bit pixels and the failure is not random — in almost every wrong pixel one
or more RGB565 **channels comes out zero**:

| retail | ours | R | G | B |
|---|---|---|---|---|
| `b307` | `0307` | 22 → **0** | 24 = 24 | 7 = 7 |
| `fc85` | `0485` | 31 → **0** | 36 = 36 | 5 = 5 |
| `f924` | `0124` | 31 → **0** | 9 = 9 | 4 = 4 |
| `47e8` | `07e8` | 8 → **0** | 63 = 63 | 8 = 8 |
| `3186` | `0186` | 6 → **0** | 12 = 12 | 6 = 6 |
| `230e` | `2300` | 4 = 4 | 24 = 24 | 14 → **0** |
| `58a1` | `58a0` | 11 = 11 | 5 = 5 | 1 → **0** |
| `a15f` | `a140` | 20 = 20 | 10 = 10 | 31 → **0** |
| `43ff` | `03e0` | 8 → **0** | 31 = 31 | 31 → **0** |
| `31a6` | `0006` | 6 → **0** | 13 → **0** | 6 = 6 |
| `21a1` | `2000` | 4 = 4 | 13 → **0** | 1 → **0** |

The surviving channels are **exactly right**, which rules out an arithmetic error in the
blend.

**The first inference drawn from that was WRONG and is left here as a worked example of a
plausible-but-false reading.** It said: a per-channel sub-table whose base or stride is
wrong reads zeros instead of its table, and the sibling `CDDrawShadeBlit` uses precisely
that idiom — three sub-tables of `?g_clut@@3PAEA` at 64 KB strides (`DIR32` relocations with
inline addends `0x2`, `0x10002`, `0x20002`) — so a missing `+0x10000`/`+0x20000` would
produce exactly this.

It is wrong because it never checked the packer. Decoding the failure to `buf[]` indices
(section at the end of this file) shows every retail value is one of `Shape3`'s own `cNN`
constants, and `buf[1..8]` (`c00 = Pack(0x4e,0x78,0x1c) = 4bc3`) **agrees on both sides**,
exercising the variable-shift packer end to end. So the colour computation is fine and the
defect is in WHICH CONSTANT reaches WHICH RANGE. The lesson: "one channel is zero" is a
signature of the *value selected*, not necessarily of how it was computed — and a sibling
TU using an idiom is not evidence that this TU uses it.

A second, smaller effect is a **span boundary**: at `0x03ab9186` we write eight more
pixels of the previous span's colour (`4bc3`) where retail has already switched to
`2104`, and at `0x03ab9264`/`0x03ab92ae` we write one flat colour (`01a0`) where retail
writes two different ones (`6060`, `114e`). That is a run-length or a case-selection
difference, not a colour one.

**The other seven shapes disagree too.** Each was run against retail's own `ShapeN` on the same restored state, which is a valid
OURS-vs-RETAIL comparison even though the state was captured for Shape3:

    Shape1  453 of 524 B differ      Shape5  483 of 524 B differ
    Shape2  444                      Shape6  483
    Shape4  437                      Shape7  457
                                     Shape8  451

Caveat, stated because it matters: a cross-run's state is the one a SIBLING was called
with, so those seven counts are speculative in the way any mutated input is — only
Shape3's own row is ground truth. What they establish is that the defect is not confined
to Shape3.

`src/Gruntz/LightFxRender.cpp` is owned by another lane and was **not touched here**. The
verdict is against the compiled `build/objdiff/base/lightfxrender.obj` at commit
`2bebf202f`.

## `CMapMgr::ComputeCellFlags` — 0.00% and behaviourally correct

Its `@early-stop` note in `src/Gruntz/BrickzCellFlags_077790.cpp` claims "logic complete"
and blames the score on a delinker artefact (the jump-table data region scoring the whole
symbol at zero). The replay confirms the claim on 19 inputs, which reached switch arms with
0, 1, 2, 3, 4 and 5 bytes of effect. A 0.00% row that is *right* and a 0.00% row that is
*unwritten* are the same row on the scoreboard; they are not the same row here.

## `CChatBoxOwner::HitTest` — a function that writes nothing

Its whole observable is the return value, which is compared, plus the negative claim that
neither side touched any of the ~130 MB of restored state. One input would have been a weak
test, so it was swept over both of its screen-coordinate arguments (0, 1, 100, 320, 479,
480, 639, 640, −1) — 18 extra inputs, all agreeing, with no game launch.

## Not reached, and why

| function | fuzzy % | why |
|---|---|---|
| `CGrunt::Activate` | 33.60 | **structural**: an ILT thunk and ZERO direct `call rel32` sites, so the capture has nothing to patch. Needs the prologue detour. |
| `CDDrawShadeBlit::ConvertRow` | 54.83 | fired 6057× in one probe session and 0× in two capture sessions |
| `CDDrawShadeBlit::ConvertRowFlip` | 49.99 | never fired; rides on a ConvertRow capture via `--cross` (identical signature) |
| `CDDrawShadeBlit::ConvertRowDouble/Fwd` | 48–55 | never fired; on the `BlitLoop` path the sessions did not take |
| `CFaderShape::RenderTile` | 59.02 | never fired |
| `CMotionState::Step` | 63.79 | fired 4–24× in probe sessions, 0× in five capture sessions |
| `CWwdSpatialMgr::ScrollTo` | 34.99 | fired 249–2980× in probe sessions, 0× in four capture sessions |
| `CTriggerMgr::HitTestCell` / `CellHitTest` | 80–86 | same |
| `CGameLevel::MoveToward` and the four `MoveHandler`s | 70–83 | same; one MoveToward capture would give all six via `--cross` |

All of these except `CGrunt::Activate` are HOOKABLE and BIND CLEANLY — the plumbing is
ready and the missing ingredient is a session that calls them. See the session-coverage
note in `../replay/README.md`.

## Method notes

* The snapshots stay under `build/replay/snap/<name>_<rva>/`. Everything after the
  capture — re-running, mutating, fuzzing, cross-running a sibling — costs seconds.
* `fuzz.py --arg K` mutates the K-th incoming stack argument. `fuzz.py` puts everything
  supplied with `--oob` in the out-of-distribution bucket, which under-claims for a
  hit-test whose arguments are ordinary screen coordinates; under-claiming is the safe
  direction and the values used are recorded above.
* A mutated input that does not terminate is killed by the watchdog and reported as
  TIMEOUT rather than as a disagreement. `ComputeCellFlags` with `y = -1` is one: the
  8-neighbour walk runs away on both sides, and an input the program can never produce is
  not evidence about either.
* `replay.exe --show N` prints N differing bytes per verdict (default 32). A DISAGREE is
  not diagnosable without the whole list — the 8-pixel grouping above is invisible in the
  first 32.

---

## `Shape3` decoded to `buf[]` indices — the usable residue

`buf[0] = 0x03ab9174`, so every differing address maps to an index. **Every retail value
below is exactly one of the `cNN` constants declared at the top of `Shape3`.** 175 wrong
pixels across 31 ranges.

    buf[] range     retail          buf[] range      retail
    buf[9..16]      2104 (c01)      buf[221..224]    fe88 (c10)
    buf[40..73]     21a1 (c02)      buf[225..228]    43ff (c12)
    buf[77..84]     58a1 (c08)      buf[229..232]    47e8 (c13)
    buf[117..118]   b1e1 (c05)      buf[233..236]    f924 (c14)
    buf[120..123]   6060 (c06)      buf[237..238]    a15f (c11)
    buf[125..126]   b1e1 (c05)      buf[239..240]    fc85 (c15)
    buf[144..155]   230e (c04)      buf[243..246]    31a6 (c17)
    buf[157..158]   114e (c03)      buf[247..248]    fc85 (c15)
    buf[160..163]   230e (c04)      buf[249..250]    d6ba (c16)
    buf[165..166]   114e (c03)      buf[251..252]    47e8 (c13)
    buf[168..179]   230e (c04)      buf[253..254]    f924 (c14)
    buf[213..214]   3186 (c19)      buf[255..256]    a15f (c11)
    buf[215..216]   a504 (c20)      buf[260], [266]  b1e1 (c05)
    buf[217..218]   d6ba (c16)      buf[270..281]    21a1 (c02)
                                    buf[286..294]    58a1 (c08)
                                    buf[302..323]    b307 (c18)

The SHAPE of it is the lead: short 2- and 4-pixel runs alternating between a few constants
(`c03`/`c04` interleaving at `buf[157..179]`, `c13`/`c14`/`c11`/`c15` cycling at
`buf[229..256]`). That is a run-length / case-selection structure, so the defect is in the
run boundaries or the case ordering — not the colour values.

**The `ours` column is deliberately omitted.** The module measured was baked from the object
at `2bebf202f`, which predates the palette rebuild; every `retail` value matches a constant
in the current source, but no old `ours` value does, so the table had already changed
underneath the measurement. Retail's column is still ground truth. The old column is not,
and printing it would invite someone to "fix" a difference that no longer exists.

**There is no way to re-measure.** The replay harness is gone. Any instruction in an older
revision of this file to run `recomp/replay/verdict.py` is dead — that path does not exist,
and `GRUNTZ.EXE` must not be launched. Use the table as a static specification.
