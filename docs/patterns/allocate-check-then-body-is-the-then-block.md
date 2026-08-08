# `new` factories: which arm is the fall-through names the source's if/else shape

tags: cpp:if cpp:new | asm:jcc | topic:codegen-idiom
symptoms: a `new X; check; init; AddTail; return` factory sits at 70-80% with the block skeleton
diverging at the first branch after the allocation; `--blocks --diff` reports `jcc B7 | fall B4`
against `jcc B5 | fall B4` and the two tails are in the opposite order
confidence: 9/10

MSVC 5.0 emits an `if (c) { S }` with the THEN-block laid out inline immediately after the
inverted branch. So for the object-factory family the fall-through arm reads the source
directly, and it is worth transcribing arm-by-arm instead of assuming a shape:

| retail | source |
|---|---|
| `test esi,esi` / `jne body` / `xor eax,eax` + a FULL epilogue | `if (m == NULL) { return 0; }` — an early return with its own epilogue copy |
| `test esi,esi` / `je <shared xor eax,eax at the end>` | `if (m != NULL) { ... } return 0;` — the two `return 0`s cross-jump |
| `mov eax,[esi+0x10]` / `test` / `jne <tail at END>` | `if (m->m_live == 0) { <body>; return m; }` then `delete; return 0;` |
| `mov eax,[esi+0x10]` / `test` / `je <body>` | `if (m->m_live != 0) { delete; return 0; }` then the body |

The pair matters: writing the guard the "wrong" way around moves BOTH the branch polarity and
the physical order of the two tails, which is why the fuzzy score moves by ~10 points and the
skeleton diff reports a kind mismatch rather than a size drift.

`CTileTriggerContainer::AddToList3` / `AddToList3Switch` / `AddToList1` (0x116a40 / 0x116b80 /
0x116cf0) are one family. All three take the **first and third** rows: an early
`if (p == NULL) return 0;`, then `if (p->m_live == 0) { ...init...; list.AddTail(p); return p; }`,
then the teardown as the trailing statements.

**Correction (2026-08-08).** This file previously said the family takes the *second* row on the
null check. It does not: all three retail bodies branch `jne <body>` there (0x116a8b, 0x116bd0,
0x116d45), which is row 1. Row 2 is what OUR build emits from the row-1 source, and it is a
cl cross-jump, not a source shape — see
[`identical-return-epilogue-tailmerge`](identical-return-epilogue-tailmerge.md). Do not "fix" the
null check by writing `if (p != NULL) { ... } return 0;`; that spelling is what the recompile is
already collapsing to on its own.

The two rows are also **independent**, and the twins prove it: `AddToList1` was written
teardown-first (row 4) and moving it to row 3 closed its `m_live` polarity row but flipped the
null check into row 2 (80.00 -> 77.41), while `AddToList3`, already on row 3, sits at 80.12 with
the same row-2 residue. Both numbers are the row-2 cross-jump, not the row-3 edit; keep row 3,
which is the byte-evidenced order (retail's teardown block is laid out after the `return p`
epilogue in all three).

## Related

- `docs/patterns/forward-goto-hoists-target-block.md`
- [`identical-return-epilogue-tailmerge`](identical-return-epilogue-tailmerge.md) — the row-2
  residue that survives a correct row-1 source.
