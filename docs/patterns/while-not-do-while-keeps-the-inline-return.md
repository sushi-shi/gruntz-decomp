# A seeded spin loop is a `while`, not a `do..while` — the bottom test decides whether its `return` is inlined or tail-merged
tags: cpp:loop cpp:branch cpp:return | asm:jcc asm:jmp | topic:codegen-idiom
symptoms: `jne <shared epilogue>; jmp <loop top>` where retail has `je <loop top>` followed by its own inlined `mov eax,1 / pop.. / ret`; the function is short by exactly one epilogue
confidence: 8/10

When a flag is seeded to 0 immediately above the loop, `do {...} while (f == 0);` and
`while (f == 0) {...}` are semantically identical — but they lower differently. The
do-while leaves cl a bottom test whose false edge it merges into another identical
`return` elsewhere in the function (costing a `jmp` back to the top); the `while` form
rotates so the loop-back edge is the conditional and the exit falls into its OWN
epilogue, which is retail's layout.

```cpp
m_verifyDone = 0;
while (m_verifyDone == 0) {   // NOT `do { ... } while (m_verifyDone == 0);`
    ...
}
return 1;
```
```asm
mov    eax,DWORD PTR [esi+0x540]
test   eax,eax
je     <loop top>
mov    eax,0x1                   ; its own inlined epilogue, not a jmp to a shared one
pop    edi
pop    esi
pop    ebp
pop    ebx
pop    ecx
ret    0x4
```
STEERABLE. One of the two fixes that took CMulti::Poll 0x0bba10 94.22 -> 100 EXACT
(its @early-stop called the epilogue difference "a regalloc/block-layout wall, not
steerable here"). Related: when the LAST statement of a tail is a call whose result is
the return value, spell it `return F(...);` — retail's shared epilogue then carries no
`mov eax,1` (CStatusBarMgr::ClickHilite 0x0ff850 89.07 -> 97.58).
