# Dense x87 floating-point expression: the fxch stack schedule is not steerable from C
tags: cpp:float | asm:fxch asm:fsin asm:fcos asm:fld asm:fmul | topic:wall
symptoms: a multi-term `double` expression (esp. with sin/cos/sqrt) lowers to a block of `fld/fxch/fmul/faddp/fstp` where the recompile's stack ordering diverges from retail; the integer scaffolding around it (control flow, member stores, `__ftol` rounds) is byte-identical; ~60-75% plateau
confidence: 7/10

MSVC 5.0's x87 code generator keeps intermediate `double`s on the 8-deep FPU
register stack and reorders them with `fxch` to feed each `fmul`/`faddp` from the
right slot. For a non-trivial trajectory/physics expression — several products and
sums of member doubles, often gated by `fsin`/`fcos` — the exact sequence of
`fld st(N)` / `fxch st(M)` / `fstp [esp+K]` spills depends on the compiler's
internal expression-DAG scheduling, NOT on any source-visible ordering. Reassociating
the C expression, splitting it into named temporaries, or reordering the terms
changes WHICH spills happen but does not converge on retail's schedule — there is no
source spelling that pins the x87 stack the way the retail build's scheduler did.

This is the FP analogue of the integer register-coloring wall: the VALUES computed
are identical (so the result stores match), but the temp-register / stack-slot
choreography differs byte-for-byte.

WALL. Recognize it and stop: reconstruct the math faithfully, get the surrounding
integer code (the branch structure, the `mov`-to-member stores, the `__ftol`
truncations into screen coords) byte-exact, accept the FP block as the residual.

```cpp
// CProjectile::StepMotion (0xe08b0): integer scaffolding byte-exact, the parabola
// block (fld m_250 ... fsin/fcos ... fxch x10 ... fstp m_1a0/m_1a8/m_250) is the wall.
double s = sin(m_250), c = cos(m_250), amp = (double)g_645584;
m_1a0 = m_240 + m_238 * m_198 * s - (-m_230) * amp * c + m_250;  // value correct
m_10->m_5c = (long)m_1a0;                                         // __ftol store matches
```

```asm
e0935: d9 c1  fld   st(1)
e0937: d9 fe  fsin
e094b: d9 ca  fxch  st(2)     ; <- the fxch choreography retail emits; recompile differs
e094d: d9 ff  fcos
...
e09cd: dd 9e a0 01 00 00  fstp QWORD PTR [esi+0x1a0]   ; result store: matches
```

Evidence: CProjectile::StepMotion 70.7% (full control flow + muzzle-snap + expire
path + all four `__ftol` screen-coord rounds byte-identical; only the sin/cos
parabola fxch block diverges).

variants: [[statement-schedule-faithful]] (integer statement scheduling),
[[outparam-zeroinit-scheduling]] (integer temp scheduling).
