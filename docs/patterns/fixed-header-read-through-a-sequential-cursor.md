# A run of `add <reg>,K` immediates over a fixed record head is a sequential field cursor
tags: cpp:local cpp:pointer cpp:struct | asm:add asm:lea | topic:codegen-idiom
symptoms: add eax,0x4 twice adjacent, add ebx,0xc, skipped header fields as immediates, EXCLUSIVE disp keys the other side does not have, RecordBytes, RIFF, PidHeader
confidence: 10/10

A decoder that reads a fixed file-record head shows, on the retail side, one
register walking the record with an `add` per source statement and reads at
`[reg]`/`[reg+4]`, while member access (`buf->width`) folds every offset into a
displacement instead. `walls semdiff` names it as EXCLUSIVE `disp` keys on the
member side against EXCLUSIVE `imm` keys on retail's. The add immediates decode
the source directly: cl folds two `+=` steps only when nothing reads between
them, so each emitted immediate is the sum of the consecutive skipped fields.

```cpp
RecordBytes<PidHeader> p;
p.m_rec = buf;
p.m_bytes += sizeof(u32);              // formatTag
PidFlags flags = static_cast<PidFlags>(*p.m_dwords);
p.m_bytes += sizeof(u32);
i32 width = *p.m_dwords;
p.m_bytes += sizeof(u32);
i32 height = *p.m_dwords;
p.m_bytes += sizeof(u32);
p.m_bytes += 2 * sizeof(u32);          // offsetX, offsetY
i32 fill = *p.m_dwords;
p.m_bytes += sizeof(u32);
p.m_bytes += sizeof(u32);              // reserved1c
```
```asm
lea    ebx,[eax+0x4]
mov    ecx,DWORD PTR [ebx]
mov    ebp,DWORD PTR [ebx+0x4]
add    ebx,0x4
add    ebx,0x4
mov    esi,DWORD PTR [ebx]
mov    edx,DWORD PTR [ebx+0xc]
add    ebx,0xc
add    ebx,0x8
```
STEERABLE. `ParseWaveChunks` 0x137110 67.05 -> 100.00 EXACT (its chunk loop reads
id and size then steps `+= 4` twice, which is why two `add eax,4` sit adjacent);
`CRezImage::DecodeRidData` 0x1762c0 74.30 -> 100.00 EXACT (adds 8/4/0x14);
`CRezImage::DecodePidData` 0x176440 first moved 76.18 -> 81.48 (adds
4/4/0xc/8). The pixel walk must continue from the SAME cursor - a `u8* src =
p.m_bytes;` copy costs a `mov` retail does not have (5.3 points on the old
transcription). The public tree's 1996 `CDib::InitPid` later proves why the
cursor exists: the authored source names all eight `DWORD` header values while
post-incrementing one `DWORD*`, including values C2 deletes. Restoring that
complete header census together with the sibling decoder's run-loop body moves
81.48 -> 100.00 EXACT. The sequential cursor inference was correct; treating
its first partial reconstruction as a bounded whole-function source model was
not.
