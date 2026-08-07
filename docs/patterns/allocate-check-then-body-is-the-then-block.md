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
0x116cf0) are one family and all three take the second and third rows: an early `if (p == NULL)
return 0;`, then `if (p->m_live == 0) { ...init...; list.AddTail(p); return p; }`, then the
teardown as the trailing statements.

## Related

- `docs/patterns/forward-goto-hoists-target-block.md`
