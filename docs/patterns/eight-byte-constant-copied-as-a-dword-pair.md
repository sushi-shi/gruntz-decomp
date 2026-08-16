# Two dword reads at +0 and +4 with no `fld` is an 8-byte constant COPY, not a layout bug
tags: cpp:global cpp:const | asm:mov | topic:data topic:codegen-idiom
symptoms: a `const double` global read only by `mov r32,DWORD PTR ds:X` and `mov r32,DWORD PTR ds:X+4`; a data-access sieve reporting "+0x4 lands INSIDE field (+0x0 double, 8 B)"; an 8-byte datum the x87 unit never touches; equal read counts at `+0` and `+4`
confidence: 10/10

cl 5.0 initialises a `double` (or any 8-byte scalar) member from a file-scope
constant with **two dword moves**, not with `fld`/`fstp`. So the only evidence a
`const double` global leaves behind can be a pair of integer loads at `+0` and
`+4` — which reads exactly like a mis-declared 8-byte field with something else
living at `+4`. It is not: it is the copy.

```cpp
static const double g_movingLogicMin = ...;   // .rdata, 8 bytes
CMovingLogic::CMovingLogic() { m_min = g_movingLogicMin; }
```
```asm
mov  ecx,DWORD PTR ds:0x5f04b0     ; lo dword of the double
mov  edx,DWORD PTR ds:0x5f04b4     ; hi dword - NOT a second field
mov  DWORD PTR [esi+0x38],ecx
mov  DWORD PTR [esi+0x3c],edx
```

**Adjudication rule** (what separates the copy from a real layout bug): the copy
shows the *same* access count at `+0` and `+4`, all width 4, all reads, and **no
x87 access anywhere on the datum**. A genuine two-field object shows differing
counts, differing widths, or writes. `?g_movingLogicMin@@3NB` / `?g_movingLogicMax@@3NB`
(0x1f04b0, 0x1f04b8) are the calibration pair: 13 reads at `+0` and 13 at `+4`
each, from `CProjectile`/`CMotionState`/`CMovingLogic`/`CGrunt` ctors — and the
`double` is correct. `gruntz verify data-access` counts this suppression as
`width-skip-dword-pair`.
