# Split the row cursor: the OFFSET multiply belongs outside the width guard, the `base + off` add inside
tags: cpp:loop cpp:local cpp:pointer | asm:imul asm:add asm:test asm:jle | topic:codegen-idiom
variants: loop-preheader-init-sinks-behind-an-explicit-guard.md, licm-placement-pre-guard-vs-preheader.md

A per-row `p = base + i * pitch` written above the row's inner loop puts BOTH the
`imul` and the `add base` before cl's `test <w>,<w> / jle`. Retail splits them: the
multiply is pre-guard, the base addition is in the preheader. Wrapping the loop in
`if (w > 0)` alone moves the whole expression inside (still wrong, and it can score
WORSE); the shape retail has needs the offset named as its own statement.

```cpp
// BEFORE - imul and add both pre-guard
u8* top = buf + i * m_pitch;
i32 j = 0;
if (width > 0) { do { tmp[j] = *top; ++top; ++j; } while (j < width); }

// AFTER - imul pre-guard, add in the preheader (retail's split)
i32 topOff = i * m_pitch;
i32 j = 0;
if (width > 0) {
    u8* top = buf + topOff;
    do { tmp[j] = *top; ++top; ++j; } while (j < width);
}
```
```asm
    mov    esi,DWORD PTR [esp+0x10]   ; i
    xor    edx,edx                    ; j = 0
    imul   esi,DWORD PTR [ebx+0x20]   ; i * m_pitch   - PRE-GUARD
    test   ecx,ecx
    jle    <skip>
    add    esi,edi                    ; + buf         - PREHEADER
    mov    edi,DWORD PTR [esp+0x18]   ; tmp
<loop>:
```
STEERABLE. `CDDSurface::FlipVertical` 0x13ebb0 **70.55 -> 80.38**, and the row
reclassifies CFG -> REGALLOC (115/115 instructions, 12/12 branches, the loop-entry
trampoline appears on our side too). The intermediate spelling - pointer moved inside
the guard WITHOUT naming the offset - is only 71.39, so measure both. The expression
must be repeated textually per block (a member read is killed by the loop's byte
stores, so retail recomputes `i * m_pitch` for each row pass); one shared offset local
would be hoisted and does NOT reproduce it. Same family: hoisting the counter and the
scratch cursor into the guarded block took `CDDSurface::ShadeRect` 0x13f460 76.99 ->
79.00 and `CDDSurface::ShadeBlt` 0x13f020 70.14 -> 73.43 (`i32 n = width;` and
`u16* t = temp;` both belong INSIDE `if (width > 0)`, not above it).
