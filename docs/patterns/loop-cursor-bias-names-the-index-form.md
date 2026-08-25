# A biased loop cursor (`[esi-0x8]`) names an INDEXED loop, not a `p++` cursor
tags: cpp:loop | cpp:array | asm:lea | topic:codegen-idiom
symptoms: lea esi,[edi+0x2c8] | mov eax,[esi-0x8] | negative member displacement | every displacement shifted by a constant | mov eax,[eax-0x24]
confidence: 9/10

When retail walks an array of structs and reads members at **negative**
displacements off the cursor, the cursor is one cl SYNTHESIZED — which happens
only when the source **indexes** (`arr[i].f`) rather than carrying its own
pointer. A source-level `T* p = arr; ... p++;` pins the cursor at offset 0, so
every member reads at a non-negative displacement and the whole loop body is one
byte off retail at every memory operand.

The bias cl picks is roughly the MIDPOINT of the accessed member-offset range,
snapped to a real field: `{0,4,8,0xc,0x10,0x14}` -> +8, `{0,4}` -> +4,
`{0,0x24,0x28}` -> +0x24. Do not derive it - just drop the source pointer.

```cpp
// NO - the loop-carried pointer pins the cursor at +0:
CSbiHlRow* ph = m_groupSlots;
i32 count = 3;
do { ... ph->m_state ... ph->m_last ... ph++; } while (--count);

// YES - index, and let cl synthesize the cursor:
for (i32 i = 0; i < 3; i++) { ... m_groupSlots[i].m_state ... }

// ALSO YES, when the body wants a name - derive it from the index EACH iteration:
for (i32 i = 0; i < 4; i++) { GruntzPlayer* p = &g_gameReg->m_players[i]; ... }
```
```asm
    lea    esi,[edi+0x2c8]        ; &m_groupSlots[0] + 8, not +0
    mov    eax,DWORD PTR [esi-0x8]   ; m_state
    mov    ecx,DWORD PTR [esi-0x4]   ; m_counter
    mov    edx,DWORD PTR [esi]       ; m_last  (the pair cl centred on)
    add    esi,0x18
```
Steerable. Which of the two `YES` forms wins is per-function - try both.
`CMultiBootyState::QueryGruntSlots` 73.69 -> **100.00 EXACT** with the
per-iteration pointer (and the return then reads `[eax-0x24]` off the cursor
instead of recomputing `base + i*0x238`);
`CStatusBarMgr::UpdateRezConveyorStatusBar` 93.71 -> 96.26 with the plain
indexed form and only 87.13 with the per-iteration pointer;
`CStatusBarMgr::LoadRezMachineConfig`'s 3-slot init loop matched its `+4` bias
the same way (88.40 -> 98.97 with the other fixes in that function).
