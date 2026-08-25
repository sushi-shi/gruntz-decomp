# The "rand() % var divisor-zero peel" is not a peel — it is an inlined `GetRandom(lo, hi)`
tags: cpp:modulo cpp:rand cpp:inline | asm:idiv asm:test asm:jcc | topic:codegen-idiom
symptoms: a `test n,n / jne` around TWO distinct `call _rand`s, one feeding `cdq; idiv`, the other a coin-flip select between two endpoints
confidence: 9/10

## What it actually is

Retail has **131** `call _rand` sites. **26 of them, across 16 functions, come in PAIRS**:

```
    lea  ebp,[edi+1]          ; n = hi - lo + 1
    cmp  ebp,ebx              ; ebx == 0
    jne  div
    call _rand                ; <-- FIRST rand: the degenerate arm
    and  al,1
    neg  al
    sbb  eax,eax
    not  eax
    and  eax,edi              ; (rand() & 1) ? 0 : (hi - lo)
    mov  edx,eax
    jmp  join
div:
    call _rand                ; <-- SECOND rand: the normal arm
    cdq
    idiv ebp
join:
    add  edx,7530h            ; + lo
```

MSVC 5.0 does **not** peel a divisor-zero guard around `%`, and it cannot invent a
`rand()`. Two `call _rand`s means **two `rand()` calls in the source**. The source is a
Monolith inline helper over a CLOSED range, recovered in `include/Gruntz/GameRand.h`:

```cpp
__inline i32 GetRandom(i32 lo, i32 hi) {
    i32 n = hi - lo + 1;
    if (n == 0) {
        return (rand() & 1) ? lo : hi;
    }
    return lo + rand() % n;
}
```

`n == 0` means `hi == lo - 1` (an inverted/empty range), and the helper answers it with a
coin flip between the two endpoints. Every site's guard is dead at run time — which is
exactly the signature of an inlined helper rather than a hand-written check.

## Writing a site

Read `lo` off the `add edx,<imm>` in the join block and `hi` off the `lea` that forms `n`:

* `add edx,7530h` with `lea ebp,[edi+1]`  ->  `0x7530 + GetRandom(0, d)`
  (cl emits the branchless `sbb/not/and` select because one endpoint is the literal 0)
* `add edx,4e20h` with `lea edi,[ebp-4e1fh]`  ->  `GetRandom(0x4e20, d)`
  (both endpoints are values, so cl emits the branchy `mov edx,lo / mov edx,hi` select)

Both forms reproduce retail's instruction sequence exactly (verified in
`CGrunt::ResetEntranceAnimation`, 0x62e10).

## The one arm that still differs

`GetRandom(1, count)` folds: `n = count - 1 + 1 == count`, so inside the degenerate arm our
cl *proves* `count == 0` and substitutes the literal, collapsing `(rand() & 1) ? 1 : count`
to `movsx edi,al; and edi,1`. Retail keeps `mov edi,1` against a live `count`. Same helper,
different constant propagation; not a source difference we have been able to name.

## History

This file previously claimed the guard was a *compiler* peel that "no honest spelling
reproduces", and told matchers to accept the gap. That was wrong, and it parked
`ResetEntranceAnimation` at 61% with three missing `rand()` calls. The tell that broke it was
a CRT-symbol reference census (retired): retail referenced `_rand` 131 times and our base only 126, and the
five missing references localized to three functions in one file.

The 16 functions with paired sites (retail RVAs): `ChooseIdleBehavior` 0x2f620 (x4),
`ResetEntranceAnimation` 0x62e10 (x3), `LoadAttributes` 0x810f0 (x3), `ScanShuffleQuads`
0xd9290 (x3), `ApplyGruntAreaEffect` 0x7b930 (x2), and one site each in `StepCompassMove` 0x51c00,
`LoadGruntAbilityTuning` 0x57100, `UpdateArrival` 0x62110, `RunMoveConfig` 0x65630,
`StepBomberBehavior` 0xec670, `StepHitAndRunnerBehavior` 0xed9f0, `StepDumbChaserBehavior` 0xef6b0,
`UpdateArrival` 0xf0130, `StepScrollGruntBehavior` 0xf2b20, `StepSmartChaserBehavior` 0xf42f0,
`StepMagicWandGruntBehavior` 0xf8240. Most already spell the guard out by hand; those are the
`inline_clones` worklist for folding onto `GetRandom`.
