# `#include <string.h>` alone re-allocates registers across the whole TU
tags: cpp:include cpp:inline | asm:mov asm:reg | topic:codegen-idiom topic:regalloc

symptoms: a function nobody edited loses 40 points of fuzzy; the only change in the
window is a new `#include <string.h>` somewhere in its TU; the diff is pure register
rotation (which register holds the zero, which holds the shifted byte) with the same
instruction selection on both sides

confidence: 9/10 (single-axis A/B, four functions in one TU, reproduced 6 times)

## What happened

`Blowfish_encipher` (0x0016f7f0, 1147 B) sat at **100.0%** for 1,006 consecutive
baseline revisions and fell to **60.4%** at `cc0bac748` with its own source byte-for-byte
unchanged. The window's only edit to `src/Crypto/Blowfish.cpp` that touched anything
above it was `#include <string.h>`, added so `InitializeBlowfish` could call `memcpy`.

Single-axis A/B on the exact pre-fall TU (`cc_wrap` + `llvm-objdump`), with the source
of every function held fixed:

| TU | `Blowfish_encipher` | `Blowfish_decipher` | `InitializeBlowfish` |
|---|---|---|---|
| no `<string.h>` | **0 differing insns** | 253 | 0 |
| `+ #include <string.h>` (nothing else) | **253** | 4 | 0 |

Adding the header, and nothing else, is the whole 100 -> 60 fall. The instruction
*selection* is unchanged; MSVC 5.0 picks different registers for the byte-extract
temporaries (retail zeroes `eax` and loads the `>>16` byte into `al`; with `<string.h>`
cl zeroes `ecx` and loads it into `cl`), and that rotates through all sixteen rounds.

`<string.h>` on MSVC 5.0 carries the `#pragma intrinsic` set for the whole `str*`/`mem*`
family. Under `/O2` (which implies `/Oi`) those pragmas are TU-scoped state, and they
move the allocator even in functions that call none of them.

## What to do

**Include the narrowest header that declares what you need.** `<memory.h>` declares
`memcpy`/`memset`/`memmove` and does *not* perturb this TU: with `<memory.h>` +
`memcpy`, `Blowfish_encipher`, `InitializeBlowfish` and `Blowfish_InitKey` are all
byte-exact simultaneously. `<string.h>` was strictly worse for the same functionality.

Anti-patterns this refutes:

- Do not conclude "register-rotation wall, no source lever" from the diff shape alone.
  The lever here is four lines above the function, in the include block.
- Do not reach for a `reinterpret_cast` flatten of a 2-D array to move the allocator.
  On this TU `#define BF_S (reinterpret_cast<u32*>(g_bfS))` also moved it (it defeats a
  CSE `Blowfish_decipher` needs), but that is a coincidence of the optimizer, not
  evidence about the original source, and it costs a cast-metric row. The cast-free
  spellings that address the same bytes - `g_bfS[0][0x100 + i]`, `&g_bfS[0][0]`, a
  `union { u32 m_box[4][256]; u32 m_flat[1024]; }`, and a flat `u32 g_bfS[1024]` - all
  produce the *2-D* codegen, so the cast is not a reproducible modelling device.

## How to find the next one

When a function falls with its own fingerprint unchanged, diff its TU's **include
block** across the window before anything else:

    git diff <last-100-rev> <fall-rev> -- <the .cpp> <its headers>

then A/B the single include with a `cc_wrap` probe. One compile per hypothesis (~10 s)
settles it, and `max_divergence --history` gives you the `<last-100-rev>`.
