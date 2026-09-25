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
coin flip between the two endpoints. The guard is not necessarily dead at run time:
that requires a proven domain for the particular caller. Constant nonempty bounds
can erase it, while variable-bound callers must retain the exceptional path.

## Writing a site

Read `lo` off the `add edx,<imm>` in the join block and `hi` off the `lea` that forms `n`:

* `add edx,7530h` with `lea ebp,[edi+1]`  ->  `0x7530 + GetRandom(0, d)`
  (cl emits the branchless `sbb/not/and` select because one endpoint is the literal 0)
* `add edx,4e20h` with `lea edi,[ebp-4e1fh]`  ->  `GetRandom(0x4e20, d)`
  (both endpoints are values, so cl emits the branchy `mov edx,lo / mov edx,hi` select)

Both forms reproduce retail's instruction sequence exactly (verified in
`CGrunt::ResetEntranceAnimation`, 0x62e10).

## Constant and variable bounds need separate controls

Constant bounds can fold the peel away completely without erasing the helper's
effect on the caller. The [CMenuSparkle constructor control](constant-range-helper-changes-constructor-zero-carrier.md)
restores exactness through `GetRandom(1000, 5000)` even though both versions
contain the same single CRT random call and remainder arithmetic. An
include-only control is byte-flat; the actual inline call changes an earlier
zero carrier in the constructor chain.

`GetRandom(1, count)` can fold the width back to `count`, and the zero arm can
collapse to `movsx edx,al; and edx,1`. Check the particular retail caller:
the former claim that retail necessarily keeps a live endpoint is not a family
exclusion. The three brick-color sites below already use the collapsed parity
form in retail.

## A complete variable-bound wrapper can preserve the signed divisor

The three color-selection paths in `BuildCellAttributes` (0x810f0) repeat one
protocol: test the full signed total, draw parity if zero, otherwise draw a
signed remainder and increment it. Restoring the complete existing range helper
inside `RollBrickColor` gives this controlled real-TU result:

| Source state | Normalized bytes | Instructions | Calls / branches / returns | References |
| --- | ---: | ---: | --- | ---: |
| Expanded zero/remainder body | 2,666 | 753 | 23 / 127 / 2 | 122 |
| `return GetRandom(1, totalWeight)` | 2,666 | 753 | 23 / 127 / 2 | 122 |
| Same, redundant direct CRT include removed | 2,666 | 753 | 23 / 127 / 2 | 122 |

All three states have identical complete normalized bytes and ordered
relocation offsets, targets and kinds. Switch-table data is excluded from the
instruction census. Current fuzzy remains 90.2473%; the unrelated caller CFG
residue is still open. This is source restoration, not an exact closure.

At each actual site, VC5 uses the original full-dword total in `TEST` and
`IDIV`; no subtract-one instruction or narrowed divisor remains. Thus the
emitted sequence works at `INT_MIN` too: CRT `rand()` yields 0..32767, so this
division cannot encounter the signed division overflow case. The zero path
still executes exactly one draw and chooses odd→1/even→0. This proof is about
the pinned compiler's emitted program. It does **not** make `hi-lo+1` portable
signed arithmetic at every endpoint, nor prove callers supply positive bounds.

Reverse-use rule: test the complete sourced helper before excluding it from an
expanded remainder signature. Audit the full bound-producing dataflow, both
draw referents and guard destinations, and arithmetic extremes—not just the
nominal probability or current score. `scripts/test_brick_color_rng.py` checks
the three original/production local protocols and rejects changed divisors,
parity masks, signed comparisons, guard edges and either RNG referent. Whole
caller before/after equality is a separate control; these tests do not certify
the entire game-mode/switch/loop CFG. The specific adoption and domain-review
dispositions live in `brick-rng-color-range` and `brick-color-*`.

## Preserve the sampled integer before choosing a predicate abstraction

PR #79's fresh RNG reassessment tested the actual VC5 translation units, not
isolated replacements for their callers. Two different authored boundaries are
available: `GetRandom(lo, hi)` returns the integer sample, whereas
`IsRandomChance(percent)` returns `char`. Equivalent outcomes do not make their
expanded intermediate values identical.

| Actual consumer | Controlled source form | Current fuzzy | Observed result |
| --- | --- | ---: | --- |
| `FindIdleGruntInBox`,0x2ab80 | Existing direct remainder | 83.1071 | Conditional clear of caller-owned integer `keep` |
| Same | Existing chance helper in the nested negative guard | 79.8839 | Additional `setl al` and byte test |
| Same | Assign chance result to `keep` | 81.0446 | `movsx` and one fewer branch |
| Same | Existing integer range helper, original caller statements | 83.1071 | Entire normalized body and ordered references identical to baseline |
| `BuildCellAttributes`,0x810f0 | Original four two-stack remainder ternaries | 89.3672 | Four one-based `inc / cmp 50 / setle / add enum` sequences already present |
| Same | Include-only control | 90.2473 | Declaration-context movement before actual helper use |
| Same | Four chance-helper ternaries | 87.7073 | Byte-to-condition conversion adds `neg al / sbb eax,eax / neg eax` |
| Same | Four inclusive-range ternaries; only used header retained | 90.2473 | All four original local sequences and ordered references preserved |
| Same | Compose all four three-stack range initializers | 90.2473 | Entire normalized body and references identical to the preceding range state |

The reverse-use signature is a materialized integer sample: inspect its offset,
signed comparisons, reuse, and subsequent enum construction before selecting a
boolean-returning helper. A folded exceptional branch does not disprove an
inclusive-range inline. Conversely, an aggregate improvement already reproduced
by include-only control is not evidence that the new call boundary improved
instruction scheduling. No unused include is retained as a steering device.

These are scoped source restorations, not exact closures or bounded whole-caller
verdicts. The ghost frame/allocation difference and brick caller CFG remain open.
Canonical dispositions and reopening criteria are `chance-ghost-range-preserve`,
`chance-brick-range-preserve`, `brick-rng-three-stack-range`,
`brick-rng-shogo-fifty-oracle`, and `brick-rng-include-only`; the broad
`reassess-israndomchance` family remains pending. Production-object/original-PE
negative controls live in `scripts/test_rng_helper_consumers.py`.

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
