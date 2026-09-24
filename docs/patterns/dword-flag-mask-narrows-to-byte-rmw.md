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
- Search remaining consumers for `m_flagBytes[3] &= 0xdf`; the original
  inventory is not a current exclusion or completion list.

## Compose the existing occupancy API

At source checkpoint `2e88f8b7b`, `CGrunt::LoadGruntCombatAnimations`
(`0x597a0`) still transcribed both operations through `m_flagBytes[3]` and
narrowed the actual `CGruntzMapMgr*` receiver to its base. Reusing the existing
`ReleaseCellOccupancy` and `AcquireCellOccupancy` header methods preserves the
two separate row expressions and names their ownership operation. No new
helper, cached cell reference, out-of-line call or bounds guard is needed.

Controlled real-TU sequence (current fuzzy, not historical MAX):

| State | LoadGruntCombatAnimations | StepBehavior |
|---|---:|---:|
| Baseline | 66.33637% | 66.79507% |
| Three scalar `SQR` uses | 66.33637% | 66.79507% |
| Required occupancy header only | 66.33637% | 66.79389% |
| Release helper composed | 66.35839% | 66.79389% |
| Both helpers composed | 66.87775% | 66.79272% |

The square-only normalized COFF is byte-identical to baseline. The other 44
scored TU records remain unchanged through the complete composition. The final
load body remains 5,272 bytes, but loses one instruction (1,355 to 1,354);
its 59 calls, 190 branches, six returns and 242 references are unchanged.
It still differs from retail's 57 calls and 184 branches: this is an applied
source correction, not a closed caller or a proven inline-budget repair.

Retail release `0x5a940` and acquire `0x5a989` each perform the narrowed byte
memory operation, reload `m_rows`, and then store the occupant at cell offset
four. The final production body restores that protocol at offsets `0x126b`
and `0x12c0`. Keep the separate old/new grid receiver loads. Do not infer
complete indexing or register equivalence merely from this local signature.

`scripts/test_combat_helper_consumers.py` exercises both actual production and
original phases, rejecting changed masks and missing row reloads. It also
compares the complete entrance-flash phase, allowing only six independently
constrained stack displacement changes; four named reference identities and
four emitted literal payloads are checked, not final literal placement.
Guard, square, high-word, reference, multiplicity and addend negative controls
prevent the matcher from accepting a superficially similar phase. Full helper
semantics and member layout remain covered by `test_map_occupancy_headers`.

Reverse-use rule: inspect the full receiver and existing helper before
transcribing byte stores. A header-only control separates declaration-state
movement from the actual helper composition; an unchanged call-set gap must
remain open even when the memory access shape improves.
