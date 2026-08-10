# `mov cl,BYTE PTR [esp+N]; cmp cl,imm8` on a PARAMETER slot means the parameter is `u8`

tags: cpp:param cpp:enum | asm:cmp asm:mov | topic:codegen-idiom

symptoms: retail loads a parameter's stack home with a BYTE `mov` and compares with an
  8-bit `cmp`, where base loads the same slot with a dword `mov` and compares 32-bit;
  the compared constants are all small (bit depths, kinds, small tags) and the parameter
  is currently typed `i32` or a `GZ_ENUM` domain

confidence: 8/10

cl 5.0 does **not** narrow a 32-bit compare on range grounds. An enum parameter whose
enumerators all fit in a byte still compares 32-bit (measured on `ColorDepth`, whose
widest enumerator is 32). So a byte-wide load-and-compare of a *parameter home* is a
statement about the DECLARED type, not an optimisation: retail declared that parameter
one byte wide.

```asm
; retail                          ; base with `ColorDepth fmt`
mov  cl,BYTE PTR [esp+0x1c]       mov  ecx,DWORD PTR [esp+0x1c]
cmp  cl,0x10                      cmp  ecx,0x10
...                               ...
cmp  cl,0x8                       cmp  ecx,0x8
```

The caller is unaffected: a narrow parameter is still pushed as a full dword, and a
forwarding caller that already holds the value in a register or a slot emits the same
`push`. Verify that — a caller that has to *narrow* an int would gain an `and`/`movzx`.

## Fix — keep the domain, narrow the storage

Do not throw the enum domain away for this. `<Enums.h>` has the spelling already:

```cpp
// header + definition, both sides
i32 Build(PidHeader* src, i32 size, GZ_ENUM_PARAM(ColorDepth, u8) fmt);
```

`GZ_ENUM_PARAM(N, S)` expands to `S` on the MSVC 5.0 branch (so the byte compares come
back) and to the domain `N` under the C++20 strict-enum type check. The mangled name
moves (`W4ColorDepth@@` -> `E`), which is transparent: `RVA()` binds through our own
name on both sides.

A function-local `u8 depth = static_cast<u8>(fmt);` reproduces the same bytes, but it
is a fabricated local — prefer the parameter, which is what the byte-wide read of the
*parameter home* actually proves.

Evidence: `CDDrawShadeBlit::Build` @0x1490d0 80.39 -> **80.65**, recovering three
`cmp cl,imm8` sites; `CDDrawShadeBlit::LoadFromFile` @0x148fc0, which forwards the same
value, stayed 100.00 EXACT.
