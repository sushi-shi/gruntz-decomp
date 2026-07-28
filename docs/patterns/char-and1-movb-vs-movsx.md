# byte `& 1` — an `if`/`return 1`/`return 0` BRANCH keeps `movsx`; an expression narrows to `movb`

tags: cpp:int cpp:branch | asm:mov asm:movsx | topic:codegen-idiom
symptoms: movsx eax byte and eax 1, movb al and eax 1, signed char mask, flag byte & 1
confidence: 9/10

A method loads a one-byte flag and yields `flag & 1`. Retail sign-extends first
(`movsx eax, byte ptr [reg+N]; and eax,1`); a base built from the obvious
expression spelling is one byte shorter (`mov al,[reg+N]; and eax,1`).

## Cause

Written as a **returned expression**, MSVC5 /O2 sees that only bit 0 of the
loaded byte is live and runs its narrowing peephole, replacing the
sign-extending load with a partial-register `mov al`. That peephole runs on the
straight-line expression only.

Written as a **branch** it does not fire: cl first lowers
`if (w & 1) return 1; return 0;` normally (sign-extended load feeding a test),
then a later pass folds the two constant returns back into `and eax,1` — leaving
the `movsx` in place. Same instruction count as retail, one byte longer than the
narrowed form.

## Fix

```cpp
// narrows to `mov al,[..]; and eax,1`  -- NOT retail
return static_cast<char>(m_host->m_wrapFlag) & 1;

// keeps `movsx eax,byte [..]; and eax,1`  -- retail
i32 w = static_cast<char>(m_host->m_wrapFlag);
if (w & 1) {
    return 1;
}
return 0;
```

The lever is the branch, not the types: it works with an `i32` field plus a
`(char)` cast and with a genuinely `char`-typed field, and with either a `char`
or an `i32` local holding the narrowed value.

## Evidence (2026-07-28)

`CMenuPage::CanWrap` @0x183e30 **95.38 → 100.00 EXACT** on this edit alone.
A controlled `cl /O2` A/B over ten spellings — plain `f->b & 1`, `(int)f->b & 1`,
`(signed char)f->b & 1`, a `bool` field, `(char)i32field & 1`, a `char` temp, an
`int` temp, `?:`, `(f->b + 0) & 1` — ALL narrow to `movb`; only the
`if (w & 1) return 1; return 0;` branch form emits `movsx`. (`f->b % 2` emits the
signed-modulo sequence `cdq/xor/sub/and/xor/sub`, not this.)

This file previously recorded the case as a `topic:wall` ("no natural MSVC5
source produces the standalone `movsx`") with a 2026-07-13 addendum extending the
claim to the `(char)`-of-`i32` arm. Both were wrong the same way: every spelling
tested was an *expression*. The missing axis was statement FORM.

## Related

- [`gate-falls-through-to-shared-latch.md`](gate-falls-through-to-shared-latch.md)
  — the other family where the source's branch structure, not its types, is the lever.
