# 2D grid access: byte-offset address forces row-base then scaled index
tags: cpp:member cpp:array | asm:shl asm:lea asm:mov | topic:codegen-idiom

For `this+0xBASE + outer*0x10 + inner*4` style 2D-grid reads, retail computes the
ROW BASE first (`shl outer,4; lea reg,[this+reg+0xBASE]`) and indexes by the inner
dim (`mov eax,[reg+inner*4]`). Writing the access as a fused 1D index
(`m_grid[inner + outer*4]`) makes `cl` combine the two dims into one `lea` then a
single `*4` — different bytes AND it swaps the outer/inner register assignment.
Spell the address in explicit BYTE arithmetic so the row base (`outer*0x10`) and
the inner index (`inner*4`) stay separate, matching the target SIB exactly.

```cpp
// WRONG (fused index): lea eax,[eax+edx*4]; mov eax,[ecx+eax*4+0x98]
return m_flags[inner + outer * 4];
// RIGHT (row base + scaled index): shl eax,4; lea eax,[ecx+eax+0x98]; mov eax,[eax+edx*4]
return *(i32*)((char*)m_flags + outer * 0x10 + inner * 4);
```
```asm
mov  eax,[esp+0x4]      ; outer
shl  eax,0x4            ; outer*0x10  (row stride, BYTES)
lea  eax,[ecx+eax+0x98] ; row base = this + outer*0x10 + 0xBASE
mov  eax,[eax+edx*0x4]  ; + inner*4
```
Steerable. Closed CBattlezData::GetFlag 86.5%→100% (the byte-offset form also
fixed the outer/inner→eax/edx register split the fused index got wrong). The same
row-base spelling (`(char*)m_grid + y*0x10`) for a row-SUM loop closed
CBattlezData::SumWinRow once paired with an early `sum=0` (see
[zero-init-before-pointer-setup-for-loop.md](zero-init-before-pointer-setup-for-loop.md)).
