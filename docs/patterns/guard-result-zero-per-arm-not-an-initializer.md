# A guard result retail zeroes PER ARM is an if/else-if chain, not an initializer + one `&&`

tags: cpp:branch cpp:if cpp:local cpp:call | asm:xor asm:jmp asm:mov | topic:codegen-idiom topic:regalloc
symptoms: retail materializes `xor eax,eax` once per failing guard and jumps each
arm to a shared `test eax,eax`, keeping the value in the call's own return
register; ours hoists ONE zero into a non-EAX register, needs a `mov <that>,eax`
after the call, and merges the failing arms - `walls diagnose` reports CFG with
retail holding one MORE `jmp` and one FEWER `mov`
confidence: 9/10

## The pair

```asm
; retail                                  ; ours
test ecx,ecx        ; the first guard     xor  ecx,ecx        ; ok = 0 (hoisted)
je   L_second                             test eax,eax
xor  eax,eax        ; ok = 0              jne  L_join         ; both failing arms
jmp  L_join                               ...                 ; share ONE target
L_second:                                 cmp  [esi],0
cmp  edx,4                                je   L_join
jne  L_body                               L_body:
cmp  DWORD PTR [esi],0                    ...
jne  L_body                               call [esi]
xor  eax,eax        ; ok = 0 again        mov  ecx,eax        ; <- the extra mov
jmp  L_join                               jmp  L_join
L_body: ... call [esi]                    L_join:
L_join: test eax,eax                      test ecx,ecx
```

`i32 ok = 0;` followed by ONE combined `if (A && !(B && C)) { ...; ok = Call(); }`
gives cl a single zero definition. Because that definition dominates the call, it
cannot live in EAX, so the call result has to be copied into it and both failing
paths collapse onto one join edge - which also drags unrelated loads (a later
argument read) up into the merged arm.

Assigning `0` inside each failing arm restores retail's shape: the zero is
re-materialized per arm, `ok` and the call result are the SAME value in EAX, and
each arm gets its own `jmp` to the shared test.

```cpp
i32 ok;
if (m_initGate != 0) {
    ok = 0;
} else if (typeId == TRIGID_EXCLUSIVE_SWITCH_4 && rect[0].left == 0) {
    ok = 0;
} else {
    memcpy(m_block, rect, sizeof(m_block));
    ok = Setup(owner, typeId, tileX, tileY, cellKey, linkGate, damageParam, checkpointType);
}
if (ok == 0) {
    return 0;
}
```

STEERABLE. `CCheckpointTriggerSwitchLogic::BuildSmall` 0x00112a50 84.26 ->
**100.00 EXACT**, byte-identical, on this change alone.

Distinct from [ladder-default-is-the-initializer.md](ladder-default-is-the-initializer.md):
that rule is about a ladder of DIFFERENT CONSTANTS whose default must be the
initializer. Here every failing arm yields the SAME zero and the successful arm
yields a CALL RESULT - the register identity is what decides, so count retail's
`xor <acc>,<acc>` sites before choosing the spelling.
