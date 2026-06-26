# Lone `cond ? c1 : c2` bool call-arg: write if/else with literal args to force the branch-push (not `sete`)
tags: cpp:branch cpp:local | asm:sete asm:push asm:jmp | topic:codegen-idiom
symptoms: xor eax,eax; test reg,reg; sete al; push eax where retail has test; je; push imm; jmp; push imm; the callee arg is a 0/1 derived from one condition; small thiscall ~60% with body otherwise exact

A function argument computed as `f(cond ? 1 : 0)` / `f(cond == 0)` / `f(!cond)` — a lone
boolean with NO coincident constant operand (cf. `boolarg-coincident-const-ifelse-literal`).
The inline / ternary spelling makes cl materialize the bool with `xor eax,eax; test;
sete al; push eax` (and hoists the other operand loads up before the test). Retail instead
branches and pushes the two literals directly:

```asm
; retail:
  mov  eax,[esi+0x500]
  test eax,eax
  je   .true
  push 0          ; false
  jmp  .after
.true:
  push 1          ; true
.after:
  mov  ecx,[esi+0x2dc]   ; arg evaluated, THEN the receiver loaded
  call <Method>
```

Force it by writing an explicit if/else with **literal** args in each branch — cl
tail-merges the two identical call sites into one call with the branched push, and the
receiver load naturally lands after the branch (fixing register/order divergence too):

```cpp
// NOT: m_guts->Method(m_paused ? 0 : 1);   // -> xor;test;sete al;push eax (~60%)
if (m_paused) {
    m_guts->Method(0);
} else {
    m_guts->Method(1);
}
```
Steerable. Closed CPlay::PauseGame (0x0cee90) 61% → 100%; an intermediate `int` local
(`if(cond) x=0; else x=1; Method(x);`) still folds to `sete` — the literal call args are
load-bearing.
