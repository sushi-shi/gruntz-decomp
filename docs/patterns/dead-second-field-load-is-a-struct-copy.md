# A second field loaded and never read is a STRUCT COPY, not two scalar reads

**Tags:** `cpp:local` `cpp:struct` `cpp:member` | `asm:mov` | `topic:codegen-idiom`
`topic:regalloc`
**Confidence:** 9/10 (one clean A/B, corroborated by the frame size)

## Symptom

Retail's frame is exactly 4 (or 8) bytes bigger than ours, and at the divergence
retail loads BOTH halves of a two-field struct member and spills one of them to a
slot **nothing ever reads back**:

```asm
; retail 0x30730
cmp  DWORD PTR [eax+0x2d8],0x4     ; if (src->m_battleState == BZTASK_ADVANCE)
jne  skip
mov  ecx,DWORD PTR [eax+0x2f0]     ; m_arrivalCell.m_x
mov  eax,DWORD PTR [eax+0x2f4]     ; m_arrivalCell.m_y   <- never used again
cmp  ecx,edx
mov  DWORD PTR [esp+0x18],eax      ; ... and spilled anyway
je   skip
```

```asm
; ours
cmp  DWORD PTR [eax+0x2d8],0x4
jne  skip
cmp  DWORD PTR [eax+0x2f0],ecx     ; one field, no load, no slot
je   skip
```

## Mechanism

Two scalar reads whose second result is dead are dead-code-eliminated. A **struct
copy** is not: cl 5.0 lowers `Coord c = obj->member;` to two loads and two stores
of a local it then keeps alive to the end of the statement's live range, even
when only one field is subsequently read. The dead half is what the extra frame
slot is for, so the frame-size mismatch and the missing load are ONE finding.

## Steer

```cpp
// NO - the m_y read is dead and cl deletes it
i32 sx = src->m_arrivalCell.m_x;
i32 sy = src->m_arrivalCell.m_y;
if (sx != m_playerIndex) { return 0; }

// YES
Coord sc = src->m_arrivalCell;
if (sc.m_x != m_playerIndex) { return 0; }
```

## Evidence

`CBattlezMapConfig::ClaimCellFromRow` 0x30730 **89.35 -> 94.44**, frame `sub
esp,0x8` -> `sub esp,0xc` matching retail, and the whole tail of the function
re-aligns behind the one recovered slot.

## How to find them

`clang -Wunused-variable` over the unit's clangd compile command lists exactly
the locals cl will delete; cross-check each against a frame-size mismatch in the
mirror direction (retail reserves MORE). The frame sieve is one line over the
`--lite` dumps: read the `sub esp,N` of both sides per function and list the rows
that differ.

## Not this pattern

A dead local that retail ALSO spills and never reads (retail's cl kept the store
too) — then both sides agree once the statement exists at all, and there is
nothing to steer. `CBattlezMapConfig::StepRowUnits`' `stX` load before the
battle-state switch is that case: retail emits `mov edx,[esi+0x170]` +
`mov [esp+0x10],edx` and then recycles the slot for an unrelated Coord.
