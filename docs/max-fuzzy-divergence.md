# The gap between `Fuzzy` and `Fuzzy Max` — what is actually in it

`gruntz verify status` / the README print two numbers per module:

```
| Module   | Units |       Functions exact |  Fuzzy | Fuzzy Max |
| `Gruntz` |   233 | 1,882 / 2,561 (73.5%) |  88.6% |     88.8% |
```

`Fuzzy Max` is `Fuzzy` plus the code-weighted sum of `(best-ever − current)` over
functions now below their peak. Every point of that gap is **a source state we had
and lost**. Nobody had ever opened the set. This is what is in it, why, and what to
do differently.

Tool: `gruntz walls inventory [--history]`.

The history cache also retains every per-function source fingerprint that actually
earned a peak. `--history --same-source-only` separates code that merely resembles
an old implementation from code whose exact current fingerprint already reached
that score. `--history --restore-same-source` may re-bank those proofs, but only
when the proof was recorded after `a2fb422e0` fixed stale generated header
dependencies. Older same-fingerprint peaks remain visible as
`[pre-dependency-fix]`; they are not restored automatically because the object may
have been compiled against headers that no clean build would use today.

Measured at `f9dd558e5`, 29 lost exact rows had a fingerprint that had itself
earned 100%. Twenty-six proofs post-dated the dependency fix and were restored.
Three were deliberately withheld: `CHashBase::Insert`,
`CDDrawChildGroup::SumWeighted`, and `CImage::BlitShadeFlipH` had only
pre-fix proofs.

---

## 1. Two instruments, and why they disagree by 4x

Measured at `638503cf4`, before this lane's own fix (§3):

| instrument | functions below peak | points | code-weighted | lost a proven EXACT |
|---|---:|---:|---:|---:|
| **bank** — `best_pct` as committed in `config/match_baseline.tsv` (what `status` sees) | 37 | 118.6 | 0.094% | 3 |
| **history** — max over every revision of that file in git, keyed by RVA | **297** | **1,423** | **0.940%** | **114** |

(After §3's fix the history set is **234 functions / 1,287 points / 51 lost EXACT** —
the 63 recovered factories leave it.)

The bank understates the loss by four times, and it is not a rounding artifact:
**426 blesses have lowered a recorded peak whose RVA never moved — 3,786 rows,
15,528 points.** A flattened peak is simply gone from the file. Git still has it.

Two mechanisms, split by era:

| era | rows | points | mechanism |
|---|---:|---:|---|
| before `437b85633` (2026-07-17) | 2,512 | 8,676 | **a tool bug**: `update` did `best = pct` on any *edited* row, so the bank was not a ratchet at all |
| after `437b85633` | 1,274 | 6,851 | **`update --accept-regressions`**, which sets `best = cur` for every regressed row and records nothing about what it flattened |

