# A long homogeneous short-circuit chain: collapse to one `&&` (one shared return-0 tail), NOT a per-rung `if (!Fn(...)) return 0;`

tags: cpp:branch cpp:return cpp:loop | asm:test asm:je asm:ret | topic:codegen-idiom
confidence: 8/10
variants: identical-return-epilogue-tailmerge.md, guard-skip-loop-not-early-return.md

## Symptom

A function is a long run of the SAME predicate (same callee, only the args differ),
each of which aborts the function on failure, then a single success return:

```cpp
if (!Fn(a, "X0")) return 0;
if (!Fn(a, "X1")) return 0;
... (N rungs) ...
if (!Fn(a, "Xn")) return 0;
return 1;
```

Retail shares ONE return-0 tail: each rung is `test eax,eax; je <fail>` jumping to a
single `pop…; xor eax,eax; pop…; ret`. But spelled as per-rung `if (!Fn) return 0;`,
MSVC5 /O2 **inlines a full epilogue at every rung** (`jne <next>; pop…; ret` ×N).
That bloats the body well past retail's size — and an objdiff function whose base is
much larger than the target stops fuzzy-pairing entirely (no `fuzzy_match_percent`
at all, not just a low one), so a logically-perfect reconstruction scores ~0.

This is the *opposite* direction from `identical-return-epilogue-tailmerge.md`: there
retail inlines and the recompile merges (a wall); here retail merges and the naive
per-rung spelling inlines.

## Fix — one short-circuit `&&` chain (or a single `goto fail`)

```cpp
if (a && Fn(a, "X0") && Fn(a, "X1") && ... && Fn(a, "Xn")) {
    return 1;
}
return 0;
```

Each `&&` rung lowers to `test eax,eax; je <fail>` against the ONE trailing
`return 0;`, and the lone `return 1;` is the shared success tail — exactly retail's
layout. A leading null/zero guard (`if (a && …)`) folds into the same chain (its
`test;je <fail>` matches retail's first rung).

Evidence: `CSpriteRefTable::LoadToolToyPalettes` (0xe2980, a `src` guard + 34
`LoadGruntzPalette(src,"<COLOR>TOOL/TOY")` rungs) — the per-rung
`if (!LoadGruntzPalette(...)) return 0;` form compiled to ~0x310 bytes (vs retail's
0x2cd) and failed to pair (no %); the single `&&` chain landed **100.0%**.

## When NOT to use it

Only when the rungs are PURE predicates with no side-effect between the test and the
next rung. If each rung also stores a result (`m_slot[k] = Add(name, k); if (!…)
return 0;`), the intervening store breaks the merge and retail INLINES each return-0
— there the plain per-rung `if (!r) return 0;` is what matches (e.g.
`CSpriteRefTable::BuildToolToyColorTable` 0xe2400, 100% with per-rung returns). Read
the target: shared `je <one-tail>` ⇒ `&&` chain; inline `pop;ret` per rung ⇒ keep
the `if/return`.
