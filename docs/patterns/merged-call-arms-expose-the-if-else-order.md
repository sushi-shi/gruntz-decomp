# A cross-jump-merged call puts both if/else arms' constants in one push window
tags: cpp:branch cpp:call | asm:push asm:jmp | topic:codegen-idiom
symptoms: two `push <imm>` for a callee that takes one such parameter; `push 0x16 / jmp / push 0x12 / push this / call`; the same call appears once but the sieve sees N+1 arguments
confidence: 9/10

When both arms of an `if`/`else` end in the same call with the same arguments
bar one constant, cl 5.0 emits the constant push per arm and SHARES the call.
Linear decoding then shows both constants in one window, in arm order - so the
order of the two immediates reads the source condition's POLARITY directly,
with no need to trace the branch.

```cpp
// retail: the then-arm's constant is pushed FIRST
if (rand() % 5) { EnterDefenderMode(unit, 0x12); }
else            { EnterDefenderMode(unit, 0x16); }
```
```asm
call _rand
cdq
mov  ecx,0x5
idiv ecx
test edx,edx
je   0x5d2          ; -> the else arm
push 0x12           ; then-arm constant, emitted first
jmp  0x5d4
push 0x16           ; else arm
push ebp
mov  ecx,ebx
call ?EnterDefenderMode@...
```
Steerable: match the arm order and the branch polarity follows. Found by the
scratchpad pushed-literal sieve, which reports a merged pair as two "differing"
argument slots that are each other's transpose - the transposition IS the
signature. `CBattlezMapConfig::ValidateUnitPath` 0x29b40 88.01 -> 88.11 (0x12 /
0x16) and `CGrunt::UpdateArrival` 0x62110 95.48 -> 95.62 (0xa / 0xb, where
`if (sel != 0)` had to become `if (sel == 0)`). Not a behavioural defect on its
own - both spellings compute the same thing - but a transposed pair on a call
that is NOT merged would be, so adjudicate the merge first.
