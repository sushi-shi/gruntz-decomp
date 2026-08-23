# An escaped local's declared SCOPE decides whether its frame home can be packed
tags: cpp:local cpp:decl cpp:scope cpp:switch cpp:macro | asm:sub asm:lea | topic:codegen-idiom topic:regalloc
symptoms: sub esp,N one dword too big, frame LARGER than retail, extra dword nothing addresses, `walls framescan` residual 0, address-escaped local, out-param, `&idx`, do/while(0) macro block
confidence: 10/10

`walls framescan` says the frame is 4 (or 8) bytes bigger than retail's and the
frame-masked residual is small or zero, i.e. every used `[esp+N]` already agrees.
The excess is a whole dword slot, and the slot belongs to an ADDRESS-ESCAPED local
- one whose `&` reaches a call (`Read(&idx,4)`, `Lookup(key, out)`,
`AnyValueMatches(p, buf, &v)`). cl 5.0 gives every such local a permanent home and
packs two of them onto one slot only when their DECLARED scopes are disjoint. A
function-scope or case-scope declaration therefore cannot share a home with
anything, however short its real live range.

```cpp
// costs a slot: `idx` spans the whole arm, so it cannot take the home the first
// group's hoisted spill has already vacated
case SERIAL_LOAD: {
    i32 idx;
    ...read group 1...;              // `reg` gets hoisted into ESI here
    { ...read group 2 using idx... }
}

// retail: each group scopes its own `idx`, which then overlays the dead spill
case SERIAL_LOAD: {
    ...read group 1...;
    { i32 idx; ...read group 2 using idx... }
    { i32 idx; ...read group 3 using idx... }
}
```
```asm
   base    sub esp,0x8c
   retail  sub esp,0x88          ; every [esp+N] otherwise identical
```

Steerable, and it runs BOTH ways - read the slot ROSTER, not just the size:

- **Frame too big, packing wanted.** `CSBI_GruntMachine::SerializeFields` 0xe8e00
  99.89 -> **100.00 EXACT**: scoping `idx` to each of the three LOAD groups let it
  overlay the `g_gameReg->m_world` spill, dead from the first group on because that
  group hoists it into ESI. `CGrunt::LoadStateRecord` 0x555e0 96.43 -> 97.33 (then
  100.00 with three further levers): moving `id`/`obj` from function scope into the
  `SERIALREF` do-block let them pack with `NAMEREF`'s `value`.
- **Frame too small, packing UNwanted.** `CSBI_Image::SerializeFields` 0xe6e40
  89.73 -> 89.85: retail's SAVE and LOAD arms hold their escaped scalars at
  DIFFERENT slots (0x10 and 0x18), which one shared variable cannot produce, so
  retail declares two. Splitting them took the frame 0x88 -> retail's 0x8c and every
  slot to retail's.

**Two slots at different offsets prove two variables.** One variable is one home for
every arm that uses it; if retail's two arms address different offsets for the same
role, the source has two declarations. That is the check that separates this lever
from inventing a local to move a number.

**Not this pattern when the excess slot is only ever reg<->mem** (no `lea [esp+N]`
of it anywhere). That is a compiler spill and its slot count is a register-pressure
reading, not a declaration - `CAniAdvanceCursor::Deserialize` 0x15ca70 carries one
address-CSE too many (`&m_index` in EBP), `CGrunt::LoadEntranceConfig` 0x67f80 one
extra shifted-coordinate spill, and neither moves under a scope edit. Tree-wide
census 2026-08-23: of the 29 frame-LARGER rows, 28 pair cleanly and 10 of those have
no `lea [esp+N]` of any slot at all (the 29th, `ConvertRowDoubleFwd` 0x14d5e0, has an
embedded jump table that truncates the base-side disassembly).
