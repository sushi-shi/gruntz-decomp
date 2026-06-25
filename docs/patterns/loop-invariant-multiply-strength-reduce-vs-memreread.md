# Inner-invariant `idx = (s*c) % m` hoisted + strength-reduced; retail re-reads `c` from memory
tags: cpp:loop cpp:local | asm:imul asm:idiv asm:mov | topic:wall topic:regalloc topic:scheduling
symptoms: inner loop reads a spilled running accumulator + idiv where retail does `mov eax,reg;imul eax,edx;cdq;idiv reg` inside the loop; ebx<->ebp swapped on the modulus vs loop counter; frame 2 spill slots too big
confidence: 7/10
variants: blowfish-feistel-unroll-regalloc.md

A nested loop whose inner body uses an INNER-loop-invariant `(s*count) % prime`
(s = outer counter): MSVC5 /O2 may hoist it to the outer loop AND strength-reduce
`s*count` into a running `+= count`, spilling the accumulator. Retail instead keeps
`count` in MEMORY (re-reads `[esp+arg]` each outer iteration) which DEFEATS the
strength reduction, so it recomputes `imul`+`idiv` inside the loop — a register-
pressure outcome, not a source choice. The same pressure flips the callee-saved
assignment (modulus prime->ebp + counter s->ebx + array->edi in retail vs
prime->ebx + array->ebp in the recompile), which cascades across the whole body.

```cpp
for (i32 s = 1; s < prime - 1; s++) {        // outer counter
    for (i32 k = 0; k < prime - 1; k++) {
        i32 r = (s * count) % prime;          // inner-invariant -> cl hoists+reduces
        if (used[r - 1] != 0) { ok = 0; break; }
        used[r - 1] = 1;
    }
}
```
```asm
; retail (inside the inner loop, count stays in memory):
  mov eax,ebx            ; s
  imul eax,edx           ; * count  (edx re-loaded from [esp+0x2c] each outer iter)
  cdq ; idiv ebp         ; % prime  (ebp = prime)
; recompile (hoisted + strength-reduced, accumulator spilled):
  mov eax,[esp+0x10]     ; running s*count accumulator
  cdq ; idiv ebx         ; % prime  (ebx = prime)  <- ebx<->ebp swap
```
WALL (not source-steerable): inlining the recompute / `step=(i32)used` / for-vs-while
all regressed; logic is provably correct. ScatterSamples 0x182940 ~59.2% (@early-stop);
mirror of blowfish-feistel-unroll-regalloc (identical multiset, different reg pick).
