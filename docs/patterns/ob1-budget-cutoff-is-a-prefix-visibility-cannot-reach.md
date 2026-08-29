# The /Ob1 budget cutoff is a PREFIX of the call sites — no visibility choice can reach the middle
tags: cpp:inline cpp:ctor cpp:dtor cpp:eh | asm:call | topic:wall topic:superseded
symptoms: a function repeats one construct N times; retail expands the first few inline and CALLS the rest; every whole-callee visibility choice (all inline / all out of line) lands on one end of the range and never in the middle
confidence: 9/10

## *** SUPERSEDED (2026-08-08) — the middle IS reachable ***

The census method below is right and still the tool to use. The **verdict** is wrong.
Visibility cannot reach the middle; **callee front-end COST can**, and it walks the
expansion count down one site at a time. The cost was missing because this very
header had transcribed cl's own arm FOLD back into the source. Nine typed `delete`
arms in `~CButeValue` took butemgr 74.4752 -> 85.3231 and `~CButeValue` 99.30 -> 100.00.
Read [`inline-callee-frontend-cost-drives-ob1-budget`](inline-callee-frontend-cost-drives-ob1-budget.md)
instead of the "Why the cutoff cannot be moved from source" section below.

## The census is free — read it off `insn_seq --multiset`

Every inline expansion of a ctor that allocates leaves an `operator new` behind in the
CALLER; every out-of-line one leaves a `call ??0...`. So the reloc multiset alone gives
the complete inline census, with no disassembly reading:

```
??0CButeValue@@QAE@W4ButeType@@M@Z   base=2   tgt=4     <- retail CALLS 4 of the 7 sites
??2@YAPAXI@Z                         base=11  tgt=9     <- ...so retail expands 3, we expand 5
??1CButeValue@@QAE@XZ                base=0   tgt=1     <- retail calls the dtor once
??3@YAXPAX@Z                         base=3   tgt=1
```

`??_GCString` appearing on OUR side only is the same signal one level down: an expanded
ctor makes `box.type` a compile-time constant, so the expanded `~CButeValue` switch FOLDS
to a bare `operator delete`. Once a *call* to `CopyValue(&box)` escapes `&box`, the type is
no longer known and the expanded dtor keeps its full jump table — which is why a
half-expanded site costs far more than either endpoint.

## The measurement (CButeMgr::Set<T> family, 9 functions, 2026-08-07)

Retail's expansion set is a strict PREFIX of the site sequence and then stops dead:

| | ctor | CopyValue | ~CButeValue |
|---|---|---|---|
| retail `SetFloat` (family A) | 3 of 7 expanded | 2 of 3 | 2 of 3 |
| retail `SetRect` (family B) | 3 of 7 | 1 of 3 | 1 of 3 |
| ours, all three inline (header) | **5** of 7 | 2 of 3 | **3** of 3 |
| ours, all three out of line | **0** | **0** | **0** |

Family total fuzzy over the nine `Set<T>`: **592.28** all-inline, **491.13** all-out-of-line
(SetInt 74.2->45.3, SetDword 80.0->57.2, SetFloat 66.2->58.2, SetDouble 81.3->52.9,
SetString 82.9->57.2, SetRect 49.4->53.1, SetVector 56.2->57.4, SetRange 49.1->53.1,
SetValue 52.9->56.7; ParseAttributeFile 46.0->39.9 as collateral). `SetFloat` swings from
**+48 insns** (we expand too much) to **-64** (we expand nothing); retail is strictly
between the two, and NO whole-callee visibility choice puts it there.

Partial moves do not help either, and the reason is structural: **the budget is shared
across callees within one caller**, so taking any one callee out of line hands its budget
to the others. Lane 2m measured the single-callee version: forcing only
`??0CButeValue@...PAVCAVector@@@Z` out of line turns all of `SetVector`'s sites into
calls *and* makes the ctor body byte-exact, but cl spends the freed budget expanding
`CopyValue` instead and SetVector drops 56.2 -> 21.2. Doing ctor + `CopyValue` + `~CButeValue`
together (this measurement) does not recover it — it just reaches the other endpoint.

## Why the cutoff cannot be moved from source

Model the accepted prefix as `sum(cost) <= B`. Our expansions are byte-identical to retail's
through expansion #7, so the per-callee costs are equal; the observed cutoffs then require
different `B`, and `/O2` already implies `/Ob1`.

**Correction (2026-08-08).** This section used to claim `#pragma inline_depth(0)` "produces a
byte-identical obj". **That is false** — it was measured with the pragma in a placement where
cl ignores it. `inline_depth` *is* a live lever; see
[`msvc5-inline-depth-zero-is-the-only-live-lever`](msvc5-inline-depth-zero-is-the-only-live-lever.md).
What it cannot do is land on the middle of the range: depth 0 gives 0 of 7 expansions,
depth 1 gives 5 of 7, depth >= 2 gives 6 of 7, and retail's 3 of 7 is between two of those
cells with nothing in it. The prefix conclusion stands; only the "no lever exists" clause was
wrong. Also re-measured: cl expands **6** of 7 whether the function is compiled alone or in
the full TU — the "7 when alone" figure below is wrong, and the alone/TU difference is the
dropped destructor, not an expansion count.

## Rule

Keep the single authentic inline definition in the owning header. Record the census
(`insn_seq --multiset`) in the `@early-stop` note and stop — do not trade one end of the
range for the other, and do not introduce per-TU visibility devices to model the middle.

Related: [`ob1-inline-budget-divergence`](ob1-inline-budget-divergence.md),
[`msvc5-variable-ctor-inline-depth`](msvc5-variable-ctor-inline-depth.md),
[`count-jump-tables-to-find-missing-inline-expansions`](count-jump-tables-to-find-missing-inline-expansions.md)
(the same census in the opposite direction — there retail expands MORE than we do, and that
one IS steerable because the missing expansions were literally absent from the source).
