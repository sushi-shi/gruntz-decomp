# cl 5.0 latches a NONZERO `#pragma inline_depth` at the compiland's first function definition — only depth 0 is honoured per function

**Tags:** `cpp:inline` `cpp:ctor` `cpp:dtor` `cpp:pragma` | `topic:codegen-idiom` `topic:wall`
**Confidence:** 10/10 (nine-cell sweep + five placement cells, `/FAs` listing + objdiff)

## The rule

`#pragma inline_depth(N)` in a cl 5.0 compiland behaves in **two different ways**
depending on `N`:

| `N` | behaviour |
|---|---|
| `>= 2` | **inert.** `2`, `3`, `4`, `8`, `16`, `255` and "no pragma at all" all produce the identical object. The natural depth these bodies need is 2, so any cap at or above it is a no-op. |
| `1` | **live, but whole-compiland.** It is latched at the **first function definition the translation unit sees** — which, with any `#include`, is a function in a header. Written above the includes it changes the whole TU; written *after* them (or immediately above one definition) it is **inert**, and a later `#pragma inline_depth()` restore does **not** undo it. |
| `0` | **live and per function.** Honoured wherever it appears, and `#pragma inline_depth()` afterwards restores the default for the following definitions. This is the only site-selective inlining control cl 5.0 gives you. |

Measured placements for `inline_depth(1)` in `src/Bute/ButeMgr.cpp` (`del` = in-body
`operator delete`, i.e. an inlined destructor that folded; `ctor` = out-of-line
`??0CButeValue` calls of 7 sites):

| placement | effect |
|---|---|
| line 1, before every `#include` | **live** (ctor 1 -> 2, copy 1 -> 0, del 0 -> 1) |
| line 1, restored right after the includes | same as above — the restore is ignored |
| just before one `#include` in the middle of the block | inert |
| after the whole include block | inert |
| immediately above one function definition | inert |
| `inline_depth(0)` immediately above one function definition | **live, that function only** |

## Why it matters

The table below is a historical causation panel, not a retained workaround. The
2026-08-14 per-arm-return reconstruction of `CButeValue::CopyValue` closes the Set*
family naturally; no `inline_depth` pragma remains in `ButeMgr.cpp`.

`docs/patterns/ob1-budget-cutoff-is-a-prefix-visibility-cannot-reach.md` previously
recorded that "`#pragma inline_depth(0)` in the TU is accepted by cl 5.0 with no warning
and produces a byte-identical obj". **That is wrong** and it closed off a live lever for
a whole campaign. Re-measured 2026-08-08 on the `CButeMgr::Set<T>` family
(`??2@YAPAXI@Z` / `??0CButeValue` / `??3@YAXPAX@Z` / `??1CButeValue` reloc census per
function, plus `report.json`):

| config | `??2` | out-of-line ctor | in-body `??3` | `??1` | family fuzzy (9 fns) |
|---|---|---|---|---|---|
| retail | 9 | 4 | 1 | 1 | — |
| no pragma / depth >= 2 | 11-12 | 1-2 | 0-4 | 0-1 | 592.28 |
| depth 1, whole TU | 11 | 2 | 1 | 1 | 621.84 |
| depth 0, whole TU | 6 | 7 | 0 | 3 | 491.13 |
| depth 0, four functions | 6 / 11-12 | 7 / 1-2 | 0 | 3 | 605.11 |

## The trap that hides it

An inlined callee that no site calls out of line **loses its COMDAT**. Whole-TU
`inline_depth(1)` on `butemgr` deleted `?CopyValue@CButeValue@@QAEPAU1@PAU1@@Z` and
`??6ostream@@QAEAAV0@P6AAAV0@AAV0@@Z@Z` from the object — both are real retail bodies
with `RVA()`/`RVA_COMPGEN()` pins, so the unit's labelled-function count fell 60 -> 58
and the `merge_labels` denominator gate fired. A depth change that raises the per
function numbers can still be a net structural loss; check the label count, not just
the score.

## Related

[`ob1-budget-drops-the-inlined-dtor-and-the-return`](ob1-budget-drops-the-inlined-dtor-and-the-return.md)
— the reproducible wrong-code probe whose retail diagnosis is now refuted.
[`ob1-budget-cutoff-is-a-prefix-visibility-cannot-reach`](ob1-budget-cutoff-is-a-prefix-visibility-cannot-reach.md)
— the expansion-count residual that survives every depth.
