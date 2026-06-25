# Branchless `cond ? value : 0` mask: the `neg/sbb/and` destination register (eax vs edx) is a free-list pick
tags: cpp:branch cpp:local | asm:neg asm:sbb asm:and | topic:wall topic:regalloc

symptoms: a `cond ? ptr : 0` argument (a redundant non-null re-test of a pointer already
known live) lowers branchlessly to `mov R,cond; neg R; sbb R,R; and R,value` — the all-ones
mask `&`ed with the value. The whole sequence (and the surrounding function) is byte-exact
EXCEPT the **destination register R**: retail materializes it in `eax`, the recompile in `edx`
(or vice-versa). 1-region residual, ~99.3% on an otherwise byte-identical function.

The mask lands in whichever scratch register the allocator frees first across the immediately
preceding compare. Here the guard `if (a == b)` loads `a`->eax, `b`->edx, `cmp edx,eax`; both
are dead afterward, and the allocator's pick of which to reuse for the mask is NOT steerable
from source: none of an inline ternary, an explicit `(cond ? -1 : 0) & (i32)value` mask, a named
`i32` local, nor swapping the `==` operands moved retail's `eax` choice off `edx`. Same compiler
(MSVC 5.0), so a spelling may exist, but the four obvious ones don't reproduce it.

```cpp
// retail picks eax, our cl picks edx for the masked result — both byte-exact otherwise:
Unlink(e ? node : 0);                         // node = &e->m_link (already in ecx)
// equivalent, same edx pick:
Unlink((DSoundLink*)((e ? -1 : 0) & (i32)node));
```
```asm
; retail (what you SEE):           ; our cl:
mov  eax, esi                      mov  edx, esi
neg  eax                           neg  edx
sbb  eax, eax                      sbb  edx, edx
and  eax, ecx                      and  edx, ecx     ; node in ecx; AND materialized in both
push eax                           push edx
```
Distinct from `branchless-mask-and-explicit-vs-fused-test.md` (which is about whether the AND
*materializes* vs fuses into a `test`); here the AND materializes in BOTH, only the register
differs. WALL — bank the 99.3% and move on. Evidence: DSoundList::RemoveMatching @0x136f60
(src/Dsndmgr/SoundVoiceList.cpp); the other 3 list helpers (InsertHead/InsertTail/Unlink) are 100%.
