# Branchless `(cond ? -1 : 0) & value` guard: retail materializes the AND (`and; test eax,eax`), our cl fuses to `test reg,reg`
tags: cpp:branch cpp:local | asm:neg asm:sbb asm:and asm:test | topic:wall topic:regalloc topic:codegen-idiom

symptoms: a `Lookup(k,v) && v` style duplicate/exists guard lowers to the branchless mask form `neg eax; sbb eax,eax` (BOOL -> all-ones mask), but then the target does `and eax,edx; test eax,eax` (value in edx, explicit AND materialized, NO spill) while the recompile does a fused `test ecx,eax` (value in ecx, AND folded into the test, no separate insn) — or, if you name the masked result, `and eax,ecx; mov [esp+N],eax` (explicit AND but a gratuitous stack spill of the dead local). 1-region residual on an otherwise byte-exact function, ~97.8–98.6%.
confidence: 7/10

The `&&` of a BOOL call-result and a pointer, evaluated as a value (not two branches), lowers
branchlessly: `mask = (Lookup() != 0) ? -1 : 0` via `neg eax; sbb eax,eax`, then `mask & value`.
The mask half is steerable — spell `(Lookup(k, v) ? -1 : 0) & (i32)v` (NOT `&&`/`& (i32)v` alone,
which give two `test/je` branches or test only the low pointer bit). What is NOT steerable is
whether our cl *materializes* the AND result in a register and `test`s it, vs *fuses* the AND into
a `test reg,reg`. Retail materializes (`and eax,edx; test eax,eax`, value in edx, no spill); an
inline expression makes our cl fuse (`test ecx,eax`, 97.76%); naming the masked result in an `i32
found` local forces the explicit `and` but adds a dead-local spill `mov [esp+N],eax` (98.57%, the
higher score but still 1 region off). Same compiler (MSVC 5.0), so a spelling presumably exists,
but none of `i32 found = …`, `i32 mask = …; if(mask & v)`, `(… ? v : 0) != 0`, or operand-swap
reproduced the no-spill explicit form. WALL — bank the named-local 98.57% and move on.

```cpp
void* v = 0;
i32 found = (m_map.Lookup(code, v) ? -1 : 0) & (i32)v; // explicit AND (best: 98.57%)
if (found != 0) return FALSE;
// inline `if ((m_map.Lookup(code, v) ? -1 : 0) & (i32)v)` folds to a fused test (97.76%)
```
```asm
; retail (what you SEE) - value in edx, AND materialized, no spill:
neg  eax
sbb  eax, eax
and  eax, edx          ; mask & value -> eax
test eax, eax
; our cl, inline form  - value in ecx, AND fused into the test:
neg  eax
sbb  eax, eax
test ecx, eax          ; (mask & value) for flags only, no separate AND
; our cl, named-local  - explicit AND but dead-local spill:
and  eax, ecx
mov  [esp+N], eax      ; gratuitous store of the dead `found`
```
Evidence: CCheatMgr::AddCheat @0x22be0 (src/Gruntz/CheatMgr.cpp) — inline 97.76%, named-local
98.57%; the other three CCheatMgr methods are 100%.
