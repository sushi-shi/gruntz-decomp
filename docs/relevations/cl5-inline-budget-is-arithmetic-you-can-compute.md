# The cl 5.0 inline budget is arithmetic you can compute, and every constant is in c2.exe

`docs/patterns/inline-callee-frontend-cost-drives-ob1-budget.md` established the
SHAPE of the mechanism ("N statements written in the caller credit ~2N, the same
N behind an inline call spend ~N") from response curves. This entry closes it:
the bookkeeping is fully specified, every constant is a literal you can read in
`c2.exe`, and the resulting model predicts real Gruntz classes' expansion counts
exactly — including the nested level that actually cuts the sbi_rectonly builders.

## The spec

```
cb(f)      = 13 + SUM over the function's statements of cost(statement)   [front-end
                  size estimate, a SIGNED 16-BIT field; it is a PRE-optimization
                  measure, so statements /O2 deletes still count]
budget     = max(1000, 2 * cb(caller))            capped at 35000
running    = cb(caller)                            [a global, per caller]

for each candidate call site, in tuple order, with nrem = sites remaining:
    if budget < cb(callee) and cb(callee) > 40:        -> DECLINE (budget)
    if running > 35000:                                -> DECLINE (running cap)
    if depth > site allowance:                         -> DECLINE (depth)
    if cb(callee) > 40:  budget  -= cb(callee)         [cb <= 40 is budget-EXEMPT]
    running += cb(callee)
    sub      = trunc(budget / nrem)                    [nested sites get a SHARE]
    spent    = <recurse into the callee's own sites with sub, depth+1>
    budget  -= spent ;  running += spent
```

## Every constant, as a literal in c2.exe (imagebase 0x400000 — Ghidra's default)

| entry | what to look at |
|---|---|
| **`FUN_0042491e`** — the inline pass entry | `0x0042492f  movsx eax,WORD PTR [eax+0x64]` — **cb lives at +0x64 of the function record and is read SIGNED 16-bit**; `0x00424933 mov ds:0x490e70,eax` — the running estimate is SEEDED with cb(caller); `0x00424938 add eax,eax` — **budget = 2*cb**; `0x0042493a cmp eax,0x3e8` / `0x00424941 mov eax,0x3e8` — the **1000 floor**; `0x00424997 cmp eax,0x88b8` then `0x0046afa4 mov eax,0x88b8` — the **35000 cap**; `0x00424956 call 0x004249cb` |
| **`FUN_004249cb`** — the recursive expansion walk | `ret 0x8`; returns `spent` as `[esp+0x30] - ecx` at `0x00424a30` |
| the decline tests, all inside `FUN_004249cb` | `0x00424f36 mov ax,[ebx+0x64]` + `0x00424f41 cmp ecx,edx` + `0x00424f43 jl` — **budget < cb**; `0x00424a5d cmp ax,0x28; jle` — the **cb <= 40 exemption** (a second copy at `0x00424f6f`); `0x00424f49 cmp ds:0x490e70,0x88b8; jg` — the **running cap**; `0x00424f1d..0x00424f30` — the depth test (`[esp+0x30]` vs `[site+8] & 0xff`); `0x00424f0b movsx edx,[ebx+0x62]` + `0x00424f15 cmp esi,edx` — an argument-count check that declines on mismatch |
| the nested share | `0x00424bf8 mov eax,[esp+0x4c]; cdq; idiv DWORD PTR [esp+0x30]` — **trunc(budget / sites-remaining)**, then `lea edx,[eax+1]` (depth+1) and the recursive `call 0x004249cb` at `0x00424c0b`; `0x00424c29 sub eax,esi` — budget -= spent |
| `DAT_00490e70` | the running estimate |

So the model that `gruntz walls inline-model` ports from the VC6 RE is not merely
plausible on cl 5.0 — every branch of it is the literal code in our pinned `c2.exe`.

## The cost table (exact, measured with a calibrated ruler)

The ruler: a callee of four `gA[i] = row;` statements is expanded exactly 24 times
under the 1000 floor, and `floor(1000/cb) == 24` has the unique solution
**cb = 41** — so `13 + 4*7 = 41` anchors both the base and the unit. Adding m
statements of one kind to that callee and reading the expansion count brackets
`41 + m*c` for m = 1..4; the intersection pins `c` to a single integer:

| statement | cost |
|---|---|
| `gA[i] = row;` (store to a global array) | **7** |
| `int t = row + i;` (**dead** local init — /O2 deletes it entirely) | **7** |
| `gA[i] += row;` | **7** |
| `gT = gT->m_next;` | **8** |
| `extf(row + i);` (call to an extern function) | **9** |
| `gT->m_a = row + i;` (member store through a global pointer) | **10** |
| `gA[i] = gB[i] = row;` | **11** |
| `if (row == i) return;` | **11** |
| `gA[i] = gA[i+1] + row;` (load, add, store) | **12** |
| `if (row) gA[i] = 1;` | 13-14 |
| `gA[i] = row * i + gB[i];` | 13-14 |
| `for (k = 0; k < row; ++k) gA[k] = i;` | 32-33 |
| function overhead | **13** |
| an inline call statement in a caller (`leaf(i);`) | **6** |

The dead-local row is the quantified form of the doctrine: **a statement /O2
deletes costs exactly as much as one it keeps** (7 = 7). And the "2N credit vs N
spend" coefficients are not two separate empirical numbers — they are the single
`add eax,eax` at `0x00424938`: a statement adds ~7-13 to `cb(caller)` and
therefore ~14-26 to the budget, while the same statement inside a callee costs
~7-13 per expanded site.

## Worked example on REAL Gruntz classes, with a predict-then-measure

`CStatusBarItem::CStatusBarItem` (`include/Gruntz/StatusBarItem.h:61`, retail
`0x001005d0`) and `CSBI_RectOnly::CSBI_RectOnly` (`include/Gruntz/SBI_Image.h:69`,
retail `0x00101fa0`) are both header inlines, and the second's body contains the
first as a NESTED candidate. A probe TU that includes the real headers and does
`g[i] = new CSBI_RectOnly();` N times gives two decline series at once — the outer
ctor's and the nested base ctor's:

| N | declined outer | declined base |
|---|---|---|
| 4, 8 | 0 | 0 |
| 12 | 1 | 5 |
| 16 | 1 | 7 |
| 20 | 1 | 10 |
| 24 | 1 | 12 |
| 28 | 1 | 14 |
| 32 | 2 | 15 |
| 40 | 2 | 19 |
| 48 | 3 | 22 |

Running the algorithm above over that harness and searching integer parameters,
**exactly two triples reproduce all twenty numbers**: `(cb_outer, cb_base, c_site)
= (61, 55, 42)` or `(63, 56, 43)`. Note `cb_base = 55 > 40`, so the base ctor is
NOT budget-exempt; and note what cuts it — the outer ctor keeps expanding while
the nested base ctor starts declining at site 9, because the nested level only
gets `trunc(budget / sites-remaining)`.

Taking `(61, 55, 42)` and predicting **seven configurations that were not used to
fit it** — three new site counts and four with caller padding (12 cb units per
padding statement, i.e. the caller-credit half of the model):

| N | PAD | predicted (outer/base declines) | measured |
|---|---|---|---|
| 36 | 0 | 2 / 17 | **2 / 17** |
| 44 | 0 | 2 / 21 | **2 / 21** |
| 64 | 0 | 3 / 31 | **3 / 31** |
| 16 | 20 | 0 / 1 | **0 / 1** |
| 16 | 40 | 0 / 0 | **0 / 0** |
| 24 | 30 | 0 / 1 | **0 / 1** |
| 12 | 60 | 0 / 0 | **0 / 0** |

7/7. The spec is predictive on real classes, in both directions (callee cost and
caller credit).

## What it says about the sbi_rectonly builders

Fresh compile of `src/Gruntz/SBI_RectOnly.cpp` versus retail, counting DECLINED
(i.e. out-of-line) ctor calls — a decline is a call, an expansion is not:

| builder | retail RVA | retail RectOnly/Item | ours | deficit |
|---|---|---|---|---|
| `BuildStatusBarTabs` | `0x000ffde0` | 0 / 4 | 0 / 4 | **exact** |
| `BuildGameMenu` | `0x00101580` | 5 / 3 | 4 / 2 | −1 / −1 |
| `LoadTabSprites` | `0x00102250` | 10 / 15 | 7 / 10 | −3 / −5 |
| `BuildTabzDialog` | `0x0010a340` | 4 / 8 | 2 / 7 | −2 / −1 |

Every deficit has the SAME sign: we expand more than retail, i.e. **our callers
have more budget than the originals**, i.e. **our reconstructions carry more
front-end mass than the original functions did**. Injecting K statements of known
cost (12 cb units each) at the top of each builder — a disposable A/B on a copy of
the TU — moves them further away, which fixes the direction beyond argument:

| builder | retail | K=0 | K=4 | K=8 | K=16 | K=24 | K=32 |
|---|---|---|---|---|---|---|---|
| `BuildStatusBarTabs` | 0/4 | **0/4** | 0/3 | 0/3 | 0/2 | 0/2 | 0/1 |
| `BuildGameMenu` | 5/3 | 4/2 | 3/3 | 3/3 | 2/4 | 1/5 | 1/4 |
| `LoadTabSprites` | 10/15 | 7/10 | 6/11 | 6/11 | 6/11 | 4/13 | 3/14 |
| `BuildTabzDialog` | 4/8 | 2/7 | 2/6 | 2/6 | 1/7 | 0/7 | 0/7 |

`BuildStatusBarTabs` sits exactly on its boundary: four statements of added mass
break it. The other three need mass REMOVED, and the slope quantifies how much —
roughly one RectOnly decline per 8-12 statements (~100-150 cb units):

* `BuildGameMenu` ≈ **−100 cb** (≈8-10 statements)
* `BuildTabzDialog` ≈ **−200 cb** (≈16-24 statements)
* `LoadTabSprites` ≈ **−250 to −300 cb** (≈24-30 statements)

which is a structural instruction, not a knob: the original builders held that
much of their body behind something we have spelled out inline — an inline member,
a helper, or a loop where we wrote a run. (`BuildTabzDialog` also has one
duplicate `AddTail` tail, 14 vs 13 — that is a separate structural row, not
budget.) The known over-correction — converting all 72 `delete v; return 0;` runs
into an inline member — overshoots every builder AND breaks `BuildStatusBarTabs`,
which the table above explains: that edit removes far more than 100-300 cb units
and `BuildStatusBarTabs` has no slack at all.

## What `walls inline-model --gap` should incorporate

1. **Nested sites are not optional.** What cuts these builders is not the top-level
   budget but `trunc(budget / sites-remaining)` handed to the nested base-ctor site
   inside an expanded derived ctor. A flat site list cannot produce the observed
   series; the two-level list reproduces it exactly (20/20 cells).
2. **cb is computable, not just measurable.** `13 + SUM cost(statement)` with the
   table above gives per-callee cb without a compile; `--measure-cb`'s bracket is
   then a check rather than the only source.
3. **Report the gap in cb units AND statements**, signed. The useful output for a
   matcher is "this caller carries ~200 cb units (~16-24 statements) more
   front-end mass than retail", which is a search instruction for missing
   structure — not "add sites to the caller".

## Bounds

Measured 2026-08-17, pinned cl 5.0 SP3 under wine, `/O2 /MT /GX /GR`. The
cost table is exact where a single integer is shown and bracketed otherwise;
costs are per statement AS WRITTEN, so a differently-spelled statement of the
same semantics can have a different cost — re-titrate rather than assume. All
probe TUs were scratch (never in the build graph) and are deleted; the builder
A/B was on a copy of the TU, `src/` unchanged.

## The helper route is bounded too (2026-08-21, measured)

The natural reading of "the original builders held mass behind an inline
member" was tested: the 11 uniform `new CSBI_Image` registration sites in
`LoadTabSprites` were factored into one TU-static
`TabImage(mgr, code, cmd, tab, rc, key, frame, extra)` carrying the
new + SetupImage-check + delete-on-fail idiom (~50 cb by the table).
Result, read from the obj: **TabImage was DECLINED at all 11 sites** (it
exists as a real function with 11 calls; LoadTabSprites shrank 0x1d14 ->
0x1b7c), Item declines moved +3 toward retail but Rect declines moved -3
away as the freed budget re-expanded base ctors elsewhere. Reverted.

The bound this fixes: under these callers' mass, any helper above ~45 cb is
refused everywhere (the per-site share `trunc(budget/nrem)` sits below it),
and the 40-cb exemption ceiling only admits bodies of ~2-3 cheap statements
(~5-10 cb/site absorbed) - reaching the -100..-300 targets that way needs
2-3 stacked micro-helpers per site, a fitted device, not a reconstruction.
Together with the two axes already measured (statement mass, free
static-inline call sites), every practical route to retail's decline counts
is now individually bounded. These four walls are PARKED on arithmetic:
the missing mass is real, but its source spelling is under-determined by
the bytes, and every candidate spelling that fits the budget also has to
keep zero-slack BuildStatusBarTabs untouched.
