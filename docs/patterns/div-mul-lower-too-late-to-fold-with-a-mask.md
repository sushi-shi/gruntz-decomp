# `and 0xff / shr 3 / shl 0xb` kept SEPARATE means `/ 8 * 0x800`, not `>> 3 << 0xb`

tags: cpp:int cpp:const | asm:and asm:shr asm:shl | topic:codegen-idiom
symptoms: retail keeps a mask and two shifts as three instructions, ours folds them into one
`and`+`shl` with a merged mask (`and 0xf8; shl 8`), and the frame/regalloc shifts with it
confidence: 9/10

cl5's peephole folds an adjacent mask/shift chain: `((x & 0xff) >> 3) << 0xb` becomes
`and reg,0xf8` + `shl reg,8`, because at the point the peephole runs both shifts are already
SHIFT nodes and it can merge them into the mask.

Spelling the same value with `/` and `*` defeats it. cl5 lowers a power-of-two divide and
multiply to shifts LATE — after that peephole window — so the mask and the two shifts survive
as three instructions:

```cpp
i32 bank = (shade & 0xff) / 8 * 0x800;   // YES - and 0xff / shr 3 / shl 0xb
// NO: i32 bank = ((shade & 0xff) >> 3) << 0xb;   // and 0xf8 / shl 8
```

```asm
and    ebx,0xff        ; retail
shr    ebx,0x3
shl    ebx,0xb
```

The two spellings are value-identical here because the mask makes the operand non-negative, so
`/8` needs no sign correction (`cdq`/`and 7`/`add`/`sar`) and lowers to a bare `shr`. Check that
before swapping: on a possibly-negative operand `/8` is NOT `>> 3` and cl5 emits the correction.

STEERABLE. `CDDSurface::ShadeBlt` 0x13f020: 66.6 -> 68.2 from this one line. The inverse read is
the useful one — when retail keeps a mask and a shift apart that you folded, look for a `/` or `*`
in the original, not for a regalloc wall.
