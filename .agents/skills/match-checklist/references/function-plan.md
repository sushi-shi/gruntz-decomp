# Function Match Plan template

Fill this in before editing. Keep it concise but concrete; replace prompts with
evidence. A naked checklist without function-specific hypotheses is not a plan.

## Evidence snapshot

```text
Function / RVA / owner TU:
Current / bank / historical MAX:
Target/base size and frame:
Calls / branches / returns / relocations:
Wall class and first real divergence:
Semantic diff:
Raw constants:
Ordered referents:
Source fingerprint and relevant prior review:
Surviving source / sibling binary / Debug-object evidence:
```

## History pass

Record:

- commits that changed this body, declaration, class, helper population, or TU;
- the source-hash transition that gained or lost historical headroom;
- exact functions with the closest emitted symptom or source family;
- pattern-index entries selected by both C++ tags and assembly symptom;
- old conclusions invalidated by a later complete body or source oracle.

## Completeness matrix

Every row needs one of: `candidate`, `checked — no evidence`, `tested`, or
`proved inapplicable`. “Probably regalloc” is not a disposition.

| Family | Status | Function-specific evidence / candidate |
|---|---|---|
| Owner, signature, calling convention, cv/ref, return type | | |
| Class/base/member identity, layout, aggregate and storage widths | | |
| Complete body, call set, constants and ordered referents | | |
| Surviving source and sibling implementations | | |
| Inline/helper/macro/operator/accessor/constructor boundary | | |
| Helper visibility, declaration order, overload and authored store order | | |
| Parameter reuse, receiver/static type and local aliasing | | |
| Local census, declaration/creation order and initialization | | |
| Scope topology, escaped homes, lifetimes and result temporaries | | |
| Statement grouping, evaluation order and expression boundaries | | |
| Guard polarity, predicate materialization, switch/ternary/if spelling | | |
| Shared exits, goto labels, success/failure region and tail placement | | |
| Loop form, counter/cursor ownership, update order, break/continue | | |
| Allocation, destructor, delete/delete[], EH and constructor layers | | |
| Standard-library, CRT, MFC, Win32 and era macro idioms | | |
| Global/static/string/FP/data identity and TU ownership | | |
| Compiler-state eligibility and applicable proven steering lever | | |

## Prioritized A/B queue

| # | Hypothesis | Licensing evidence | Exact source A/B | Expected emitted change | Result |
|---:|---|---|---|---|---|
| 1 | | | | | pending |

## Required attempt checklist

Copy every row from
[attempt-matrix.md](attempt-matrix.md) that can affect this function. Keep the
checkbox open while work remains. Close it only with one of these explicit
verdicts:

```text
[x] tested — <real-TU result and structural delta>
[x] proved — <retail/source/history evidence selects this form>
[x] checked — no evidence — <where the search was performed>
[x] proved inapplicable — <machine/source reason>
```

For a candidate family, list each concrete spelling separately. For example,
do not write `[x] inlining`; write separate rows for member inline, free/static
inline, macro, wrapper/operator/accessor, nested/flat composition,
visibility/COMDAT, helper statement order, and repeated-call versus cached
result as applicable.

Queue composition rules:

- Put correctness/model/source-lineage candidates before codegen spellings.
- Include at least one explicit inline/helper search result, even when the
  result is “no candidate found”; list the headers, sibling sites, macro family,
  COMDAT/call-set evidence, or surviving source checked.
- For a dip that moves toward retail, add the next composed lever as a child of
  that base instead of deleting it immediately.
- For each rejected candidate, record the first structural reason, not only its
  percentage.
- Before permuting, state why every earlier family is resolved and which
  register/schedule feature the campaign is intended to steer.

## Handoff

```text
Kept source and why it is humane/authentic:
Historical-MAX movement:
Exact or remaining class:
Calls/CFG/constants/referents final verdict:
Negative controls:
Full-build/MAX result:
Commit:
```
