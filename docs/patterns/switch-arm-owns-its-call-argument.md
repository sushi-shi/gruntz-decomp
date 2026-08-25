# A per-arm `push <const>` means the CALL is inside the arm, not after the merge

tags: cpp:switch cpp:branch cpp:call cpp:local | asm:push asm:jmp asm:mov | topic:codegen-idiom
symptoms: every switch arm ends `push <imm>` / `jmp <shared>` and the shared block
is only `mov ecx,<receiver>` + `call`; ours has `mov eax,<imm>` per arm and ONE
`push eax` at the join, so `walls residue` reports `mov r,0x0` x N against
`push 0x0` x N and `walls diagnose` calls it REGALLOC
confidence: 8/10
variants: arm-result-temp-controls-copies-and-shared-store.md

Sibling of the arm-result-temp rule, for the case where the value an arm produces
is a CALL ARGUMENT. Writing the call in each arm lets cl 5.0 cross-jump only the
common `mov ecx,<this>` + `call` tail; the argument push stays inside its arm.
Binding a local instead gives the arm a pseudo, and the single push lands at the
merge.

```cpp
switch (owner) {                        // retail: the arm owns the call
    case WARLORDZ_KING:   SetImageSetByName("GAME_FORTRESSFLAGZ_KING");   break;
    case WARLORDZ_VIKING: SetImageSetByName("GAME_FORTRESSFLAGZ_VIKING"); break;
    default: SetObjectFlags(0x10000); return;
}
```
```asm
        push 0x0            ; <- the arm's own push (reloc-masked string)
        jmp  L_call
        push 0x0
L_call: mov  ecx,DWORD PTR [esi+0x38]
        call <SetImageSetByName>
```
STEERABLE. `CFortressFlag::CFortressFlag` 92.82 -> 97.00 and the residue class
moved `selection` -> `regname` by moving `SetImageSetByName(name)` into the four arms.
