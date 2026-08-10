# A per-arm src load below the loop guard means the void* PARAM is the cursor

tags: cpp:param cpp:cast cpp:loop | asm:mov asm:cmp | topic:codegen-idiom
symptoms: cmp dword ptr [esp+N],0x2 in-place, mov reg,[esp+param] inside each
arm after the js/jle guard, mov [esp+param],reg cursor write-back, param slot
recycled for a byte staging local
confidence: 9/10

In the CDDSurface Blit-family (`Blit(void* srcv, .., RasterRowOrder)`) retail
compares `rowOrder` straight from its stack slot and loads the source pointer
from ITS param slot inside each row-order arm, after the arm's loop guard —
and in the big bodies (Blit824) it stores the advanced cursor BACK to the
param slot each pixel. A local `u8* src = (u8*)srcv;` cannot produce this: the
two arms' identical loads get PRE-hoisted above the `cmp`, the param slot dies
and is recycled, and every downstream slot/register assignment rotates. The
devs advanced the PARAMETER itself; use the param at every site (a plain
static_cast from void*, no pun - the reinterpret_cast<u8*&> reference alias is
byte-identical but trips the cast ratchet):

```cpp
u8 idx = *static_cast<u8*>(srcv);          // the param IS the cursor
srcv = static_cast<u8*>(srcv) + 1;
```

Caveat: the extra statements are a TU-content perturbation of their own - in
Blit168 the use-site spelling flips the LUT pack's first-channel coin that the
reference spelling left byte-exact.

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
