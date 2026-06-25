# Read header fields THROUGH the advancing cursor, not via array index
tags: cpp:pointer cpp:loop | asm:lea asm:add asm:mov | topic:codegen-idiom
symptoms: lea 0x4(ecx); add $4; mov (eax); add $4 vs a single lea 0xc(ecx) + direct mov 0x8(ecx); hoisted struct-header loads; reading riff[1]/riff[2]
confidence: 8/10

A function that validates a fixed header then walks chunks/records from the same
base (RIFF/WAVE parsers, TLV blobs, packet headers): if you read the header
fields by ARRAY INDEX (`hdr[1]`, `hdr[2]`) the optimizer hoists them to direct
`mov 0x8(ecx),edx` loads and folds the cursor to a single `lea 0xc(ecx)`. Retail
instead keeps ONE cursor pointer live from the header into the loop, reading each
field through it (`*p`) and advancing it with `p++` — emitting the incremental
`lea 0x4(base); add $4; mov (eax); add $4; lea -0x4(size,eax)` walk. Reproduce by
reading the post-magic header fields through the SAME pointer that becomes the
loop cursor.

```cpp
u32* p = (u32*)((char*)riff + 4);   // not (u32*)riff + N
u32 size = *p; p++;                  // read THROUGH p, then advance
u32 tag  = *p; p++;
char* end = (char*)p + size - 4;
if (*(u32*)riff != 0x46464952) return 0;  // re-read magic from base, read LATE
```
```asm
lea  0x4(%ecx),%eax
add  $0x4,%eax
mov  (%eax),%esi          ; tag read through the cursor
add  $0x4,%eax
lea  -0x4(%edx,%eax),%edi ; end = size + cursor - 4
```
Steerable: forces the pointer-walk prologue (ParseWaveChunks 0x137110, 84→98%).
Residual is only the optimizer folding the in-loop `p+=2` to `add $8` vs retail's
two `add $4` (an entropy-tail wall, not steerable from C).
