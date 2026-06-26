# x87 zero-clamp compare: put the constant on the LEFT to preload it
tags: cpp:float | asm:fcomp asm:fldz asm:fxch | topic:codegen-idiom
symptoms: fcoml [mem] vs fcomp st(1), reloaded 0.0 constant, `disc < 0.0`, fsqrt clamp, testb $0x1 vs testb $0x41
confidence: 7/10
variants: x87-fp-stack-schedule.md, x87-copypaste-vs-inline-fp-block.md

A `disc < 0.0` clamp (`if (disc < 0.0) disc = 0.0;`) before an `fsqrt` lowers to
`fcoml [g_zero]` — a fresh memory reload of the zero constant. Retail instead
**preloads** the zero onto the x87 stack mid-expression and does a register
`fcomp st(1)`. Flip the comparison so the **constant is the left operand**
(`0.0 > disc`) and MSVC5 schedules the `fld g_zero` early, giving the reg-reg
compare byte-for-byte. Applies to ISOLATED FP functions; inside a deep loop the
surrounding fxch interleaving is the non-steerable x87-fp-stack-schedule wall.

```cpp
double disc = v * v - (target - pos) * a * g_negTwo;
if (0.0 > disc)   // NOT `disc < 0.0` — constant-left preloads the zero
    disc = 0.0;
double r = sqrt(disc);
return (v > 0.0) ? r : -r;
```
```asm
fmulp  st, st(2)        ; v*v
fldl   0x0              ; <- g_zero PRELOADED here (constant-left)
fxch   st(1)
fmull  0x0              ; *g_negTwo
fsubrp st, st(2)
fcomp  st(1)            ; reg-reg compare vs preloaded zero (not fcoml [mem])
fnstsw ax
testb  $0x41, %ah
```
Steerable. Evidence: CMotionState::ArrivalVelX/ArrivalVelY (0x16f3c0/0x16f430)
88%→100% by swapping `disc < 0.0` to `0.0 > disc`.