The largest single flattening: `e62df8aab` ("match: bank MAX + README post
lane-4b/1h integration") lowered **126 peaks by 2,491 points** in one commit — the
entire `??0C<X>@@QAE@PAUCGameObject@@@Z` game-object ctor family, from 95–100% down
to 13–49%, RVAs unchanged. (That one was later genuinely repaired by `b1a705d20`
"restore inline game object construction"; the *evidence* of the loss was destroyed
regardless.)

### Comparability caveat (applied, not hand-waved)

`13911bfb2` ("flake: bump objdiff 3.7.1 -> 3.7.3") recalibrated the *scorer*:
partial credit got stricter and the commit re-baselined **283 rows / 322 points**.
A **partial** peak recorded before 2026-07-10 is therefore not a target — the
current scorer cannot reach it. A **100.0** peak *is* comparable across the bump
(byte-exactness is scoring-independent; the bump's own commit message says the exact
count was unchanged).

Applying that rule to the 297: **222 functions / 1,154 points are comparable** and
worth looking at; 75 carry a partial pre-bump peak. `max_divergence --history` flags
those `[pre-bump peak]`. **Do not chase them.**

---

## 2. Ranked taxonomy of causes

Method: replay every revision of `config/match_baseline.tsv`. A *fall event* is a
function whose `cur_pct` dropped between two consecutive blesses (same RVA). The
baseline stores a per-function source fingerprint, so each fall is classified by
whether the function's **own** source changed, and — if it did not — whether any
*other* function in the same unit did.

**4,592 fall events over 1,301 blesses, 36,242 percent-points lost in total.**

| rank | cause | events | points | verdict |
|---:|---|---:|---:|---|
| 1 | **self-edit** — the lane changed this function | 1,310 | 22,639 | mostly doctrine-as-intended |
| 2 | **cross-TU ripple** — nothing in this unit changed: a shared header, an inline in a header, a tool/flag change, or the target side moved | **2,613** | 10,034 | mixed; the big recoverables live here |
| 3 | **same-TU sibling ripple** — a *different* function in the same unit was edited | 597 | 2,366 | regalloc noise; permuter's job |
| 4 | unknown (whole-`.cpp` fallback fingerprint, can't attribute) | 104 | 1,304 | unattributable, stated as such |

**The headline correction to the folklore:** the campaign talks about "sibling
drops" as *the* collateral mechanism. It is the **smallest** one — 13% of events and
7% of points. The dominant collateral cause is **cross-TU**: a change in a shared
header reshaping functions in units the lane never opened. Those are the ones a lane
cannot see, because its own unit's numbers look fine.

Second correction: **~96% of all divergence has historically been recovered**
(36,242 points fell; 1,423 are still open). The structure-over-current-% doctrine is
working. What is left is a residue, and the residue is not evenly distributed.

### The still-open 297, by whether the function's own source changed

| | functions | points |
|---|---:|---:|
| self-edit | 147 | 680 |
| collateral (own source unchanged) | 149 | 737 |
| unattributable | 1 | 6 |

A near-even split — so half of what is still lost was lost by someone who never
opened the file.

### The blesses that account for the most still-open loss

| functions | points | commit |
|---:|---:|---|
| 93 | 381 | `2498ab87e` match: refresh MAX ledger after integration *(a bless-only commit over a 333-commit window; the cause is inside it, not at it)* |
| 24 | 31 | `13911bfb2` **flake: bump objdiff 3.7.1 -> 3.7.3** — artifact, see §1 |
| 23 | 92 | `cc0bac748` readme/baselines: refresh after lane bodies-1 |
| 14 | 127 | `90f056e31` cobject: unify Wap::CObject into the single MFC ::CObject |
| 14 | 93 | `81863933c` reconstruct: relocate OOL ctor/dtor COMDAT groups to their emitting TUs |
| 6 | 183 | `a15276c63` cleanup: remove artificial COMDAT emitters |

Note the shape: with one exception the top causes are **structural campaigns**, not
matching lanes. A matching lane costs its own function and knows it. A structural
campaign costs a hundred functions in units it never named.

### Sub-taxonomy of cross-TU ripple (the interesting bucket)

Ranked by points, with the mechanism each one turned out to be:

1. **Base-class ctor moved between inline and out-of-line.** `88798ee98` alone is
   2,330 points across 62 functions, and `01f253a33` ("CUserLogic ctor back inline:
   +65 derived") is the partial undo. Already documented as
   [`base-ctor-pinned-out-of-line-costs-every-derived-ctor.md`](patterns/base-ctor-pinned-out-of-line-costs-every-derived-ctor.md);
   this measurement is its price tag.
2. **A shared type unified across the tree.** `90f056e31` (Wap::CObject → MFC
   `::CObject`), `62f7c66c9` (CRect fold): 504 and 313 points of ripple.
3. **Artificial emitters removed.** `a15276c63`: 350 points over 11 functions.
   Removing a `.cpp` whose only job was to make cl emit a COMDAT copy of an inline
   correctly leaves that retail body with no emitter — the drop is honest, the
   cleanup was right, and the real fix is a *natural* emitter.
4. **The scorer changed.** `13911bfb2`: 322 points over 282 functions, zero source
   change. Pure artifact.
5. **The build graph was lying.** `a2fb422e0` ("the header dep lists were baked and
   never re-baked; close that") is where `CImage::BlitFlipH`, `CDDrawShadeBlit::
   ConvertRow` and `ConvertRowDouble` dropped. Nothing regressed at that commit —
   those units had been compiled against **stale headers** since some earlier change,
   and their recorded peaks were measured against objects that no correct build could
   reproduce. A *phantom peak*, surfaced (not caused) by fixing the dep lists.
6. **Enum / selector retyping.** See §3 — the single biggest recoverable, and not
   regalloc at all.

---

## 3. Proven mechanism, and a 63-function recovery

**Symptom.** 62 `_Create<Leaf>` worker-pump factories all sat at *exactly* 97.857%,
each with a recorded peak of 100.000%. A uniform score across dozens of functions is
never regalloc; it is one shape.

**Evidence** (`gruntz walls diagnose 0x0003d3f0 --asm`, `_CreateExitTrigger`):

```
 cmp eax,0x1d
-jg <tgt>      <- ours
+ja <tgt>      <- retail
 ...
 cmp eax,0x50
-jg <tgt>
+ja <tgt>
 ...
 cmp eax,0x3e8
-jg <tgt>
+ja <tgt>
```

Three instructions, nothing else. `jcc_sieve` buckets it `SIGNEDNESS`.

**Cause.** `33e433fad` ("naked numbers: AnimWorkerAct — one dispatch, thirteen
files, 190 labels") replaced `switch (static_cast<u32>(rec->ActKey()))` with a typed
`AnimWorkerAct act = rec->WorkerAct(); switch (act)`. MSVC 5.0 types an `enum` as
`int`, so the switch ladder became signed. **The commit message itself records that
the old spelling had the cast** — it was removed as incidental.

**Fix**, keeping every bit of the naming the enum campaign bought:

```cpp
switch (static_cast<u32>(rec->WorkerAct())) {
    case ACT_UNINITIALISED: ...
```

21 call sites plus the shared `LOGIC_WORKER_PUMP` macro in
`include/Gruntz/WorkerHandler.h`. Measured on this branch:

| | before | after |
|---|---:|---:|
| tree exact | 3,322 / 4,290 | **3,385 / 4,290** |
| tree fuzzy | 89.08% | 89.11% |
| `jcc_sieve` SIGNEDNESS bucket | 71 | **8** |

Per-function diff of the whole report across the change: **64 functions improved
(+136.3 points), 0 functions regressed.** No ripple at all — the cast changes the
condition family and nothing else, so there is no regalloc cost to trade against.

Written up as
[`patterns/enum-switch-selector-lowers-signed.md`](patterns/enum-switch-selector-lowers-signed.md).

The important part is not the 63 functions. It is that **`jcc_sieve` would have
printed all 63, by name, the day the enum campaign landed** — and it costs one
command. The enum campaign measured fuzzy%, saw a 2-point dip spread over a family,
and read it as regalloc noise.

---

## 4. What a future lane should do differently

Concrete, in the order they pay:

1. **A campaign that retypes a `switch` selector re-checks the branch shape of
   every function it touched before it lands** (`gruntz walls diagnose <rva>`
   prints both sides' branch counts; the tree-wide jcc sieve is retired). Enum domains,
   `i32` → named type, swapping a raw read for a typed accessor — all of them can
   flip a signed ladder to unsigned or back. 63 EXACT functions, one command.
2. **A campaign that edits a shared header measures the TREE, not its own
   functions.** Cross-TU ripple is 57% of all fall events. It is invisible from
   inside the lane's unit. `gruntz verify check` after the change, and put the
   collateral count in the commit message — `88798ee98` cost 62 functions 2,330
   points and said nothing.
3. **Blessing a regression must record what it flattened.** The old
   `--accept-regressions` silently rewrote `best = cur`; 1,274 rows / 6,851 points
   went that way after the ratchet was fixed and 2,512 before it. `hist_pct` is the
   answer that landed: it never resets, so a flattened `best` still leaves the peak
   visible as known headroom. Until a bank logs them explicitly, use
   `max_divergence --history`, which reads the peaks back out of git.
4. **Do not chase a partial peak recorded before 2026-07-10.** A different scorer
   produced it. `--history` marks these `[pre-bump peak]`. A **100.0** peak from any
   era is fair game.
5. **Rebuild before you believe a peak.** `a2fb422e0` proved some recorded peaks
   were measured against stale objects. A peak whose fall coincides with a
   build-system fix is a phantom, not a regression.
6. **Chase EXACT, not fuzzy.** 114 of the 297 had lost a *proven 100%* (51 still do
   after §3). Recovering one is +1 exact function for a fraction of a fuzzy point —
   §3 bought +1.46 points of the exact rate for +0.03 of fuzzy. Uniform scores across
   a family (62 functions at 97.857%) are the signature: one shape, one fix, many
   functions. `max_divergence --history --exact-only` is that worklist.
7. **A uniform sub-100 score across a family is never regalloc.** Diff one member.
   Regalloc noise is idiosyncratic; a family that agrees to four decimal places is a
   single source-level mistake.

## 5. Refuted / refined hypotheses

- **"Sibling cratered when a neighbour changed."** Real, but the *smallest* cause
  (597 events / 2,366 points). It is not where the loss is.
- **"Enum/typing change perturbing regalloc."** Confirmed as a cause, **refuted as a
  mechanism**. It is not regalloc; it is a signed/unsigned control-flow change,
  hand-fixable and fully recoverable.
- **"Delinker re-packing unpinned COMDATs — noise, not regression."** Confirmed to
  exist and separated out, but it is small in the *still-open* set: the one clean
  artifact class is the objdiff bump (24 of 297). The COMDAT-emitter removals
  (`a15276c63`) are **not** artifacts — the drop is honest and the cleanup was right.
- **"Re-home / TU-partition moves."** Present (`81863933c`, `e9fa5bacf`) but minor.
- **"A correct shape landed that cost its own function."** Largest by points and
  working exactly as intended: of 22,639 self-edit points lost, only 680 are still
  open.

## 6. Not attributed

- **`2498ab87e` (93 functions, 381 points)** is a bless over a **333-commit** window.
  The bank cannot localise inside a window; only a build sweep can, and this lane
  spent its build budget elsewhere. Named as a window, not blamed on a commit.
- **`?ParseBuffer@CSymParser@@QAEHPAXHH@Z`** (`symtab`, 952 B) is peak 78.15 → **0.00**
  with a genuine control-flow divergence (35 vs 40 basic blocks). Real, large, open,
  unexplained. A 0.00 on a written body is worth someone's attention.
- One function's fall predates any bless that recorded it.

## 7. Incidental fixes made while measuring

- **`config/labels_manifest.tsv` at `638503cf4` was unbuildable from clean.** That
  commit's cherry-pick restored `gamelevel 75`; the tree emitted 74 (lowered by
  `03a16f99e`), so the label denominator gate failed any build that actually re-ran
  the merge. `main` did not notice because the manifest was not a ninja input, so
  the edge never re-ran. Corrected to 74. (The manifest itself is retired - the
  Model is the denominator now, and every one of its inputs is a graph input.)
- **The branch decoder raised on `jecxz`**, taking the whole jcc sieve down.
  cl5 does emit it ahead of an inline `rep` block. It had to be added to the JCC
  set (no signed/unsigned twin, so a flip involving it lands in `OTHER`).

---

## 8. The baseline reproduces; the SNAPSHOT in it does not (2026-08-08)

A lane building a **pristine worktree at `main`, zero source bytes changed**, reported
12 LOST + 40 REGRESS against the committed `config/match_baseline.tsv`. If a clean
build cannot reproduce the committed baseline, every lane's "N up, M down, MAX held"
is measured against a moving reference. It was investigated as nondeterminism. **It is
not.**

### The build is deterministic — measured three ways

`report.json` is **byte-identical** across all of:

| pair | result |
|---|---|
| two from-scratch builds in the SAME worktree (build products wiped between) | identical |
| two DIFFERENT worktrees, different absolute paths, same commit | identical |
| INCREMENTAL (built at `c088e0a6f`, then `git checkout` HEAD, rebuild) vs from-scratch | identical |

`build/gen/bindings.tsv` is identical too. Two independent `cl` invocations on the
same source differ in exactly **2 bytes** — the COFF header `TimeDateStamp` at offset
4 — which is why every base `.obj` md5 changes between runs while nothing downstream
does. 27 of 343 delinked target objs also differ byte-wise run to run, and likewise
score identically. **Do not chase an obj-hash difference; chase `report.json`.**

### What actually moved: the baseline is a hand-taken snapshot with no provenance

`cur_pct` records the build that the last `gruntz verify status update` saw.
**Nothing regenerates it.** `gruntz build` only *reads* the file (`status check`); the
integrate step banks `README.md` automatically (it is written inside the build, by
`status summary --write-readme`) but the baseline only ever changes when a human runs
`update` and commits it.

At the time of measurement the committed baseline had last been written at
`c088e0a6f` — **26 commits and 200 changed source files** behind `HEAD`. 172 of 4300
rows disagreed with a clean build before anyone touched anything. Worse, even a
from-scratch build **at `c088e0a6f` itself** disagreed on **20 rows** (e.g.
`?DrawBorder@CLightFxRender` 100.00 → 88.10, `?Verify@CNetSession` 100.00 → 89.53):
the file was written from a report whose source is not the source it was committed
with — a lane worktree's state, carried in by cherry-pick.

This cannot corrupt the MAX gate — `best_pct` is a ratchet and a stale row can only
*over*-report — but it buries the one dip a lane owns among the ones it inherited,
which is how a real regression gets waved through. That is the whole cost, and it is
enough: **a baseline that cannot reproduce launders regressions.**

### Two real tool bugs found underneath it

Both were `cmd_check` disagreeing with `cmd_update` about the same row. Fixed, and
pinned by `TestCheckReportsWhatUpdateAlreadyKnows` in `gruntz verify selftest`.

1. **TOUCHED pre-empted REGRESS.** `classify` tested `real_edit()` *before*
   `pct < best`, so an **edited** function's drop below its own high-water was never
   reported — inverting the module's own doctrine, which is that editing a body is
   exactly when the ratchet must hold. **Twelve** below-best rows were invisible inside
   a 252-row TOUCHED bucket, the worst of them **−38.24**
   (`?Blowfish_decipher@@YAXPAI0@Z`, 99.88 → 61.64, whose sibling
   `?Blowfish_encipher` moved 60.41 → 100.00 in the same window — look there first).
   The below-best test now runs first; TOUCHED means "edited and NOT below best".
2. **A cross-unit COMDAT move was reported as a LOST.** `cmd_update` carries a
   high-water across a unit change **by rva alone**; `cmd_check` keyed the same test on
   `(unit, rva)` and so could not see it. **5 of the 7 "LOST" were transfers** — every
   one of them at **100.00%** under its new unit, two of them large *improvements*
   (`??_GCButeTree` 45.58 → 100.00, `??_GCTypeCollRuntime` 31.59 → 100.00) reported as
   losses. `check` now emits **MOVED** (new unit) / **RENAMED** (same unit, new mangled
   name) and gates the carried best at the row's new home, so a transfer that *dropped*
   is a REGRESS instead of an ungated NEW.

Net at `30a9b5ec9`, same build: LOST 7 → **2** (the two genuine ones are
`??1CRezBufferObject` / `??_GCRezBufferObject` in `fader`, whose rva nothing claims),
REGRESS 32 → **44** (the 12 that TOUCHED was hiding).

### The third LOST category: a preserved-absent row reports forever

`cmd_update` deliberately RETAINS a baseline row whose body no current source claim
emits (`preserved_absent`), so an inline COMDAT that temporarily loses its emitter
keeps its high-water. Correct — but `check` then prints it as LOST on **every**
subsequent build until an emitter returns, so a lane inherits a standing LOST floor it
had no part in. `a15276c63` (remove artificial COMDAT emitters) created a batch of
these; `fef59116a` and `30a9b5ec9` gave most of them real emitters again.

This is what the original 12-LOST report was made of. **Every single row it named is
at 100.00% today** — `?PointInBounds@CGameLevel`, `?ResetParamBlock@CGameLevel`,
`??0WwdRegion`, `??0WwdDirtyRect`, `??0WwdGridNode`, `??1CWorldSoundSet`,
`??0CUserLogic` (best 89.04 → 100.00), `??0CGameObject` (91.06 → 100.00) — some in
their original unit, some transferred. **Not one was a loss.** Read a LOST row as
"nothing emits this body right now"; check the rva before calling it a regression.

### `check` now states its own currency

```
BASELINE CURRENCY: 172 of 4298 rows differ from the cur_pct snapshot in
config/match_baseline.tsv, so that file describes an OLDER tree than this build.
  Of the 44 REGRESS above, 14 were ALREADY below best in that snapshot (inherited)
  and 30 are NEW since it.
```

**`carried` is not yours; `fresh` is the set you are answerable for.** Read that line
before attributing anything to your own edit.

### What a lane / the orchestrator must do

- **Commit the baseline with the source it measures.** `status update` belongs in the
  integrate step next to the README bank, at the *integration* commit — not in the
  lane worktree whose tree differs from the merge result.
- **Do not auto-refresh it from the build.** It would erase the drift signal (`cur` is
  what makes carried-vs-fresh computable) and dirty 25 worktrees on every build.
- **Never diagnose drift from obj hashes or from a current-% total.** The pipeline is
  deterministic; if a number moved and the source did not, the reference moved.
