# A `char`/`short` argument read straight from a member takes a NARROW load — bind it to an `int` local first

tags: cpp:param cpp:call cpp:local | asm:mov asm:push | topic:codegen-idiom topic:regalloc
symptoms: base has `mov al,BYTE PTR [reg+N]` / `mov dx,WORD PTR [reg+N]` where retail
has `mov eax,DWORD PTR [reg+N]` at the SAME argument slot; the callee's parameter is
`char` or `short`; a `66` operand-size prefix appears in the base and not the target
confidence: 9/10

## Symptom

The callee's parameter really is one or two bytes wide (its mangled name carries `D`
or `F`), the argument expression is a plain `i32` member, and the recompile emits a
byte/word load:

```asm
base  : mov dl, WORD PTR [edi+0x1f0] | mov cl, BYTE PTR [eax+0x1ec] | push edx | push ecx
retail: mov esi, DWORD PTR [esi+0x1f0] | mov eax, DWORD PTR [eax+0x1ec] | push esi | push eax
```

Both are correct: only the low bits of the pushed dword are read by the callee. cl5
picks the narrow load when the value's ONLY use is the narrowing conversion, and the
full dword load when the value is a plain `int` rvalue that is merely *passed* to a
narrow parameter.

## Fix (steerable)

Bind the member to an `i32` local and pass the local. Explicit `static_cast<char>(..)`
/ `static_cast<i16>(..)` does NOT help — it is the same narrowing conversion cl was
already doing, so it keeps the narrow load.

```cpp
// NO - narrow loads, and the value is not available as an int for any other use
EnqueueSingle(1, cell->m_playerIndex, cell->m_unitIndex, 10,
              hit->m_playerIndex, hit->m_unitIndex, 0, 0);

// YES - one dword load per value, exactly retail's encoding
i32 hitPlayerIndex  = hit->m_playerIndex;
i32 hitUnitIndex    = hit->m_unitIndex;
i32 cellUnitIndex   = cell->m_unitIndex;      // declaration order = load order
i32 cellPlayerIndex = cell->m_playerIndex;
EnqueueSingle(
    1,
    cellPlayerIndex,
    cellUnitIndex,
    10,
    hitPlayerIndex,
    hitUnitIndex,
    0,
    0
);
```

The locals' DECLARATION order is the load order, so if the diff has the two loads
transposed, transpose the declarations.

## Knock-on: it also stops a cross-jump

The int locals raise the pressure enough that cl stops hoisting an unrelated global
load out of the two arms that feed the call — which is what had merged the two call
sites into one shared tail. `CTriggerMgr::HandleTargetSelection` @0x079520: **84.15 -> 98.14**
from this one change (77.14 before the arm-order work in the same commit; 99.85
after the compare operands were transposed to match).

related: byte-param-caller-ships-only-dl.md (the mirror case: retail ships only `dl`,
so the PARAMETER is narrow and the recompile's widening is the defect)
