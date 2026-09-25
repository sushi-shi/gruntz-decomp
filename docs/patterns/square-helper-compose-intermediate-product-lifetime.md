# Compose a square helper with the intermediate product lifetime

tags: cpp:macro cpp:inline cpp:local | asm:fld asm:fmul asm:fxch | topic:scheduling topic:source-lineage
symptoms: a square-helper restoration preserves the arithmetic but VC5 multiplies a displacement by the constant before the acceleration, reversing retail's product grouping
confidence: 8/10

The motion family provides a controlled non-exact example of why a helper's
first score is not its final source-shape test. Starting at `7ba449374`, apply
the existing `SQR` at all three authored discriminant sites: the shared
`ARRIVAL_V` macro, expanded nine times in `Step`, and `ArrivalVelX/Y`.
Compare with the complete `Sqr<T>(const T&) -> T` template from the pinned
source, not a made-up inline signature.

Then compose an independently evidenced intermediate:

```cpp
double delta = (target - position) * acceleration;
double disc = SQR(velocity) - delta * g_motionNegTwo;
```

Retail multiplies displacement by acceleration before negative two. The
baseline and both helper-only profiles lack that order. Both composed forms
recover it in the standalone arrival bodies, without adding a stack spill.
This is a baseline-delta check, not an inference from a dipped state alone.

## Real-TU controls

All trials compile the actual `MovingLogic.cpp` with pinned VC5 `/O2 /MT`.
No inert declarations, forced spills, layout changes or new TUs participate.

| Complete three-site profile | Step fuzzy | Step bytes / instructions | Arrival X and Y fuzzy |
| --- | ---: | ---: | ---: |
| Baseline expanded squares | 80.6230% | 1758 / 557 | 95.6250% |
| Shared square macro | 80.8294% | 1740 / 548 | 91.2500% |
| Macro plus corrected zero referents | 80.8294% | 1740 / 548 | 91.8750% |
| Const-reference template plus zero referents | 80.6912% | 1764 / 560 | 96.2500% |
| Template plus intermediate product | 83.0952% | 1746 / 551 | 93.7500% |
| Macro plus intermediate product | 82.3465% | 1758 / 557 | 93.7500% |
| Macro plus intermediate and eager target value | 81.0826% | 1796 / 572 | 93.7500% |
| Same eager target reused for the product | 81.0826% | 1796 / 572 | 93.7500% |

The two unrelated scored TU owners remain 98.545456% and 100% throughout.
Step retains zero calls, 64 branches and two returns. All arrival variants
remain 97 bytes / 32 instructions, zero calls, three branches and two returns.
The constant identity correction is tracked separately in the canonical ledger:
it must not be confused with an arithmetic-scheduling improvement.

## Inspect the texture, not just the score

The macro/intermediate composition restores Step's complete 48-reference
ordered identity stream; the higher-scoring template/intermediate form still
swaps zero and negative-two loads at its nine discriminants. Reference offsets
and the complete instruction streams remain different. Step's first divergence
is still the earlier position/time prologue at +0x13, so this is not closure
of its first wall.

Both intermediate forms produce identical standalone arrival bodies. The first
divergence moves from +0x22 to +0x29. The unresolved 14-byte region has the same
three operations in different orders:

| Retail | Current composed source |
| --- | --- |
| load zero | multiply by negative two |
| exchange ST(0), ST(1) | load zero |
| multiply by negative two | exchange ST(0), ST(1) |

`scripts/test_motion_square_consumers.py` checks the complete arrival bytes
outside that explicitly allowed region, each schedule's exact instruction
shape and raw operand identities, all 48 ordered Step constant roles, and actual
production/original pool payloads. Mutation controls reject changed members,
square/guard instructions, wrong/missing/repeated references and addends.
This does not certify Step's complete arithmetic or make either arrival exact.

## Compose the caller-owned target too

Retail evaluates all nine arrival targets before resolving the acceleration
guard. Its clamp blocks add the clamped step to position; its six bound blocks
load the bound, then pop both that target and the previous position on the
zero-acceleration path. The baseline and all six profiles preceding eager
capture above lack this ordering and have only one bound-path cleanup pop.

Capturing `double targetPosition = (target)` at `ARRIVAL_V` entry, then using it
in the existing `delta`, restores all nine eager evaluations and all six second
cleanup pops. The frame stays eight bytes with no additional QWORD spill; all
48 ordered constant roles remain correct. This explains why the lower 81.0826%
state has useful structural evidence that the 82.3465% intermediate lacks.
Reusing `targetPosition` for the product instead of naming `delta` produces the
same output and does not remove the remaining extra live FP values.

The test also compares the complete nine-target evaluation/guard/cleanup
protocol and rejects removed target loads and cleanup pops on both production
and original sides. It deliberately does not claim the nonzero arithmetic path
or earlier prologue is exact. The by-value target ownership is supported by
retail and the standalone double parameter; the local's name is reconstructed,
not recovered source text.

Reverse-use rule: restore the attested helper first, identify a concrete
remaining evaluation/lifetime discrepancy, verify it was absent from the
baseline, then compose one natural intermediate across the complete family.
No universal preference for either macros or extra locals follows from this
example. Candidate dispositions and reopening criteria belong only to
`reassess-sqr-motion`, `reassess-sqr-motion-zero-referents` and
`reassess-sqr-constref-template-profile` in `config/lithtech_lineage.tsv`.
