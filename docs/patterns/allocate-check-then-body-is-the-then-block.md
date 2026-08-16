# `new` factories: which arm is the fall-through names the source's if/else shape

tags: cpp:if cpp:new | asm:jcc | topic:codegen-idiom
symptoms: a `new X; check; init; AddTail; return` factory sits at 70-80% with the block skeleton
diverging at the first branch after the allocation; `gruntz walls diagnose --asm` reports `jcc B7 | fall B4`
against `jcc B5 | fall B4` and the two tails are in the opposite order
confidence: 6/10

MSVC 5.0 emits an `if (c) { S }` with the THEN-block laid out inline immediately after the
inverted branch. So for the object-factory family the fall-through arm usually reads the source
directly, and it is worth transcribing arm-by-arm instead of assuming a shape:

| retail | source |
|---|---|
| `test esi,esi` / `jne body` / `xor eax,eax` + a FULL epilogue | `if (m == NULL) { return 0; }` — an early return with its own epilogue copy |
| `test esi,esi` / `je <shared xor eax,eax at the end>` | `if (m != NULL) { ... } return 0;` — the two `return 0`s cross-jump |
| `mov eax,[esi+0x10]` / `test` / `jne <tail at END>` | `if (m->m_live == 0) { <body>; return m; }` then `delete; return 0;` |
| `mov eax,[esi+0x10]` / `test` / `je <body>` | `if (m->m_live != 0) { delete; return 0; }` then the body |

## Do NOT read row 2 off the base

**Row 2 is what OUR build collapses row-1 source into**, not an independent source shape.
`CTileTriggerContainer::AddToList3` / `AddToList3Switch` / `AddToList1` (0x116a40 / 0x116b80 /
0x116cf0) all branch `jne <body>` in RETAIL — row 1, a real early return. An earlier revision of
this file claimed they took row 2, which would have had a matcher rewrite three already-correct
guards into the shape cl produces by itself.

## The rows are not independently steerable — measure, do not generalise

Measured 2026-08-08 on all three, twice each: flipping the guard between rows 3 and 4 moves BOTH
the gate polarity and the null-return block, in opposite directions, and never lands both.

| function | shape | gate polarity | null-return block | fuzzy |
|---|---|---|---|---|
| AddToList1 0x116cf0 | `if (gate != 0) { teardown }` then body | wrong | **right** (own `xor eax,eax; jmp`) | **80.00** |
| AddToList1 | `if (gate == 0) { body }` then teardown | **right** | wrong (merged into the teardown's xor) | 77.41 |
| AddToList3Switch 0x116b80 | teardown-first | wrong | **right** | **75.78** |
| AddToList3Switch | body-first | **right** | wrong | 72.30 |
| AddToList3 0x116a40 | body-first | **right** | wrong | 80.12 |

An explicit `else` on the teardown is byte-identical to the trailing-statement form (measured on
AddToList1). So is `delete m` versus a hand-written `m->m_live = 0; ::operator delete(m)`.

The residue underneath is a three-block ROTATION no guard spelling reaches: retail lays out
`[body][shared epilogue][teardown jmp back up]`, we lay out `[teardown][body][epilogue]` or
`[body][teardown][epilogue]`. A 0..15 throwaway-declaration sweep over the whole TU left all four
functions bit-for-bit unchanged, so this is source-determined, not TU state.

## A class `new` can have two null tests

Do not delete an adjacent source null check merely because cl emits a null test around a
constructor call. For a non-trivial class construction, retail can contain both:

1. the compiler-generated allocation/constructor guard, which tests the raw result of
   `operator new` and skips the constructor when it is null; and
2. a source-authored check of the selected constructed pointer, which takes the factory's
   null-return arm.

`CShadeTableCache::HsvShiftTable` (`0x14e540`) is the compact discriminator. Retail tests `eax`
at `0x14e56e` before calling `CShadeTable::CShadeTable`, selects null or the constructor result,
then independently tests `ebp` at `0x14e58b` before either returning null or entering the body.
The six `CWorldSoundSet` factories and `WinMain` have the same two-test structure. Removing the
explicit checks from the six factories preserved their branch counts because the generated
constructor guard remained, but each candidate lost one of retail's returns and all six exact
matches. Restore the checks: the first test proves construction semantics; the second proves
source control flow.

The constructor-remodel heuristic is narrower. If source has `p = new T; if (p) p->Init();` and
`Init` is independently proven to be pure default construction returning `this`, remodeling
`Init` as `T::T()` lets the compiler replace that wrapper with its generated constructor guard.
It does not generalize to factory error handling after an already modeled constructor.

## The control that says the idioms themselves are fine

`CTileTriggerContainer::AddSwitchLogic` 0x115f60 is **100%** in the same TU with the same
`new` / null-check / `if (init failed) { delete; return 0; }` / `AddTail; return obj` idiom, and
retail gives it three separate full epilogues. So MSVC5 does not always cross-jump these; when it
does, look for the cause in the epilogue's own shape (below), not in the guard.

## Related

- [`retail-duplicates-small-return-epilogues`](retail-duplicates-small-return-epilogues.md)
- [`hand-rotated-loop-merges-the-exit-epilogues`](hand-rotated-loop-merges-the-exit-epilogues.md)
  — the same "our two `return 0`s merged, retail's did not" symptom WITH a working lever, when a
  loop is involved.
- `docs/patterns/forward-goto-hoists-target-block.md`
- [`identical-return-epilogue-tailmerge`](identical-return-epilogue-tailmerge.md) — the row-2
  residue that survives a correct row-1 source.
