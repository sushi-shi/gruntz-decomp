# `mov BYTE PTR [esp+N],al` into a dword-spaced slot: the local is `u8`, and it COSTS a frame slot
tags: cpp:local cpp:cast | asm:mov asm:idiv asm:call | topic:codegen-idiom topic:frame
symptoms: retail's `sub esp,0x34` where ours is `sub esp,0x24`; retail spills a computed colour/index with `mov BYTE PTR [esp+N],al` but reloads it with a full `mov eax,DWORD PTR [esp+N]`; slots are 4 apart, not 1; the callee it is passed to opens with `and reg,0xff`
confidence: 9/10

A narrowing store into a slot that is later read as a DWORD looks like a bug and
is easy to dismiss as scheduling noise. It is neither. It is the exact signature
of a **`u8` local** at MSVC 5.0 /O2:

- the STORE is byte-wide because the declared type is one byte;
- the slot is still **4-byte aligned and 4-byte spaced** (cl5 does not pack
  scalar locals), so three such locals cost **12 bytes of frame**, not 3;
- the RELOAD at the call site is a plain DWORD `mov` — cl5 does **not** emit
  `movzx` when promoting such a local to an `int` argument, so the top three
  bytes of the pushed argument are whatever the slot held before. That is
  harmless only because the callee re-masks, and **the callee's `& 0xff` is the
  corroborating evidence** that this is what the original source did.

## How to use it

`sub esp,N` is a *count of locals*, and a delta there is a modelling defect, not
regalloc. When retail's frame is bigger than yours, look for byte stores: each
`mov BYTE PTR [esp+N],al` whose slot is 4 from its neighbour is one `u8` local
you declared as `i32`. Retyping it both recovers the store width and grows the
frame toward retail's.

`CShadeTableCache::GammaTable` 0x14e9f0 - retail `sub esp,0x1c`, ours `sub esp,0x10`,
three byte stores at `[esp+0x28]/[esp+0x24]/[esp+0x20]` feeding
`FindNearestColor(pal,r,g,b)` (which opens `and ebx,0xff`). Typing r/g/b as `u8`
made the frame exact. `CShadeTableCache::FlashTable` 0x14df40 - same shape after
two `__ftol` calls, `sub esp,0x24 -> 0x30` against retail's `0x34`, 53.36 -> 54.85.

Counter-case: the same retype on `HsvShiftTable` 0x14e540 *lost* a point, and its
frame is still 0x28 against retail's 0x44. Do not apply this blind - only where
the byte stores are actually in the target. A frame that is still far off after
the retype means there are more unmodelled locals, and their absence dominates.

## Related

- `docs/patterns/inline-helper-unrolled-vs-out-of-line-call.md`
