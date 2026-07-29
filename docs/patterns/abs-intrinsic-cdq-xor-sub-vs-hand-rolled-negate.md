# `cdq / xor eax,edx / sub eax,edx` is the `abs()` INTRINSIC — a hand-rolled negate emits a branch
tags: cpp:int cpp:branch | asm:cdq asm:neg asm:jns | topic:codegen-idiom
symptoms: retail `cdq / xor eax,edx / sub eax,edx`, ours `jns` + `neg`, extra basic blocks, skeleton drift
confidence: 10/10

Retail's branchless three-instruction absolute value is MSVC5's `/Oi` **`abs()` intrinsic** (`/O2`
implies `/Oi`). Writing it out as `if (d < 0) { d = -d; }` — or as the ternary `d < 0 ? -d : d` —
emits a `jns`/`neg` PAIR instead, which also splits the function into two extra basic blocks and
cascades into a different register/spill assignment for everything downstream.

```cpp
#include <stdlib.h>
i32 dx = abs(t->m_screenX - arg1);   // YES - cdq/xor/sub
// NO:  i32 dx = t->m_screenX - arg1; if (dx < 0) { dx = -dx; }
// NO:  i32 dx = d < 0 ? -d : d;
```
```asm
cdq                       ; retail
xor    eax,edx
sub    eax,edx
```
STEERABLE. `CGameLevel::MoveToward` 0x15de40: the two hand-rolled negates were TWO extra block
pairs — swapping both to `abs()` took the skeleton from **37 blocks to 31** against retail's 32
(and closed the whole prologue). Grep a stuck function's target disasm for `cdq` next to a `xor`
+`sub` on the same register before assuming a regalloc wall.
