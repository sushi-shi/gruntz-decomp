# A cumulative numerator is an authored strength reduction, not `value * index`

tags: cpp:loop cpp:local cpp:integer | asm:add asm:idiv asm:mov | topic:codegen-idiom topic:mis-model
symptoms: retail carries several zero-initialized dwords across an outer loop, divides each
immediately before use, and increments all of them at one shared loop tail; source instead
recomputes `channel * index / divisor` at the top of every iteration
confidence: 10/10

Two integer formulations can produce the same sequence without presenting the same values
or lifetimes to cl 5.0:

```cpp
// Mathematically equivalent, but not the retail source shape.
for (i32 fill = 0; fill < 16; fill++) {
    i32 shade = channel * fill / 15;
    // use shade
}

// Retail shape.
i32 shadeNumerator = 0;
for (i32 level = 15; level > -1; level--) {
    // use shadeNumerator / 15
    shadeNumerator += channel;
}
```

The distinction is stronger when several channels move together. Retail
`CShadeTableCache::SubTable` 0x14f310 initializes three adjacent dword homes to zero,
loads and divides the appropriate home at each red/green/blue use, and performs three
adds by the unpacked colour channels only after the complete nested pixel loop. That is
direct evidence for three cumulative numerators; it is not merely an optimizer choice
between equivalent expressions.

Replacing three `channel * fill / 15` temporaries with the carried numerators moved the
function from **89.76% to 96.36%**, 214 to 213 instructions, and 0x29e to 0x29c bytes
against retail's 211 instructions and 0x297 bytes. Calls, branch/return skeleton,
constants, and all 12 ordered referents then agree. The remaining difference is register
allocation: the base reloads the green and blue numerator homes in the opposite register
order and accumulates the packed pixel in EAX/AX rather than retail's EDX/DX.

## Negative controls

- All six numerator declaration orders crossed with all six tail-update orders produced
  no closer instruction stream; declaration order can recolour the residue but cannot
  replace the missing loop-carried model.
- Counting upward with a derived `level`, counting downward directly, and equivalent
  `while`/`do` spellings were byte-identical once the numerator model was present.
- A file-inline `PackPixel16(u8,u8,u8)` with sequential `u16` accumulation is
  byte-identical to the same sequential statements in the loop. The helper is an
  abstraction prior, not the cause of this improvement.
- Explicit coordinate accumulators for the three inner loops preserved the inner-loop
  bytes but rotated earlier register roles and fell to 83.83%. They do not explain a
  feature absent from the 96.36% baseline.

## Reverse reading

When retail has one initialized home per channel, repeated `idiv` uses inside a loop nest,
and a group of channel adds at the common outer back edge, model the homes as cumulative
numerators before trying declaration-order or register-state permutations. Only after
that structure agrees is the remaining colour a register/scheduling wall.

related: [One OR expression lets cl reassociate and hoist the loop-invariant pair](or-chain-reassociates-and-hoists-the-invariant-pair.md)
