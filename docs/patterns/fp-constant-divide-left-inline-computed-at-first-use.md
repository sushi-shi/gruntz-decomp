# An x87 constant like `-1.0/sqrt(2)` is spelled INLINE, so cl computes it at its first use
tags: cpp:local cpp:float | asm:fld asm:fst asm:fdiv asm:fdivr | topic:codegen-idiom
symptoms: retail `fdivr <c1>` / store / `fld <c2>` / `fdiv [esp+N]` interleaved with the stores, base computes BOTH quotients up front and needs an extra `fld st(1)` to get the first one back on top of the stack, filed as an "x87 FP instruction-scheduling wall"
confidence: 8/10

A `double n = <expr>;` declaration is a STATEMENT: cl5 emits the whole x87 computation at
the declaration, which leaves the earlier value buried at `st(1)` and costs an `fld st(1)`
to restore it for its own first store. Retail computes the second quotient lazily — right
where it is first needed — because the source spells the expression at the use site and
lets cl CSE it into the scratch slot from there. Leave the second constant expression
inline; keep only the value that IS used first as a named local.

```cpp
double diag = sqrt(2.0);
double s = 1.0 / diag;                     // used first -> a local is fine
…
cells[i].m_dirX = s;
cells[i].m_dirY = -1.0 / diag;             // NOT `double n = -1.0/diag;` up top
…
cells[j].m_dirX = -1.0 / diag;             // cl CSEs it into retail's [esp+0x10] slot
```
```asm
    fst    QWORD PTR [esp+0x10]            ; diag saved
    fdivr  QWORD PTR ds:0x5e9a30           ; st = 1.0/diag
    fst    QWORD PTR [ecx+edx*8+0x4b0]     ; ... stored IMMEDIATELY
    fld    QWORD PTR ds:0x5e9a38           ; -1.0
    fdiv   QWORD PTR [esp+0x10]            ; st = -1.0/diag, computed only now
```

Read the two divisor constants out of `.rdata` before assuming a relationship between them
— a `n = -1.0 / s` spelling that "looks equivalent" is `-sqrt(2)`, i.e. twice the intended
magnitude, and the diff hides it because both sides are just `fdiv`.

Evidence: `CGrunt::Activate` @0x5caa0, filed "x87 FP instruction-scheduling wall … rarely
matches from C source" — 33.57 → **100.00 EXACT** together with two real value bugs the
note had masked (the `-1.0/s` magnitude and a `+s` where retail stores the negative
diagonal) and per-field subscripting of the record table.
