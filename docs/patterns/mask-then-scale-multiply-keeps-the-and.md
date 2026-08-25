# `and 0xffff` surviving before `shl 16` means `* 0x10000`, not `<< 16`

tags: cpp:int cpp:const | asm:and asm:shl asm:or | topic:codegen-idiom
symptoms: retail packs two halfwords with BOTH masks visible (`and reg,0xffff` on the
high half AND the low half, then `shl 16` / `or`), ours emits only the low-half mask;
statement splitting (`data <<= 16;` on its own line) does not bring the AND back
confidence: 8/10
variants: div-mul-lower-too-late-to-fold-with-a-mask.md

cl5's C1 tree fold deletes a mask the following shift makes redundant:
`(x & 0xffff) << 16` emits a bare `shl 16`. The fold is tree-level and survives any
statement decomposition — `i32 hi = x & 0xffff; hi <<= 16;` and the compound
`data = x & 0xffff; data <<= 16; data |= ...` were both measured identical (the AND
still folds).

Spelling the scale as a multiply defeats it, exactly as in the `/ 8 * 0x800` variant:
cl5 strength-reduces a power-of-two `*` to `shl` only AFTER the redundant-mask fold has
run, so both masks survive:

```cpp
// YES - and/and/shl/or, retail's multiset (CLatencyList::FillCombo 0x37ff0)
i32 data = ((rec->m_resendInterval & 0xffff) * 0x10000) | (rec->m_commandDelay & 0xffff);
// NO  - one and + shl: the high half's mask folds into the shift
i32 data = ((rec->m_resendInterval & 0xffff) << 16) | (rec->m_commandDelay & 0xffff);
```

```asm
mov    edx,[ecx+0x8]     ; high half loaded FIRST (matches the * spelling's eval order)
mov    esi,[ecx+0x4]
and    edx,0xffff        ; SURVIVES only under * 0x10000
and    esi,0xffff
shl    edx,0x10
or     esi,edx
```

Controlled A/B in `slotcombofill` (72.01 -> 73.22, the whole delta is this site):
the multiply spelling also moves the two field loads into retail's order (high first).
OR-operand order is inert — swapping the `|` operands was measured byte-identical,
consistent with the `+`-operand findings at the grid-index sites.
