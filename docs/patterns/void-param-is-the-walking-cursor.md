# A per-arm src load below the loop guard means the PARAM is the cursor

tags: cpp:param cpp:cast cpp:loop | asm:mov asm:cmp | topic:codegen-idiom
symptoms: cmp dword ptr [esp+N],0x2 in-place, mov reg,[esp+param] inside each
arm after the js/jle guard, mov [esp+param],reg cursor write-back, param slot
recycled for a byte staging local
confidence: 9/10

In the CDDSurface Blit-family (`Blit(u8* srcv, .., RasterRowOrder)`) retail
compares `rowOrder` straight from its stack slot and loads the source pointer
from ITS param slot inside each row-order arm, after the arm's loop guard —
and in the big bodies (Blit824) it stores the advanced cursor BACK to the
param slot each pixel. A local `u8* src = (u8*)srcv;` cannot produce this: the
two arms' identical loads get PRE-hoisted above the `cmp`, the param slot dies
and is recycled, and every downstream slot/register assignment rotates. The
devs advanced the PARAMETER itself; use the byte-pointer parameter directly:

```cpp
u8 idx = *srcv++;                          // the param IS the cursor
```

The old `void*` plus use-site `static_cast<u8*>` spelling was not load-bearing.
A full A/B rebuild after changing the entire family to `u8*` retained the same
per-function scores: Blit, BlitDirect, Blit248, Blit2416, DecodeRun8 and
DecodeRun24 remain exact; the four partial functions retain their prior
percentages. The codegen mechanism is advancing the parameter slot, not erasing
the pointee type.

```asm
cmp    DWORD PTR [esp+0x20],0x2      ; rowOrder tested in place, no reg load
jne    arm2
mov    edx,[esi+0x18]                ; height guard first
dec    edx
js     exit
mov    ebp,DWORD PTR [esp+0x18]      ; srcv loaded per arm, below the guard
...
mov    DWORD PTR [esp+0x40],ecx      ; big bodies: cursor written back (Blit824)
```

STEERABLE. Blit248 0x13fe60 94.31 -> 100.00 EXACT and BlitDirect 0x13ece0
78.84 -> 100.00 EXACT (with `while (i-- > 0)` for the inner byte copy);
Blit824/Blit816/Blit1624 each moved 2-7 points. Counter-example: Blit816
retail copies srcv into the DEAD palv slot instead (a plain local, memory-
homed) — check for the write-back before applying.
