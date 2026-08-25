# The inline `strcmp` loop is two instructions longer when a byte register is free
tags: cpp:string cpp:loop | asm:cmp asm:mov | topic:wall topic:regalloc
symptoms: `walls loopscan` reports a 13-vs-15 body whose head is `mov dl,BYTE PTR [eax]`; `retail+movx2` or `ours+movx2` with nothing else; `sbb eax,eax` at the exit
confidence: 9/10

cl 5.0 expands `strcmp` inline as a two-bytes-per-iteration loop, and picks
between comparing against MEMORY and loading into a spare byte register purely
on whether one is free. Both forms are the same intrinsic; the two-`mov`
difference is downstream register pressure, not a source spelling.

```asm
; free byte register (BL): 15 instructions
mov dl,BYTE PTR [eax]
mov bl,BYTE PTR [edi]
mov cl,dl
cmp dl,bl
; none free: 13 instructions, memory operand
mov dl,BYTE PTR [eax]
mov cl,dl
cmp dl,BYTE PTR [edi]
```
Wall. It recurs across at least five rows of the loopscan census in both
directions - `CBattlezMapConfig::StepRowUnits` 0x267c0 and
`CTriggerMgr::UseToyAt` 0x6e120 read `retail+movx2`, while
`CBattlezMapConfig::StepDefenderUnit` 0x33520 (four loops),
`CGrunt::StepArrivalDrop` 0x4b370 and `CTriggerMgr::UseEquippedToolAt` 0x6dae0
read `ours+movx2` - so the sign carries no information about the source.
Recognise the head `mov dl,BYTE PTR [eax]` with an `sbb eax,eax` exit and
spend the row's budget on its other loops.
