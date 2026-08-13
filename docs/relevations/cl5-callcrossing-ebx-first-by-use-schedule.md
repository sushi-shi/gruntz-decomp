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

## The lever

When retail holds a value in EBX and our compile holds it in ESI/EDI (or vice
versa), the value that should own EBX is the one retail references FIRST after
the crossing call. Reorder the post-call source so that value's first use leads
— a semantics-preserving statement/term reorder the permuter can also find, now
with a KNOWN target instead of a blind sweep. The residual ESI/EDI split between
the other two is schedule/handle state (the C1 class the IL tap sorts); this
lever only pins the EBX pick, but that is the modal one.

## Bounds

Measured on the pinned cl 5.0 `c2.exe` 2026-08-13 with the probes above
(`build/il-probe/re/`, harness in the scratch `regalloc/` set). Three
call-crossing values; the >3 case frame-homes the fourth (unverified here).
Byte-sized values lose ESI/EDI/EBP (the char-homing exclusion) and are out of
scope. This is a PARTIAL allocator model — the EBX pick is proven, the full
processing-order traversal (the rest of the c2 regasg RE) is not done.
