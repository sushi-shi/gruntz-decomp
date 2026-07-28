# DECLARATION order picks which computation cl emits first; ASSIGNMENT order picks the registers — split them to set both
tags: cpp:local | asm:sub asm:mov | topic:codegen-idiom topic:regalloc
symptoms: two independent locals computed from parameters; the two computations are emitted in the wrong ORDER, or in the right order but in SWAPPED registers, and fixing one breaks the other; ~96-99.8%
confidence: 8/10
variants: sib-base-index-follows-local-decl-order.md, macro-local-decl-order-picks-param-home.md, arg-store-order-steers-schedule.md

For two independent locals seeded from parameters, MSVC 5.0 /O2 uses **two separate**
source properties, and a combined `T a = ...; T b = ...;` ties them together:

| source property | what it controls |
|---|---|
| **declaration** order | which computation is EMITTED first (and therefore which value's `abs`/derived sequence is the one spilled to a callee-saved register) |
| **assignment** order | the register assignment — the **last-assigned** local takes the first-allocated callee-saved register (`ebx`), the other takes `edi`, and the parameter LOADS follow their deltas |

So when retail emits the computations in one order but holds the values in the
registers the *other* order implies, `T a; T b;` alone can never match it — split the
declarations from the assignments and set the two knobs independently.

```cpp
// retail: `sub ebx,ecx` (dy) emitted FIRST, but dy lives in ebx (= last-assigned)
i32 dy, dx;      // dy declared first  -> dy's subtraction is emitted first
dx = x1 - x0;    // dy assigned last   -> dy gets ebx, dx gets edi
dy = y1 - y0;
if (abs(dx) > abs(dy)) { ... }
```
```asm
sub ebx,ecx      ; dy = y1-y0   (emitted first; ebx == y1's load register)
sub edi,esi      ; dx = x1-x0
mov eax,ebx      ; abs(dy) computed first -> spilled to ebp
...
mov eax,edi      ; abs(dx) second -> stays in eax
cmp eax,ebp      ; cmp abs(dx),abs(dy)   (source-order operands)
```
STEERABLE. `CMapMgr::LineIsClear` @0x82250 99.75 -> **100 EXACT**: the combined
`i32 dx = x1-x0; i32 dy = y1-y0;` gave the right registers with the subtractions
emitted dx-first; swapping the two combined declarations fixed the order but moved
`y1` from `ebx` to `edi` and cascaded through the whole body (96.0%). The comparison
SPELLING is a third, independent thing: cl emits `cmp <left>,<right>` in source
operand order, so `abs(dx) > abs(dy)` and `abs(dy) < abs(dx)` differ only in the
`jle`/`jge` byte — it does NOT reorder the operand evaluation.
