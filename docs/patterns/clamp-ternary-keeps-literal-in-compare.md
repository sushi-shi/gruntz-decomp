# `init-then-if` clamp CSEs the bound into the holding register (`cmp edx,esi`); the TERNARY keeps the literal in the compare
tags: cpp:ternary cpp:branch cpp:local | asm:cmp asm:mov | topic:codegen-idiom topic:regalloc
symptoms: `mov esi,0x3 / cmp edx,esi` where retail has `mov edi,0x3 / cmp edx,0x3`; a 1-byte size deficit; the two callee-saved roles (`this` vs the clamped value) are swapped versus retail
confidence: 8/10

`i32 v = K; if (x >= K) v = x;` gives cl5 one live range seeded with `K`, so it CSEs the
second `K` into that register and compares register-to-register. Spelling both arms in
ONE conditional expression leaves `K` a literal on each side: the compare keeps the
immediate, and the resulting live-range order also restores retail's `this`-first
callee-saved assignment.

```cpp
// NO   -> mov esi,3 / cmp edx,esi / jb / mov esi,edx   (this in edi)
i32 base = 3;
if (tuned >= 3) { base = static_cast<i32>(tuned); }

// YES  -> mov edi,3 / cmp edx,3   / jb / mov edi,edx   (this in esi)
i32 base = (tuned < 3) ? 3 : static_cast<i32>(tuned);
```
```asm
mov    edi,0x3
cmp    edx,0x3
jb     <skip>
mov    edi,edx
```
STEERABLE. CMulti::AutoTuneCmdDelay 0x0bcc10 97.87 -> 100 EXACT (its @early-stop read
"a pure esi<->edi swap plus cl CSEing the literal 3 into that register; permute fn
200 iters found nothing"). The if/else form with both arms assigning works too; only
the init-then-if form CSEs.

SCOPE: this is the NARROW case where the default and the compare's RHS are the SAME
literal. When they differ - and especially when the override is a memory load - the
init-then-if form is the one retail wrote, because it hoists the default into the
destination and leaves the low arm a bare fall-through with no `jmp`:
[default-hoists-into-destination-no-jmp.md](default-hoists-into-destination-no-jmp.md).
