# A loop-invariant member load retail keeps in a slot ACROSS a call was hoisted in SOURCE
tags: cpp:loop cpp:local cpp:member | asm:mov asm:sub | topic:codegen-idiom topic:regalloc
symptoms: retail's `sub esp,N` is exactly 4 (or 8) bytes LARGER than ours; `--branches --diff`
reports "base K branch(es) | target K+1"; the masked `--diff` shows a lone extra `mov reg,[this+X]`
+ `mov [esp+M],reg` pair sitting between the loop's pre-header and its first test, and every later
`[esp+..]` displacement on the target side is shifted by that 4
confidence: 9/10
variants: loop-bound-local-vs-inline-invariant.md

cl5 hoists a loop-invariant member read for free — *unless the loop body contains a call*, which
kills every memory value. So when retail evaluates `m_a->m_b` once and parks it in a stack slot for
the whole loop, while we re-read it each iteration, the hoist is not an optimisation we failed to
trigger: it is a **named local in the original source**. The extra stack slot is the tell.

```cpp
// WRONG - the virtual GetCollisionAt() inside PROBE_TILE kills the load, so cl
// cannot hoist it and re-reads m_mainPlane->m_wrapH every iteration:
while (result != TILEKIND_SOFT) {
    ...
    if (row >= m_mainPlane->m_wrapH) { return 0; }
    PROBE_TILE(this, px, row, result);
}

// RIGHT - the dev hoisted it, which is the only way the value survives the call:
i32 wrapH = m_mainPlane->m_wrapH;
while (result != TILEKIND_SOFT) {
    ...
    if (row >= wrapH) { return 0; }
    PROBE_TILE(this, px, row, result);
}
```
```asm
    xor  eax,eax
    mov  edx,DWORD PTR [ebp+0x5c]     ; m_mainPlane          <- hoisted pair,
    cmp  eax,0x1                      ;                         scheduled INTO
    mov  DWORD PTR [esp+0x24],edi     ; startRow                the loop test
    mov  ecx,DWORD PTR [edx+0x34]     ; m_wrapH
    mov  DWORD PTR [esp+0x1c],ecx     ; <- lives across the whole loop
    je   <loop exit>
```
Steerable. Corollary from the same fix: a value the loop reads but never updates (`startRow`) is
captured where retail *spills* it — declaring it after the pre-header statement it follows in the
target moved the last instruction into place. `CGameLevel::WalkColumnDown` @0x160a40
89.39 -> 98.63 with the hoist, -> **100.00 EXACT** once `startRow` was also declared after the
first probe (`@early-stop` deleted).
