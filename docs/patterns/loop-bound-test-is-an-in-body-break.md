# Retail's UNCONDITIONAL back-edge (`jcc <far exit>; jmp <head>`) = the bound test is an in-body `break`
tags: cpp:loop cpp:branch cpp:return | asm:jmp asm:jcc asm:cmp | topic:codegen-idiom
symptoms: jge far exit, jmp back to loop head, unconditional back-edge, `return 0` laid out LAST, `fall Bn` where target has `jmp Bn^`, one POLARITY row on the last conditional branch, `continue`
confidence: 9/10
variants: loop-rotates-on-the-CONDITION-not-the-first-break.md, if-body-owns-the-fallthrough.md

A counted scan whose block/instruction diff is otherwise clean but whose LAST
conditional branch has inverted polarity, and whose trailing `return 0` retail lays
out AFTER the in-loop `return 1` instead of before it. cl5 rotates a
`for (i = 0; i < N; i++, p++)` so the bound test IS the back-edge (`jl <head>`,
exit falls through, and the exit block therefore gets placed right after the loop).
Retail instead branches OUT of the loop and takes an unconditional back-edge, which
only happens when the bound test is a `break` at the BOTTOM of a `for (;;)` — then
the break target (the trailing `return`) is a forward block placed last, after every
in-loop `return`. The `continue`s become nesting, which is byte-neutral: a `continue`
and the end of an `if` body both land on the increment.

```cpp
// NO - cl rotates this; the exit becomes the fall-through and lands BEFORE `return 1`
for (i32 i = 0; i < 15; i++, p++) {
    if (a == NULL) continue;
    ...
    if (hit) return 1;
}
return 0;

// YES - the bound test is a break, so the exit is a FAR forward block placed last
for (;;) {
    if (a != NULL && ...) {
        ...
        if (hit) return 1;
    }
    i++;
    p++;
    if (i >= 15) {
        break;
    }
}
return 0;
```
```asm
    inc    eax
    add    edi,0x4
    cmp    eax,0xf
    mov    DWORD PTR [esp+0x14],eax
    jge    0x306c5          ; FAR - the trailing return, placed after `return 1`
    jmp    0x305dd          ; unconditional back-edge
```
STEERABLE. `CBattlezMapConfig::IsCoordOccupied` 0x305b0 69.91 -> 92.15 with
`--branches --diff` going from one POLARITY row to "branch sequences AGREE"; its
residue is regalloc only afterwards. Screen for it with `--blocks --diff --lite`:
retail has `1i [jmp B<n>^]` where the base has `1i [fall B<n>]`. The INVERSE
direction (base takes the unconditional back-edge, retail the conditional one) is
the epilogue tail-merge wall, not this.
