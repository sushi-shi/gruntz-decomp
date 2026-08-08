# A BYTE store into a slot then an UNMASKED dword read of it means a `u8` field + a whole-struct copy
tags: cpp:class cpp:local cpp:assign | asm:mov asm:and | topic:codegen-idiom
symptoms: target `mov BYTE PTR [esp+N],dl` in both arms of an if/else then `mov eax,DWORD PTR [esp+N]` + `mov DWORD PTR [dst+off],eax` with NO `and 0xff`; base has the same shape but keeps the `and`; four consecutive dword loads from four consecutive stack slots feeding four consecutive member stores
confidence: 9/10
variants: stack-aggregate-byte-member-loads-a-dword.md

Reading four bytes out of a one-byte local and storing all four looks like a
compiler bug until you notice the destination: it is not the field, it is the
whole struct. cl copies a small struct with one dword move per 4 bytes and
carries the tail padding, so a `u8` field with three pad bytes after it is read
as a dword. That combination - byte store in, dword read out, no mask - is only
producible by a `u8` field PLUS a whole-struct assignment. Either half alone
gives something else: a `u8` field with per-member stores writes the field with
`movb`, and an `i32` field with a `u8` source keeps the `and 0xff`.

```cpp
struct CFaderRadialCell { float m_vx, m_vy, m_radius; u8 m_pixel; };  // 0x10

CFaderRadialCell cell;
cell.m_radius = ...;                     // fst  [esp+0x20]  (fst, not fstp: r is reused)
cell.m_vx     = ...;                     // fstp [esp+0x18]
cell.m_vy     = ...;                     // fstp [esp+0x1c]
cell.m_pixel  = pix;                     // mov  BYTE PTR [esp+0x24],dl
m_cells[y * w + x] = cell;               // four dword moves, pad included
```
```asm
mov  BYTE PTR [esp+0x24],dl
...
mov  edx,DWORD PTR [esp+0x18]   ; mov DWORD PTR [ecx],edx
mov  eax,DWORD PTR [esp+0x1c]   ; mov DWORD PTR [ecx+0x4],eax
mov  edx,DWORD PTR [esp+0x20]   ; mov DWORD PTR [ecx+0x8],edx
mov  eax,DWORD PTR [esp+0x24]   ; mov DWORD PTR [ecx+0xc],eax   <- no `and 0xff`
```

Corroborate the field width on the READ side before retyping: the consumer in
another function loads it narrow (`mov al,BYTE PTR [ebx+0xc]` in
CFaderRadial::RenderFrame 0x17fd2a). Also note the FP writes land straight in
the struct temp - if your source computes `float vx, vy, r` first and copies
them in, cl inserts a `mov` per field that retail does not have.
CFaderRadial::ApplyInit 0x17fa40 86.39 -> 91.60 (the last step of which was
spelling m_vy's input `m_centerY - y`, a separate `sub`, rather than `-dy`).
