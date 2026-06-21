# `rand() % var` won't reproduce MSVC5's divisor-guard modulo-peel — only a misrepresenting ternary does
tags: cpp:modulo cpp:rand | asm:idiv asm:test asm:jcc | topic:wall
symptoms: ~180 extra TRGT instructions around a rand() use, a `test divisor,divisor; je` guard + bittrick before idiv
confidence: 8/10

When retail computes `rand() % var` with a RUNTIME divisor, the original toolchain emitted a
`divisor != 0` guard plus a sign-bit-correction bittrick before the `idiv` (a modulo "peel").
Our packaged `cl.exe` emits NO such peel for any natural `%` form (signed/unsigned, named
divisor, etc. — verified across 15+ spellings). The ONLY source that reproduces the guard is an
explicit `n==0 ? … : …` ternary — which MISREPRESENTS the source. So write the true
`rand() % (d+1)` shape and accept the gap; do not fabricate the ternary.

WALL (source would have to lie). Evidence: CGrunt Stub_062e10 (1150B, 28.9% — ~180 missing instrs are 3 such peels).
