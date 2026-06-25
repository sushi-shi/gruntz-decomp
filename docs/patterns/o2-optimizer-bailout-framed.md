# A complex function the MSVC5 /O2 optimizer gives up on emits a FRAMED, all-memory body
tags: cpp:local | asm:mov asm:sub | topic:wall topic:regalloc

symptoms: push ebp / mov ebp,esp / sub esp,0x30 prologue under /O2; every local is
[ebp-N] (no register allocation); a framed function in a TU whose siblings are
frameless; your /O2 recompile of the same logic comes out FRAMELESS + register-
allocated (the opposite codegen mode)

When one retail function in an otherwise-frameless `/O2` TU opens with
`push ebp; mov ebp,esp; sub esp,N` and keeps EVERY local on the stack
(`mov [ebp-N], ...` for each), the MSVC 5.0 optimizer **bailed out** on that
function — too many simultaneous locals/temps/basic-blocks for its register
allocator — and fell back to unoptimized, framed codegen for that ONE function.
(Recall `/O2` is frameless, `/O1`/`/Os`/`/Od` are framed — see matching-patterns.)

```asm
push   ebp
mov    ebp,esp
sub    esp,0x30          ; large frame for ~12 stack locals
mov    [ebp-0x30],ecx    ; this spilled
...                       ; every local lives at [ebp-N]; no regalloc
```

WALL: a clean `/O2` recompile of the same logic comes out frameless +
register-allocated (the opposite mode) because your source has FEWER live temps,
so it stays under the optimizer's bailout threshold. You can't reliably force
MSVC to give up from natural source; matching needs the function in an `/O1`/`/Od`
TU (or to reproduce the exact temp pressure that trips the bailout). Evidence:
Gruntz CFileImage::DecodeRun8/DecodeRun24/RunDecode1/RunDecode3 (DIRSURF.CPP RLE
run-decoders, 0x140aa0/0x140c50/0x145270/0x1453f0) are all framed in retail while
their sibling blitters (Blit/BlitSurf/FillPalette, same TU) are frameless `/O2`.

## Variant: framed-but-fully-OPTIMIZED (a `/Oy-` module), not a bailout

A DIFFERENT framed shape: `push ebp; mov ebp,esp` BUT the body is fully `/O2`
register-allocated (this in ebx, repeated constants hoisted into esi/edi). This is
NOT the optimizer giving up — it is an entire TU compiled `/O2 /Oy-` (frame-pointer
omission disabled). The registry/COM helper module (`0x1bf*`-`0x1d5*`:
`CConfigStore::OpenRoot/OpenSubKey/GetInt`, the `Stub_1bf702` COM-CLSID reader, the
`Reg*`/`Find*File` helpers) is the Gruntz example. Tell it apart from the bailout
above: a bailout keeps EVERY local at `[ebp-N]` with no regalloc; this variant
register-allocates normally and only retains the ebp frame.

MEASURED (matcher-6): adding a `framed = [/O2 /MT /Oy-]` profile and a
`src/Gruntz/ConfigStore.cpp` TU **does reproduce the ebp frame** (the missing-frame
half of the wall is genuinely cleared — `push ebp; mov ebp,esp` now matches). But
`/Oy-` is **NECESSARY, NOT SUFFICIENT**: a residual remains from the optimizer's
constant-hoisting / CSE / regalloc STATE, which `/Oy-` does not control —
- retail `OpenRoot` caches the thrice-used `0x2001f` in esi and `0` in edi and pins
  `this` in ebx (bigger frame, this also spilled); the recompile re-materializes the
  immediates each call and keeps `this` in esi -> still ~58% (was ~54% frameless).
- retail `GetInt` allocates locals via `push ecx;push ecx`, compares the member
  directly (`cmp [ecx+0x7c],0`, this stays in ecx), and **reuses the dead
  `szSection` arg slot `[ebp+0x8]` as `cbData`**; the recompile loads+tests and
  gives `cbData` a fresh slot -> ~18%.

None of that is steerable from natural source (you cannot force MSVC to reuse a
specific dead arg slot or to hoist a particular literal). So `/Oy-` is the correct
HOME for the module (frame matches, the functions are structurally right) but is an
`@early-stop` for the final sweep, which must reproduce the exact temp/constant
pressure - not just the flag - to close the last ~40%.
