# `100 - (int)E` gets folded into the FP chain — a named `double` local blocks it

tags: cpp:local cpp:cast | asm:fsub asm:fsubr asm:add | topic:codegen-idiom

symptoms: retail ends an FP chain with `fsub QWORD PTR ds:<c>` / `call __ftol` /
  `mov ecx,0x64` / `sub ecx,eax`, but the recompile emits `fsubr QWORD PTR ds:<c>` /
  `call __ftol` / `add eax,0x64` — the integer `100 - x` disappeared into a reversed
  FP subtract

confidence: 8/10

## Shape

```cpp
i32 pct = static_cast<i32>(100.0 / (static_cast<double>(y) / x - kMinusOne) - kMinusHalf);
m_toyBlendPct = 100 - pct;
```

cl 5.0 sees that the last FP op is a subtraction of a constant, so it negates the whole
expression by turning `fsub c` into `fsubr c`, converts once, and rewrites the integer
`100 - x` as `x + 100`:

```asm
fsubr QWORD PTR ds:0x5e9750
call  __ftol
add   eax,0x64
```

Binding the FP result to a real `double` local first prevents the reassociation — cl
then materializes the value as written and does the `100 -` in integers, exactly like
retail:

```cpp
double blend = 100.0 / (static_cast<double>(y) / x - kMinusOne) - kMinusHalf;
i32 pct = static_cast<i32>(blend);
m_toyBlendPct = 100 - pct;
```

```asm
fsub  QWORD PTR ds:0x5e9750
call  __ftol
mov   ecx,0x64
sub   ecx,eax
```

## Related trap in the same expression

`- -0.5` spelled as a literal is constant-folded by cl into `fadd <0.5>`, which creates
a NEW `$T` pool entry and silently blocks the reassociation above (so it looks
"almost right": one `fadd` where retail has `fsub`). If retail's reloc names an existing
global (here `_g_slopeNegHalf` = -0.5 at 0x001e9750), spell the source with that global —
then cl must emit the real `fsub <global>` and the reassociation reappears, which is
what the `double` local then closes.

## Evidence

`CGrunt::LoadAnimNameTable` @0x00049c60 (grunt TU): `- -0.5` -> 99.90% (one `fadd`
vs `fsub`); `- g_slopeNegHalf` -> 98.75% (correct `fsub`, but the reassociation fired);
`double blend = ...; (i32)blend` -> **100.00% EXACT**.
