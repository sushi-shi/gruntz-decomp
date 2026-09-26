# Statement sequencing can change LEA operand encoding

Recognizable signature: the two functions have the same instructions, register
assignments, stack layout, and branch destinations, but one unscaled `LEA`
encodes the same two input registers in the opposite base/index positions.
Both instructions compute the same sum; their SIB bytes differ.

## Measured control

In the real `gamelevelmove` translation unit, MSVC 5.0 SP3 with the project's
`/O2 /MT` profile emitted different encodings for these two forms inside the
inline extent helper:

```cpp
// One expression:
i32 top = m_extent.top + y;

// Initialize from the coordinate, then add the relative extent:
i32 top = y;
top += m_extent.top;
```

At the first expanded overlap check in `CGameLevel::BroadPhase`, the change was:

```asm
; One expression:             8d 1c 11
lea ebx, [ecx + edx]

; Sequenced initialization:   8d 1c 0a
lea ebx, [edx + ecx]
```

The other function bytes were unchanged. The sequenced form reproduced the
retail encoding, including the existing register assignments and load order.

For reproduction, use the complete source at
[a6aa271a1](https://github.com/sushi-shi/gruntz-decomp/commit/a6aa271a1),
including `CGameObject::ExtentAt` in `include/Wwd/WwdGameObjectFamily.h` and its
callers in `src/Wwd/GameLevelMove.cpp`. Run `gruntz match gamelevelmove` inside
`nix develop`, save the function comparison, replace only the two initialization
statements with the single expression above, and compare again. Keep the rest
of the TU and its headers fixed; this is a real-TU observation, not a claim that
the isolated snippet reproduces the encoding.

## Interpretation and limits

Statement boundaries can survive optimization sufficiently to affect operand
selection even when the final instruction count and data flow agree. This
control does not identify the compiler pass responsible, prove the original
source spelling, or establish that `+=` generally chooses a particular base
register. Translation-unit context and the surrounding helper still matter.

When this signature appears, test a natural initialization/update boundary and
compare the full emitted body. Preserve arithmetic types, overflow assumptions,
and observable evaluation order; splitting expressions with calls, volatile
reads, or aliasing writes requires a separate semantic check. Do not introduce
dummy locals or artificial arithmetic to request an encoding.
