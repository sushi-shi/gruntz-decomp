# A guard-crossing counter lifetime enables the authentic postincrement pair
tags: cpp:local cpp:loop cpp:array cpp:increment | asm:inc asm:test asm:mov | topic:codegen-idiom topic:regalloc
symptoms: a literal-run loop has the right CFG but one indexed output update where
retail has two cursor increments; writing the postincrement form alone produces a
large score dip and a different instruction-count island
confidence: 9/10

cl 5.0 chooses the lowering of two adjacent byte stores together with the lifetime
of the loop counter. A counter declared inside the positive-length guard is born
after the test. Retail zeroes its counter before the test, so the source counter must
exist outside that guard. Once that lifetime is restored, the natural two
postincrement stores lower as two `inc` instructions and the complete literal loop
matches. Testing the store spelling without the lifetime change gives a false
negative.

```cpp
outidx++;
i32 k = 0;
if (src[srcidx] > 0) {
    do {
        u16 px = table[src[srcidx + k + 1]];
        out[outidx++] = static_cast<u8>(px);
        out[outidx++] = static_cast<u8>(px >> PIXEL_BITS_PER_BYTE);
        k++;
    } while (k < src[srcidx]);
}
```

Steerable. `CDDrawShadeBlit::EncodeRle16` 0x001495d0 moved from 91.9592 to
97.8231. The baseline after the other recovered expression/order changes was
94.3537. Moving `k = 0` before the guard alone reached 95.6122. Applying only the
postincrement stores fell to 79.6054 (418 bytes, 149 instructions), but composing
the lifetime and store changes reached 97.7551 and retail's exact 422-byte extent,
148-instruction count, call set, branch/return skeleton, relocation sequence, and
literal-loop instruction stream. Removing the named `run` pointer and indexing the
source directly reached 97.8231 by recovering the remaining SIB expression in the
second pass.

The evidence was a reviewed 24-state Cartesian matrix over output-store spelling,
counter placement, and source-cursor lifetime. Six counter-order cells, eight
lexical-scope cells, helper/macro boundaries, byte-extraction spellings, and prefix
versus postfix increments formed flat or worse compiler islands. Conditioned
32-island/four-frontier and 60-state TU probes found only the 97.8231 baseline and a
worse 96.8707 state. The remaining difference is isolated to first-pass ESI/EDI
colour and the timing of the second-pass zero; it is not evidence against the
recovered literal loop.

Reverse-use rule: if retail initializes a loop counter before a zero-trip guard,
restore that lifetime before ranking cursor/postincrement spellings. Verify that the
target features are absent from the baseline and judge the composed state by extent,
CFG, calls, and the loop itself; a severe one-step score dip does not refute the
authentic expression.

related: [hand-carried-loop-index-lowers-register-pressure](hand-carried-loop-index-lowers-register-pressure.md), [late-store-keeps-the-loaded-byte-in-its-own-register](late-store-keeps-the-loaded-byte-in-its-own-register.md)
