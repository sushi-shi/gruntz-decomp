# A shared tiny helper is inlined or CALLED per SITE — audit it, don't guess

- **Confidence:** 9/10
- **Tags:** `cpp:inline` `cpp:class` | `asm:call` `asm:mov` | `topic:codegen-idiom` `topic:method`
- **Seen:** `FreeNodePool::Push` 0x000311b0 (20 B, the `g_coordPool` free-list splice) —
  retail makes **54 real calls in 22 functions** and inlines the same splice everywhere
  else, including **twice inside functions that also call it**.

## Symptom

A tiny out-of-line member exists in retail (so cl did NOT inline it everywhere), but a
matcher who converts every site to a call — or every site to the inlined body — gets
both right and wrong at once. `CBattlezMapConfig::ResolveArrival` calls `Push` seven
times *and* inlines the splice once; `CGrunt::ArrivalReticleScan` calls it twice and
inlines it four times. A shared statement macro (`DRAIN_COORDS()`) is a **call** in
`StepBrickLayerBehavior` / `StepGooSuckerBehavior` and **inlined** in
`ArrivalReticleScan`, so the macro cannot be the unit of decision.

## The audit

Count, per function, both halves of the evidence and compare against the base objs.

- **retail calls** — scan `.text` for `e8 <rel32>` resolving to the helper **or to its
  ILT jmp-thunk** (find the thunk by scanning for `e9 <rel32>` -> helper); bucket by
  the function ranges in `build/gen/bindings.tsv`.
- **retail inlined bodies** — the inlined splice cannot use the helper's `this`, so it
  references the global **absolutely**. Search `.text` for the little-endian VA of the
  global and of each member offset it touches (`g_coordPool`, `+0x4`, `+0xc`); one
  push-splice is three references, one call is one (`mov ecx, offset g_coordPool`).
- **our side** — `llvm-objdump -dr --section=.text build/objdiff/base/<unit>.obj`:
  `IMAGE_REL_I386_REL32 ?Push@...` is a call, `IMAGE_REL_I386_DIR32 ?g_coordPool@...`
  is an inlined reference. Attribute `$L…` labels to the last non-`$L` symbol above
  them — llvm-objdump splits at every label, so a naive bucketing loses call sites.

Agreement on **both** counts per function is the acceptance test. When they disagree,
read which one: `Rcall != Bcall` is a wrong inline-vs-call decision (fix the source);
`Rcall == Bcall` with fewer refs is an **incomplete body**, not a spelling problem.

## Placement, not just counts

Counts alone can still put the call in the wrong place. Compare the byte OFFSETS of
each event within the function: retail's `ResolveArrival` inlines the FIRST splice
(offset 0x41a, before the first call at 0x598), so converting the first source site to
a call is wrong even though it moves the count toward 7.

## Related

- [`static-helper-must-be-inline`](static-helper-must-be-inline.md)
- [`reloc-sequence-diff-finds-wrong-referents`](reloc-sequence-diff-finds-wrong-referents.md)
