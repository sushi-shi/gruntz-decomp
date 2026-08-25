# A `&=`/`|=` on a byte field of a 2-D row array materialises the CELL pointer first
tags: cpp:struct cpp:local cpp:array | asm:and asm:or asm:add asm:lea | topic:codegen-idiom topic:regalloc
symptoms: `and BYTE PTR [reg+eax*1+0x3],0xdf` in base against `add ecx,eax / and BYTE PTR [ecx+0x3],0xdf` in retail; a dead `lea reg,[reg+idx+0x3]` right after the read-modify-write; the row load and the index scaling are identical on both sides
confidence: 8/10
variants: array-cursor-bias-from-row-pointer-local.md, pointer-chain-hoist-intermediate-local.md

A read-modify-write of one byte inside `grid->m_rows[y][x]` (a `T** m_rows`
row-pointer table) written as one expression lets cl fold the whole address —
row base, scaled column, field offset — into the `and`'s addressing mode, and
then emit a redundant `lea` for the byte's address. Retail instead computes the
CELL pointer into a register and applies a disp8. Naming the intermediate
reproduces it. (Note the OPPOSITE direction from
`array-cursor-bias-from-row-pointer-local.md`: that one is about a LOOP whose
cursor cl strength-reduces, where plain subscripting is retail's; this is a
single RMW, where the named cell pointer is retail's.)

```cpp
// NO - one expression: cl folds row+col*stride+3 into the `and`, then leas it again
grid->m_rows[y][x].m_flagBytes[3] &= ~0x20;

// YES - retail's shape
BrickzCell* c = &grid->m_rows[y][x];
c->m_flagBytes[3] &= ~0x20;
```
```asm
mov    ecx,DWORD PTR [ecx+ebp*1]      ; row = m_rows[y]
add    ecx,eax                        ; cell = row + x*sizeof
and    BYTE PTR [ecx+0x3],0xdf        ; disp8 off the cell
```
STEERABLE. `CGrunt::LoadEntranceConfig` 0x67f80 87.00 -> 89.03 and
`CGrunt::FinishActiveAction` 0x6a6d0 88.96 -> 89.11 (four sites); retail
0x68110 and 0x6b0d2. Thirteen more `m_flagBytes[3] &=`/`|=` sites exist tree-wide.

## The DWORD `|=` behaves the same way

The field width is not the variable - a whole-dword `m_flags |= BIT` on the
same 2-D row array splits identically when it is written as one subscript
expression, and folds to a memory-RMW off a named cell pointer:

```asm
; base, subscript spelling: dead lea + indexed load + register or + store
lea    ecx,[eax+esi*1]
mov    eax,DWORD PTR [eax+esi*1]
or     eax,0x20000000
mov    DWORD PTR [ecx],eax
; retail (and base once the cell pointer is named)
add    eax,esi
or     DWORD PTR [eax],0x20000000
```

`CMapMgr::FindPathWithEndpointOverrides` 0x81e10 83.13 -> 86.37 on the one site (the restore of
`BRICKZ_CELL_OCCUPIED` at the tail).

## Per SITE, never tree-wide

Retail also emits the split form. `CTriggerMgr::LoadTileArrivalFx` 0x75e90 has
four `m_rows[y][x].m_flags &= ~0x40000` sites and retail spells every one of
them `lea eax,[ecx+edx*1] / mov ecx,[ecx+edx*1] / and ecx,imm / mov [eax],ecx`,
which the plain subscript already reproduces. Read the retail site before
naming a pointer; converting a matching site regresses it.
