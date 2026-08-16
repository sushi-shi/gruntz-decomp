# An `i32 flag; if(a) flag=X; else flag=Y; if(flag)` materializes a value retail branched on
tags: cpp:branch cpp:local cpp:loop | asm:jmp asm:jcc asm:test | topic:codegen-idiom
symptoms: base has ~25% MORE basic blocks than target with the same instruction total; base's
extra blocks are 1-2 instructions ending in `jmp`, and the target's are uniform 5-instruction
compare-and-branch blocks; a `gruntz walls diagnose --asm` skeleton where base reads
`2i [jcc] / 2i [jmp] / 2i [jcc] / 2i [jmp] / 1i [fall] / 2i [jcc]` against target
`5i [jcc] / 5i [jcc] / 5i [jcc] / 5i [jcc]`
confidence: 9/10

## Symptom

`ImagePolyClipRect` @0x001461b0 ran four Sutherland-Hodgman clip stages. Each was
transcribed with an explicit predicate variable:

```cpp
i32 emit;
if (prev->x < left) {
    emit = !(cur->x < left);
} else {
    emit = (cur->x < left);
}
if (emit) { /* emit the intersection vertex */ }
```

cl5 has to PRODUCE the value: two arms each set a register and `jmp` to a join, the join
falls into a `test`/`jcc`. That is **three extra basic blocks per stage** — 61 blocks
against retail's 49 — and every one of them shifts the surrounding register colouring.

## The retail shape

Retail has no join and no value: four consecutive 5-instruction
`fld / fcomp / fnstsw / test ah,1 / jcc` blocks feeding ONE emit block. That is a
short-circuit `||` of two `&&` clauses, spelled directly in the `if`:

```cpp
if ((prev->x < left && !(cur->x < left)) || (!(prev->x < left) && cur->x < left)) {
    /* emit */
}
```

The second clause re-tests `prev->x < left` and retail emits that re-test — cl5 does not
jump-thread the first compare into the second clause. (Same non-threading as
[[redundant-sibling-guard-retest]]; the redundancy is the evidence, not a defect.)

## Why it is worth checking first on any predicate-heavy function

The block COUNT is the screen, and it is visible in one command:

    gruntz walls diagnose <rva> --asm

If base has more blocks than target and the extra ones are tiny `jmp` blocks, some
condition in your source is being computed as a value instead of branched on. There is no
regalloc to chase — the shape is wrong.

## Evidence

`ImagePolyClipRect` 68.60 -> **85.59** on this change alone (four sites, one regex), then
-> **98.62** once the intersection's division was parenthesised
(`py + (b - px) * ((cy - py) / (cx - px))` — C++ left-associativity otherwise multiplies
first and reorders the whole `fld/fxch/fdivp` chain). Both had been sitting under one
`@early-stop`.

## Related

- [[redundant-sibling-guard-retest]] — the same non-threading, seen from the other side.
- [[map-lookup-ternary-ifconverts]] — the inverse lever: where retail *does* materialize a
  0/1, an `if` statement will not produce it.
