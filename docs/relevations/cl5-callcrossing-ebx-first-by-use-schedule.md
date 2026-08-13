# cl 5.0 gives the first-USED call-crossing value EBX, not ESI — a use-order register lever

The modal register wall (retail binds a value to one callee-saved register, we
bind it to another — the whole-body ESI/EDI/EBX role swap) has a partial but
EMPIRICALLY-PROVEN steering lever on cl 5.0, distinct from the sibling VC6 model.

## What is confirmed in c2.exe

cl 5.0's `c2.exe` contains the SAME register preference table the VC6 area RE'd —
the 0-terminated dword sequence `{1,2,3,7,8,4,6}` = **EAX ECX EDX ESI EDI EBX EBP**
— in TWO copies (a const and a runtime copy, VC6's exact pattern), at file
offsets 0x90300 / 0x9a6a8 (both `.data`, this c2's merged layout). So the
preference RANKING is shared.

## What DIFFERS on cl 5.0 (the measured lever)

The ORDER pseudos are handed to that table is driven by the tail SCHEDULE, and
the first call-crossing value to be USED after the call takes **EBX**, not ESI.
Two probes, `/O2 /MT`, identical definitions, only the use order swapped:

```cpp
extern int compute(int); extern void sink(int);
int probe()  { int a=compute(1); int b=compute(2); int c=compute(3);
               sink(0); return a*7 + b*11 + c*13; }   // use a,b,c
int probe2() { int a=compute(1); int b=compute(2); int c=compute(3);
               sink(0); return c*7 + b*11 + a*13; }   // use c,b,a
```

Emitted value->register (from `/FAs`):

| value | defined | probe (use a,b,c) | probe2 (use c,b,a) |
|---|---|---|---|
| a | 1st | **EBX** (used 1st) | ESI (used 3rd) |
| b | 2nd | ESI | EDI |
| c | 3rd | EDI | **EBX** (used 1st) |

The value referenced FIRST in the post-call body lands in EBX both times
(a, then c). The remaining two split ESI/EDI by their schedule slots — which is
why the ESI/EDI pair flips between the two probes. Definition order alone does
NOT decide it (a is defined 1st in both yet goes EBX vs ESI); the tail use
schedule does.

## The lever, and its two proven boundaries

When retail holds a value in EBX and our compile holds it in ESI/EDI, the value
that should own EBX is the one retail references FIRST after the crossing call —
IF that first use is a SEPARATE STATEMENT whose order the source controls.
Reorder the post-call statements so that value's use leads.

TWO boundaries, each a measured negative control, narrow this sharply:
1. The residual ESI/EDI split between the OTHER two values is schedule/handle
   state, not source-reachable (FindGruntAt: coordinate-decl swap moved
   94.04 -> 88.46 WORSE).
2. When the EBX value is an OPERAND of a single commutative arithmetic
   expression, cl 5.0 CANONICALIZES the operand load order - it is invariant to
   source term order and NOT reachable (SumWeighted 0x15aaf0: four term
   orderings of `i*(screenX+sortKey+screenY+id)` ALL scored 99.85185 to five
   decimals; the EBX/[0x4]-vs-[0x5c] load order never moved).

So the lever's true domain is NARROW: a call-crossing value whose first post-
call use is a distinct, reorderable statement. Operand-position and pure-
ESI/EDI cases are C2-anchored - park them or sort with the IL tap. This is
why the earlier per-function sweeps on this class were flat: most of the
class is outside the lever's reach.

## Negative control (the boundary, proven)

`CTriggerMgr::FindGruntAt` (0x075c60, 94.04%) has a pure ESI<->EDI role swap
(diff dominated by esi:9/edi:9, NO EBX component): retail `mov esi,ecx; sar
esi,5` vs ours `mov edi,ecx; sar edi,5` on the tcol/trow coordinates. Swapping
the two coordinate declarations moved it 94.04 -> 88.46 (WORSE), and no source
order recovers the split. This is the model working as stated: the EBX pick is
source-reachable, the ESI/EDI split between the remaining two is C1 handle
state and is NOT. So the detector must target the EBX component - a swap
dominated by ESI/EDI alone is handle state, park it (or sort with the IL tap).

## Bounds

Measured on the pinned cl 5.0 `c2.exe` 2026-08-13 with the probes above
(`build/il-probe/re/`, harness in the scratch `regalloc/` set). Three
call-crossing values. The FOUR-value case was then measured: in a frameless
function the fourth call-crossing value takes **EBP** (sequence EBX ESI EDI
EBP), NOT a frame slot - a cl-5.0 refinement over the VC6 model, which frame-
homes the fourth. So cl 5.0 has FOUR callee-saved GPRs in play (EBX ESI EDI
EBP) before it spills, and EBP drops out of the pool only when the function
needs a frame pointer. Byte-sized values lose ESI/EDI/EBP (the char-homing
exclusion) and are out of scope. This is a PARTIAL allocator model — the EBX pick is proven, the full
processing-order traversal (the rest of the c2 regasg RE) is not done.
