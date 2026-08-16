# `return <callee result>` - the `int` return with NO constant anywhere to give it away
tags: cpp:branch cpp:return | asm:ret asm:jne asm:test | topic:codegen-idiom
symptoms: retail spells each null-guard `test eax,eax / jne <+1> / ret` - a one-byte `ret` the guard jumps OVER - where the base has `test eax,eax / je <shared epilogue>`; the base has FEWER `ret`s than the target; NO `mov eax,<imm>` and NO `xor eax,eax` appears on either side; every other byte agrees; the function is declared `void`
confidence: 9/10
variants: void-vs-bool-return-epilogue-split.md, identical-return-epilogue-tailmerge.md, retail-duplicates-small-return-epilogues.md

The third subcase of the `void`-vs-`int` family, and the one with no visible tell.

| subcase | success path | the tell | verdict |
|---|---|---|---|
| [void-vs-bool](void-vs-bool-return-epilogue-split.md) | `return 1;` | a `mov eax,1` retail has and you do not | steerable |
| [identical-return-epilogue-tailmerge](identical-return-epilogue-tailmerge.md) | every path `return 0;` | `xor eax,eax` scheduled between the pops | wall |
| **this one** | `return f(...);` | **nothing** - only the `ret` COUNT | steerable |

When the success path returns a CALLEE'S result, no constant is materialised
anywhere, so the disassembly of an `int` function is indistinguishable from a
`void` one instruction-for-instruction. Declared `void`, every `return;` is an
empty block, cl5 cross-jumps them all onto the one trailing `ret`, and the base is
short by one byte per guard. Declared `int` with `return 0;`, the SAME source is
byte-exact: the guard just tested the pointer that is now the return value, so
`return 0;` needs no `xor eax,eax`, and the guard block is no longer identical to
the `return f(...)` block, so the cross-jump never fires.

```asm
; TARGET - each guard keeps its own bare `ret`, the jcc skipping exactly one byte
  114f0d: test eax,eax
  114f0f: jne  0x114f12      ; 75 01
  114f11: ret                ; c3     <- `return 0;`, eax already 0
  114f12: mov  eax,[eax+0x2c]
  114f15: test eax,eax
  114f17: jne  0x114f1a
  114f19: ret
  ...
  114f3a: add  esp,0x1c
  114f3d: ret                ; `return SaveScreenshot(...)`
; BASE (void) - both guards branch to the one shared `ret`, 1 byte shorter each
       f: je   0x3b
      16: je   0x3b
```

```cpp
// WRONG - void: the two guards' empty returns cross-jump onto the final ret
void SaveFrontBufferShotImpl(...) {
    if (pair == NULL) return;
    if (pair->m_surface == NULL) return;
    SaveScreenshot(pair->m_surface, ...);
}
// RIGHT - int: byte-exact
i32 SaveFrontBufferShotImpl(...) {
    if (pair == NULL) return 0;
    if (pair->m_surface == NULL) return 0;
    return SaveScreenshot(pair->m_surface, ...);
}
```

## How to find it

`--diff` masks branch displacements, so the two sides print as an ordinary
`je`/`jne` polarity flip and the byte cost hides in an epilogue nobody reads. The
only reliable tell is the **`ret` count**: `gruntz walls diagnose <rva>` prints
each side's ret count, and `target rets > base rets` on a `void`-declared frameless
function is this. There is no tree-wide return-type sieve any more; work the
`gruntz walls inventory` worklist and read the counts per function.

Corroborate before flipping: the neighbouring functions in the same file (`i32`
with `return 0;` guards is the file's house style), and the call sites (they must
all discard the result). The mangled name changes `...X...` -> `...H...`; the
delinker pairs by RVA, so the binding survives.

## Not the wall

[retail-duplicates-small-return-epilogues](retail-duplicates-small-return-epilogues.md)
is the same SYMPTOM from a different cause - genuinely `void` returns retail simply
declined to cross-jump - and four spellings there were measured and rejected. Check
the declared return type FIRST; the cheap fix is a signature, not a restructure.

Measured 2026-08-08: `SaveFrontBufferShotImpl` 0x114f00 87.69 -> 100.00 EXACT;
`EngStr_DrawText` 0x115440, `ShowHudMessage` 0x1154b0, `ShowHudMessageAlt` 0x115520
94.29 -> 100.00 EXACT each - all three had been parked as an unsolved two-`ret`
case with four guard spellings already tried. `CGiantRockLogic::
BuildRockBreakInGameText` 0x1122a0 is the `xor eax,eax` half the same audit finds,
96.01 -> 96.56.
