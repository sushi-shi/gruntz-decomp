# A `flags &= ~BIT` on a dword member narrows to a BYTE memory-RMW - the byte-array spelling blocks it
tags: cpp:local cpp:cast | asm:and asm:or asm:mov | topic:codegen-idiom
symptoms: retail has `and byte ptr [base+idx+3], imm8` / `or byte ptr [base+idx+3], imm8` where the base compiles `mov r8,[addr+3] / and r8,imm8 / mov [addr+3],r8` through a register, and retail RE-LOADS the row/array pointer between the flag op and the next store to the same cell
confidence: 9/10

cl 5.0 compiles `cell.m_flags &= 0xdfffffff;` (value unused) as a single
memory-RMW `and dword ptr [mem], imm32` and then NARROWS it to the byte that
the mask actually touches: `and byte ptr [mem+3], 0xdf`. Reconstructing the
same bytes as `cell.m_flagBytes[3] &= 0xdf;` (a union byte-array poke) is a
different C1 shape: the byte lvalue makes cl compute the cell ADDRESS into a
register at the first use, RMW through a register, and then REUSE that
address for the neighbouring `m_occupantId` store.

The narrowed RMW form has a second, diagnostic consequence: because the AND
is itself a STORE, the next statement's `board->m_rows[y][x]` address is
re-derived - retail re-loads `m_rows` (the store may alias it) while keeping
`board` (a local) cached. The register-RMW form computes one address before
any store and shows no re-load.

```cpp
// blocks the narrowing (register RMW + shared lea):
board->m_rows[y][x].m_flagBytes[3] &= 0xdf;
board->m_rows[y][x].m_occupantId = -1;

// reproduces retail (memory RMW narrowed to the byte + m_rows re-load):
board->m_rows[y][x].m_flags &= BRICKZ_CELL_UNOCCUPIED_MASK;
board->m_rows[y][x].m_occupantId = -1;
```

The masks were already modeled (`BrickzCellMask` in Brickz.h); the byte-poke
spelling was a transcription of the narrowed OUTPUT, not the source.

## Measured

- CGrunt::IsDropReady 0x51510 97.98 -> 98.76, both occupancy blocks
  instruction-exact (residue elsewhere in the fn).
- CGrunt::ClaimSwitchTile 0x52c70: retail bytes confirmed the same
  `and byte [ecx+eax+3],-0x21` / `or byte [ebp+eax+3],0x20` forms.
- Sweep candidates: the same `m_flagBytes[3] &= 0xdf` spelling survives in
  Grunt.cpp, GruntCombat.cpp, GruntEntranceMove.cpp, TriggerMgr.cpp.
