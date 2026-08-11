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
