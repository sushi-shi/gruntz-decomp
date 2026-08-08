# `fnstsw %ax; test $N,%ah` — N is an x87 CONDITION CODE, not a source constant
tags: cpp:float cpp:branch | asm:fnstsw asm:fcom asm:test | topic:codegen-idiom topic:scoring-artifact
symptoms: fnstsw, test ah 0x41, test ah 0x1, test ah 0x44, test ah 0x5, fcoms, fcomps, fcompl, mask_immediates, wrong mask immediate

x87 has no flags, so every float comparison ends `fnstsw %ax` + `test $N,%ah`.
`%ah` holds C3,C2,C0 at bits 6,2,0, so N selects WHICH comparison result is being
asked for — it is never a number from the source:

| N | bits | true when |
|---|---|---|
| `0x01` | C0 | st(0) < src |
| `0x40` | C3 | st(0) == src |
| `0x41` | C0+C3 | st(0) <= src |
| `0x05` | C0+C2 | less **or unordered** |
| `0x44` | C3+C2 | equal **or unordered** |
| `0x45` | C0+C2+C3 | any unordered-inclusive form |

Consequence: the *same* source comparison prints two different immediates
depending only on which side cl put in `st(0)`. `flds eps; fcompl v; test $0x1,%ah;
je` and `fcoms eps; test $0x41,%ah; jne` are both `v > eps` — the first has `eps`
in st(0) and asks C0 (`eps < v`), the second has `v` in st(0) and asks
"not (<=)". Which one you get follows from whether the value was still on the FP
stack or had been spilled, i.e. from scheduling, not from the source text.

```asm
    fcoms  <eps>            ; v already in st(0)
    fnstsw %ax
    testb  $0x41, %ah       ; "v <= eps"
    jne    <else>           ;  -> the source said  if (v > eps)
```

So a `mask_immediates`-style sieve row of `base-only 0x1 / target-only 0x41`
(or 0x5/0x45/0x44) is a **false positive** — verify the neighbouring `fcom*`
operand order instead. Measured on `CFaderMesh::ApplyInit` (base `flds
g_fxEps; fcompl [spill]; test $0x1,%ah` vs retail `fsqrt; fcoms g_fxEps; test
$0x41,%ah`, same `if (v > g_fxEps)`) and `RotateRasterize` (three vs five
`test $0x41,%ah` sites, all from the same clip comparisons). Cross-check
x87-clamp-compare-operand-order.md before deciding the source is wrong.
