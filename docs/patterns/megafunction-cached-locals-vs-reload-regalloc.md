# Megafunction regalloc cascade: cached this-> locals score higher than the byte-correct reload spelling
tags: cpp:local cpp:method | asm:mov asm:sub asm:add | topic:wall topic:regalloc

In a large dispatcher (dozens of handlers over one `this`), retail pins the four
callee-saved regs to `{esi=this, edi=key, ebx=0, ebp=1}` and RELOADS the
frequently-used sub-pointers (`this->m_4`, `this->m_2dc`, …) per block. Two source
spellings are possible and they trade off oppositely on the objdiff fuzzy metric:
caching the sub-pointers as function-level locals scores HIGHER even though it
produces the WRONG allocation, because reloading (the byte-correct spelling)
inserts/deletes `mov reg,[esi+N]` instructions that misalign the diff sequence
more than a flipped register OPERAND costs.

```cpp
// HIGHEST-SCORING (but not byte-correct allocation): cache as locals.
void* host = P(self, 0x4); void* level = P(self, 0x2dc);   // pinned in ebp/spilled
// BYTE-CORRECT allocation (ebx=0,ebp=1) but LOWER score: reload inline (macro).
#define host  P(self, 0x4)
#define level P(self, 0x2dc)
```
```asm
; retail: sub esp,0x10 ; ebx=0 ; ebp=1 ; this->m_4/m_2dc reloaded per block
; cached-locals recompile: sub esp,0x8 ; ebp=host (spilled) ; every add esp,0x8 / [esp+N] shifted
```
WALL (holistic, not source-steerable to 100%): the cached-locals spelling wins the
fuzzy score; the reload spelling recovers retail's exact ebx=0/ebp=1 but scores
lower. Retail reserves `sub esp,0x10` but uses only the 2 highest slots
([esp+0x14]/[esp+0x1c], the bounds-check spills) — the frame size follows the
allocation, not the local count. CGamePlayInput::DispatchKey (0xcbcc0, 5850 B):
5.4%→78.5% (locals) vs 74.1% (both-inline) vs 75.9% (one-inline); banked locals.
