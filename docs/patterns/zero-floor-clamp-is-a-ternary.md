# `mov r,0 / sets rl / dec r / and x,r` is a ternary clamp, not an `if`
tags: cpp:ternary cpp:branch cpp:expr | asm:sets asm:dec asm:and asm:jns | topic:codegen-idiom
symptoms: sets dl | sets cl | dec edx then and | jns then xor | no jns anywhere in the target

A "floor at zero" clamp lowers two different ways in cl 5.0 /O2, and the source
construct picks which. The statement `if` keeps a branch; the conditional
expression is if-converted into a sign-mask.

```cpp
i32 nh = a - b;
nh = (nh < 0) ? 0 : nh;      // branchless: sets / dec / and
// if (nh < 0) { nh = 0; }   // branchy:    jns / xor
```
```asm
  sub    ecx,edx
  mov    edx,0x0
  sets   dl                  ; dl = (ecx < 0)
  dec    edx                 ; edx = (ecx < 0) ? 0 : -1
  and    ecx,edx             ; ecx = max(ecx, 0)
```

The whole-function screen is decisive and costs one grep: count `jns` and
`sets`/`setns` in the target. Retail's `CGrunt::LoadGruntCombatAnimations`
0x597a0 has **two** `sets` and **zero** `jns`, and the source had exactly two
such clamps. Do not generalise to every clamp in the function - the neighbouring
`if (h >= HEALTH_FULL) { h = HEALTH_FULL; }` ceilings really are branchy
(`jl` + `mov eax,0x64`).

Steerable. `CGrunt::LoadGruntCombatAnimations` 45.93 -> 47.98, and the two arms
straightened twenty basic blocks that had been reading as size drift.
