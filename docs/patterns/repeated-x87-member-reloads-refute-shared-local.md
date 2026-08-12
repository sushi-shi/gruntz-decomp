# Repeated x87 member reloads refute a shared floating-point local
tags: cpp:float cpp:local cpp:member cpp:branch | asm:fld asm:fcomp | topic:codegen-idiom topic:correctness
symptoms: retail reloads the same qword member before every threshold comparison; candidate keeps or reuses one named double across the ladder
confidence: 9/10

Repeated `fld qword ptr [this+N]` instructions are source evidence when no call
or store between them can invalidate the member. They show that each expression
named the member again; a shared `double value = m_member` asserts a lifetime and
cross-arm CSE that retail does not contain.

```cpp
if (distance >= m_flightDist * high0 || distance < m_flightDist * low0) {
    // ...
} else if (distance >= m_flightDist * high1
           || distance < m_flightDist * low1) {
    // ...
}
```

```asm
fld     qword ptr [esi+flightDist]
fmul    qword ptr [high0]
fcomp   ...
...
fld     qword ptr [esi+flightDist]
fmul    qword ptr [low0]
fcomp   ...
```

In `CProjectile::AdvanceMotion` (`0x000dfd00`), deleting the invented shared
`mag` local and spelling `m_flightDist` at all eight reads raised the faithful
result from 71.5214% to 76.8162%. A six-form distance-entity matrix confirmed
that raw/absolute-value alternatives were worse, while every form retained the
same ordered 49/49 relocation sequence.
