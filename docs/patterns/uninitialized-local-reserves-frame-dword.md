# A `push ecx` frame dword read into a register at entry IS an UNINITIALIZED local
tags: cpp:local cpp:loop | asm:push asm:pop asm:mov asm:xor | topic:codegen-idiom
symptoms: retail has `push ecx` + a matching `pop ecx` in every epilogue, `mov <reg>,[esp+N]` off that slot before the slot was ever written, recompile has `xor <reg>,<reg>` and no slot at all, "this-spill frame wall"
confidence: 9/10

When retail reserves one stack dword with a bare `push ecx`, reads it into a register at
the top of the function, and never writes it, the source declared the corresponding local
**without an initializer**. cl5 has to materialize the enregistered variable's value on
entry to its live range, and for an uninitialized local that value is the (garbage) frame
slot. Spelling `= 0` emits `xor <reg>,<reg>` instead and the whole slot — plus every
`pop ecx` — disappears, which reads as a bogus "retail spills `this` for the loop seed"
frame wall.

```cpp
// retail:  push ecx ... mov edi,[esp+0x10] ... pop ecx (x4 epilogues)
CTileTriggerLogic* child;          // NO initializer
POSITION pos = m_owner->m_idleLogics.GetHeadPosition();
while (pos != 0) { ... child = ...; ... }
```
```asm
    push   ecx                      ; <- the local's slot, never stored to
    push   ebx
    push   ebp
    mov    ebp,ecx
    push   esi
    push   edi
    mov    edi,DWORD PTR [esp+0x10] ; <- seeds the cursor from the untouched slot
    ...
    pop    edi
    pop    esi
    pop    ebp
    pop    ebx
    pop    ecx                      ; <- one per epilogue
    ret
```

The frame-size cross-check is decisive: `gruntz walls diagnose <rva>` (or just the
`sub esp,N`/`push` count) shows retail's frame is exactly 4 bytes larger with no local to
account for it.

Evidence: `CTileTriggerSwitchLogic::VerifyBlockLinks` @0x112c70 and `VerifyBlockLinksB`
@0x111f40, both filed as a "this-spill frame wall (~86%, dead seed value, non-steerable
frame choice)" — 86.60 → 95.29 on dropping the `= 0` alone, then 100.00 EXACT with the
block-walk subscript fix (see
[array-cursor-bias-from-row-pointer-local.md](array-cursor-bias-from-row-pointer-local.md)).
