# An include's POSITION picks a function's register scheme (cl 5.0)
tags: cpp:include | asm:mov asm:reg | topic:codegen-idiom topic:regalloc topic:wall

symptoms: a function nobody edited loses 40 points of fuzzy; the only change in the
window is a new `#include` somewhere in its TU; the diff is pure register rotation
(which register holds the zero, which holds the shifted byte) with identical
instruction selection; two near-twin functions in one TU cannot be exact at once

confidence: 10/10 (single-axis A/B; 60+ probe builds across two campaigns; repeated
success after restoring the surviving Blowfish body; the whole `blowfish` unit is exact)

## What it is

cl 5.0 picks each function's register assignment partly from a **TU-cumulative
count of file-scope declarations parsed so far**. Adding declarations *above* a
function rotates its allocation; adding them *between* two functions rotates only
the second. The counter is quantized and non-monotone - it is not "more headers =
worse", it is a threshold you are on one side of.

`src/Crypto/Blowfish.cpp` is the clean specimen. `Blowfish_encipher` (0x16f7f0) and
`Blowfish_decipher` (0x16fc70) are near-twins - same 16-round macro, differing only
in P-index direction and the order of the two result stores - and **retail compiled
them into different register schemes**: encipher zeroes `eax` and loads the `>>16`
byte into `al`, decipher zeroes `ecx` and loads it into `cl`. Retail's decipher is
also 9 instructions longer, because in the second scheme cl folds only 3 of the 16
S3 loads into `add reg,[mem]` where the first folds 12.

Measured, one axis at a time, per-function objdiff fuzzy%:

| Blowfish.cpp | encipher | decipher |
|---|---|---|
| `<memory.h>` above both | **100.00** | 61.64 |
| `<string.h>` above both | 60.41 | **100.00** |
| `<memory.h>` above, `<string.h>` **between them** | **100.00** | **100.00** |
| `<string.h>` between them, nothing above | **100.00** | **100.00** |

So the include is not "wide vs narrow". `<string.h>` above encipher moves BOTH
functions past the threshold; `<string.h>` between them moves ONLY decipher, which
is exactly retail's split. The complete current TU is byte-exact that way.

## What the currency is - and is not

Sweeping N dummy `extern "C"` prototypes above encipher, everything else fixed:

    N =  0  2      enc 100.00  dec  61.64
    N =  4         enc 100.00  dec 100.00      <- the window, ONE declaration wide
    N =  5         enc 100.00  dec  99.63
    N =  6 .. 64   enc  59.71  dec  99.63
    N = 80         enc  60.41  dec 100.00
    N = 96 128     enc 100.00  dec  61.64

- It is **declarations**, not code. Adding two whole function *bodies* between the
  ciphers changed nothing; adding 4 prototypes flipped decipher.
- It is **not** the intrinsic set. MSVC 5.0's `STRING.H` contains no
  `#pragma intrinsic` at all (`grep pragma` finds only `#pragma once`); the earlier
  revision of this note blamed one and was wrong.
- A re-`#include` of an already-included header is a no-op (guard), so `<stdio.h>`
  or `<iostream.h>` inserted mid-file may measure as "no effect" when it was simply
  not parsed.
- **Expression spelling is not a lever here.** Eight semantically-equal spellings of
  the round macro (single vs split `^=`, `P` first, a separate `bf_F()`, masked top
  byte, swapped addend order, statement form) all produced byte-identical output in
  both states: cl canonicalises before regalloc.

## What to do

1. When a function falls with its own fingerprint unchanged, diff its TU's include
   block across the window first, then A/B the single include.
2. When two near-twin functions in one TU cannot be exact simultaneously, the split
   is reachable: put the declarations **between** them. Sweep N prototypes to find
   the window, then find a real header that lands in it.
3. Do NOT read the rotation as an unsteerable wall.
4. Do NOT reach for a `reinterpret_cast` array flatten to move it. On this TU
   `#define BF_S (reinterpret_cast<u32*>(g_bfS))` also moved the allocator, but no
   cast-free spelling of the same addresses reproduces it (row-0 decay,
   `&g_bfS[0][0]`, a two-reading union and a flat `u32[1024]` all give the 2-D
   codegen), so it is a coincidence of the optimizer and costs a cast-metric row.

## The tree-wide class this names: OSCILLATING functions

Walk every revision of `config/match_baseline.tsv` and count how often a function's
`cur_pct` crossed 100 while its `src_hash` never changed. Functions that do this are
the same mechanism, and no edit to their own body will hold them at 100:

| function | rva | peaks at 100 | transitions | now |
|---|---|---|---|---|
| `CSBI_MenuItem::Render` | 0xe82a0 | 35 | 130 | 100.00 |
| `CDDrawWorkerHost::Load`/`Save` | 0x1638c0/0x163780 | 36/35 | 72/71 | 99.98 |
| `CSBI_Image::Render` | 0xe6dd0 | 17 | 71 | 74.07 |
| `CRezImage::FillRectAt` | 0x176da0 | 13 | 27 | 66.44 |
| `CGruntPowerupSprite::Update` | 0x80410 | 11 | 24 | 89.82 |
| `CImage::BlitShadeNorm` | 0x154270 | 11 | 56 | 99.85 |
| `CPlay::StepScroll` | 0xd1ac0 | 6 | 74 | 88.03 |
| `CDDrawChildGroup::SumWeighted` | 0x15aaf0 | 4 | 43 | 99.85 |
| `CFaderSine::GetFrameCount` | 0x180400 | 4 | 8 | 100.00 |
| `zBitVec::SetSize` | 0x16e100 | 3 | 28 | 85.30 |
| `CAreaMgr::IsSameWorld` | 0x9b430 | 2 | 8 | 58.50 |

Each was verified as a pure register rotation with identical instruction selection.
Source levers probed and REFUTED on three of them: `StepScroll`'s six reassociations
of `a + (b - c)` are byte-identical; `IsSameWorld`'s eight guard/local/operand-order
spellings all land on 58.5 or worse (only DROPPING the `+ 1` retail's two `inc`s prove
is there reaches 88.2, so it is refused); `zBitVec::SetSize`'s residue is `shl eax,2`
vs `lea edx,[eax*4]` with the value dead on both sides. Treat an oscillator as a MAX
row, not a worklist item, unless its TU is small enough to place a real declaration in.

## Caveat

A mid-file include is a **state proxy**, not a claim that the public source placed the
header there. The real retail TU reached the split from a different declaration
composition: the EXE interleaves the lineage-proven `CCryptMgr::Encrypt`/`Decrypt`
(0x16f6e0/0x16f760) and `CCryptMgr(char*)` at 0x16f690 between `CCryptMgr::SetKey`
and `Blowfish_encipher`. Restoring that class plus the surviving `aword` and nested
round-macro body changed the two functions reciprocally but did not remove the retail
split. Keeping the real `<string.h>` declaration boundary between them then made all
six ordinary functions in the reconstructed TU exact. The position is load-bearing;
keep its operational source comment so an include-order rewrite cannot silently hoist it.
