# `setcc / dec / and D / add A` names BOTH constants AND the ternary's polarity
tags: cpp:ternary cpp:branch cpp:return | asm:setcc asm:dec asm:and asm:add | topic:codegen-idiom
symptoms: setle, setg, dec eax, andb $-0x6, and 0x6, add 0x13e, two-constant select, wrong mask immediate, mask_immediates
confidence: 10/10
variants: default-then-override-flag.md, clamp-ternary-keeps-literal-in-compare.md

A branchless two-constant select. cl 5.0 lowers `c ? X : Y` (X, Y integer
constants) to a fixed four-instruction shape whose operands are a **complete,
invertible encoding of the source**:

    setcc(!c) ; dec ; and (X - Y) ; add Y

So read it straight off the target:

| you see | the source said |
|---|---|
| `setCC` | the condition is the **negation** of CC |
| `add A` | `A` is the **else** value (`Y`) |
| `and D` | the **then** value is `A + D` (`X = Y + D`) |

`D` is signed, so the *same* select appears with a positive or a negative mask
depending only on which way the author wrote it — and that is exactly what a
mask-immediate sieve reports as a wrong constant. `and $0x6 / add $0x13e` and
`andb $-0x6 / add $0x144` are the same selection written the two ways round.

```cpp
// retail:  setg %al / dec %eax / andb $-0x6,%al / add $0x144,%eax
if (roll <= t4) {
    return BRICKTILE_GOLD_1;    // 0x13e  = 0x144 + (-6)   <- the THEN value
}
return BRICKTILE_BLACK_1;       // 0x144                    <- the ELSE value

// the same values written the other way round give the other encoding:
return (roll > t4) ? BRICKTILE_BLACK_1 : BRICKTILE_GOLD_1;
//     setle %al / dec %eax / andl $0x6,%eax / add $0x13e,%eax
```
```asm
    xorl  %eax, %eax
    cmpl  %edi, %edx
    setg  %al               ; condition is  roll <= t4
    decl  %eax
    andb  $-0x6, %al        ; then = 0x144 + (-6) = 0x13e
    addl  $0x144, %eax      ; else = 0x144
```

Steerable, and the ternary/if-return/if-else spellings are interchangeable —
`cl` probe at /O2 gives byte-identical output for `(roll <= t4) ? GOLD : BLACK`,
`if (roll <= t4) { return GOLD; } return BLACK;` and the full if/else. Only the
**polarity** matters. `CGruntzMapMgr::LoadAttributes` (`PickA`) 75.21 -> 75.42
and its `mask_immediates` row (`base-only 0x6 / target-only 0xfffffffa`) closed.
Note the `andb` form is also one byte shorter, so the wrong polarity shifts every
displacement after it.
