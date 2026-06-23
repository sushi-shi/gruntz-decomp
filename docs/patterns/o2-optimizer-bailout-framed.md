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
