# `& ~0x1f` align-down emits `and al,0xe0` (byte) when only the low byte changes — write `& ~0x1f`, not `& 0xe0`
tags: cpp:bitand | asm:and | topic:codegen-idiom
symptoms: target uses `and al,0xe0` (byte, `24 e0`) for one axis and `and edi,0xffffffe0` (dword) for another doing the SAME align-down
confidence: 7/10

`x & 0xffffffe0` (align-down-to-0x20) is encoded `and al,0xe0` (byte form, `24 e0`) when the
value MSVC tracks as low-byte-only and the result is stored back as the SAME int — `al & 0xe0`
keeps the upper 24 bits unchanged, so it IS `& ~0x1f`. The same operation on a value tracked
full-width emits `and r32,0xffffffe0`. Both are the source-level `& ~0x1f`; cl picks the
byte-encoding for whichever operand it has narrowed.

```cpp
x = (x & ~0x1f) + 0x10;   // NOT (x & 0xe0) — 0xe0 would also clear bits 5-7 of higher bytes
```
STEERABLE (write `& ~0x1f` and let cl pick the encoding per value). Evidence: CPlay::StepScroll
(@0xd1ac0) aligns two axes the same way; target uses dword `and` for one, byte `and al,0xe0`
for the other.
