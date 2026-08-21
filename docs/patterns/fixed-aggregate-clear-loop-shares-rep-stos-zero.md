# A fixed aggregate clear loop shares the post-call zero with later REP STOS arrays
tags: cpp:array cpp:loop cpp:member | asm:xor asm:rep asm:stos asm:call | topic:codegen-idiom topic:regalloc
symptoms: a short reset function has identical size, calls, stores, constants and referents, but the base carries zero in a callee-saved register across calls while retail rematerializes zero in EAX after them; four adjacent scalar stores precede several REP STOS array clears
confidence: 10/10

When a reset method clears a small fixed array and then several larger arrays,
write the small aggregate as a loop too. VC5 can unroll the small loop while
coalescing its zero value with the following `rep stos` expansions. Four
handwritten element stores are semantically equivalent, but create a different
value lifetime.

`CBattlezData::Init` is the controlled pair. The source originally reconstructed
`m_counts[4]` as four explicit assignments, followed by loops over four pickup
arrays. The base kept the zero in EDI across `ClearWins` and `ClearFlags`, used
EDI for the four count stores, then initialized EAX separately for the pickup
array clears:

```asm
push esi
push edi
mov  esi,ecx
xor  edi,edi
; scalar stores through edi
call ClearWins
call ClearFlags
mov  [esi+48h],edi
; four count stores through edi
lea  edi,[esi+0d8h]
xor  eax,eax
rep  stosd
```

The retail body saves EDI only when the post-call clears need their destination
cursor. It rematerializes zero in EAX after the calls, unrolls the four-element
count loop, and carries that same EAX into every later `rep stosd`:

```cpp
i32 i;
for (i = 0; i < BZ_PLAYER_COUNT; i++) {
    m_counts[i] = 0;
}
for (i = 0; i < 88; i++) {
    m_weaponPickupz[i] = 0;
}
```

That one aggregate correction takes `CBattlezData::Init` at `0x0fca10` from
81.0909% to 100% exact: `0x8a` bytes, 44 instructions, 2 calls, 0 branches,
1 return, 21 stores, and 2 ordered relocations on both sides.

Detection signature: the semantic census is already exact; the first byte
difference is the prologue/register order; a pre-call callee-saved zero is used
for a small array's handwritten stores, while retail has a post-call `xor
eax,eax` that feeds both those stores and immediately following `rep stos`
clears. Confirm the member is a real fixed array and use its proven bound. Do
not combine unrelated adjacent fields into an invented aggregate merely to
obtain this schedule.
