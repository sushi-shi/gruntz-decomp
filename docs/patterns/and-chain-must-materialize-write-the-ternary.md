# An inlined predicate result MATERIALIZES: spell the caller `?:`, not `&&`
tags: cpp:expr cpp:branch | asm:test asm:xor asm:jcc | topic:codegen-idiom
symptoms: retail ends a guard with `mov reg,1 / jmp / xor reg,reg / test reg,reg / jcc` and the base has none of it - the base branches straight out of the comparison chain; every other instruction matches, same registers, same prologue
confidence: 8/10 (single measured site, mechanism corroborated by the sibling site in the same TU)
variants: bool-local-materializes-what-should-be-short-circuit.md, redundant-test-elimination-is-syntactic.md, guard-reads-the-array-element-not-the-cached-local.md

`PtInRect` is an inline `BOOL` predicate with an exclusion guard and explicit
`FALSE`/`TRUE` returns. At an inlined call site cl5 materialises the return value into
`1`/`0` and the caller then `test`s it -
that is the `mov reg,1 / jmp / xor reg,reg / test reg,reg / jcc` tail, and it is exactly
what `CStatusBarMgr::HitTestRects` @0xffcb0 shows on both sides.

Prefix the call with one more `&&` operand and our cl5 folds the WHOLE thing into a
branch chain instead, losing the materialisation:

```cpp
// base: no mov/xor at all - the last cmp jumps straight to `return i`
i32 hit = p->m_enabled && PtInRect(&p->m_rect14, x, y);
// base: xor/test/jcc tail appears, as retail has it
i32 hit = p->m_enabled ? PtInRect(&p->m_rect14, x, y) : 0;
```

Measured 2026-08-08: `CStatusBarMgr::HitTest` @0x105280 **80.63 -> 88.50**.

Two spellings that do NOT work and are worth not re-trying:
- `(a && b) ? 1 : 0` - folded away completely (back to 80.63).
- hoisting the expression into a file-static helper - cl5 refuses to inline it and emits
  a real `call` (34.58).

## The residue: CLOSED (2026-08-08), and the sieve blind spot it exposed

Retail additionally **re-tests `m_enabled` inside** the materialised expression (the outer
guard already tested it) and keeps an explicit `mov ecx,1` true arm:

```asm
  mov  ebx,[ecx+0x4]
  test ebx,ebx
  je   CONTINUE          ; outer  if (p && p->m_enabled)
  test ebx,ebx
  je   FALSE             ; the ternary's own first operand
  <PointInRect chain>
  mov  ecx,1 / jmp DONE
FALSE: xor ecx,ecx
DONE:  test ecx,ecx / jne RETURN_I
```

That is the redundant-test family (redundant-test-elimination-is-syntactic.md) - but the
two `je`s go to **different destinations** (the outer one continues the loop, the inner one
falls into the materialised false arm). The duplicate-compare fingerprint requires
"the same mnemonic, the same destination", so it reported **0 hits** on this shape;
`--any-dest` now covers it.

The residue closes by spelling the outer guard on the **array element** and the body on the
cached local - see guard-reads-the-array-element-not-the-cached-local.md. `HitTest`
@0x105280 went **88.50 -> 100.00 EXACT**.
