# Accumulator `=0` BEFORE the cursor setup pins it in eax + frees the index reg
tags: cpp:loop cpp:local | asm:xor asm:shl | topic:codegen-idiom

In a sum/accumulate loop seeded from a scaled argument (`p = base + arg*stride;
sum=0; loop`), the SOURCE ORDER of `sum=0` vs the cursor computation decides the
register layout: declaring `sum=0` AFTER the pointer makes `cl` keep the argument
in eax and zero the accumulator late (in a spare reg), so the index/counter
registers diverge. Put the `sum=0` FIRST so its `xor eax,eax` is emitted eagerly,
the scaled argument lands in edx, and edx is then reused for the loop counter —
matching retail.

```cpp
// WRONG: arg in eax, xor late, counter in fresh reg
i32* p = (i32*)((char*)m_wins + y * 0x10);
i32 sum = 0;
for (i32 c = 0; c < 4; c++) sum += *p++;
// RIGHT: xor eax,eax first; y in edx; mov edx,4 (counter) reuses edx
i32 sum = 0;
i32* p = (i32*)((char*)m_wins + y * 0x10);
for (i32 c = 0; c < 4; c++) sum += *p++;
```
```asm
mov  edx,[esp+0x4]    ; y
xor  eax,eax          ; sum=0  (EAGER)
shl  edx,0x4          ; y*0x10
lea  ecx,[edx+ecx+0x58]
mov  edx,0x4          ; counter reuses edx
```
Steerable. Closed CBattlezData::SumWinRow 85%→100% (paired with the byte-offset
row base, see byte-offset-address-row-base-scaled-index.md).
