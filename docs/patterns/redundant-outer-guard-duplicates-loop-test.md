# An outer `if (n > 0)` around a `for (i=0;i<n;i++)` emits the SAME test twice
tags: cpp:loop cpp:branch cpp:local | asm:cmp asm:test asm:jcc | topic:codegen-idiom
symptoms: base has two adjacent `cmp <n>,<zeroreg> / jle` (or `cmp/jle` + `test/jle`) where retail has ONE `test <n>,<n> / jle`, plus an extra register-to-register write-back of the counter, filed as a "regalloc-coloring wall / ebx<->edi swap"
confidence: 9/10

cl5 does **not** fold a hand-written `if (n > 0)` into the `for`'s own entry guard: it
emits both, and it compares against a zero it happens to have in a register (`cmp edx,eax`)
instead of retail's `test edx,edx`. The extra block also changes which callee-saved
register the loop counter gets and forces a write-back if the counter is a separate
temp, so the residue looks like a whole-function register permutation rather than one
redundant test.

```cpp
// NO - two guards, and `cnt` costs a write-back:
i32 live = 0;
i32 n = m_items.GetSize();
if (n > 0) {
    i32 cnt = 0;
    for (i32 i = 0; i < n; i++) { if (…) cnt++; }
    live = cnt;
}

// YES - one guard, counter is the destination:
i32 live = 0;
i32 n = m_items.GetSize();
for (i32 i = 0; i < n; i++) { if (…) live++; }
```
```asm
    mov    edx,DWORD PTR [edi+0x18]
    mov    DWORD PTR [esp+0x14],ebx   ; live = 0 (from the pinned zero)
    test   edx,edx                    ; ONE guard
    jle    <after>
```

Evidence: `CDDrawWorker::ValidateFramesFromArchive` @0x1522b0, filed "regalloc-coloring
wall … permute (start 87.755%) found no better spelling" — 87.81 → **100.00 EXACT**; both
callee-saved colours fell out on their own.
