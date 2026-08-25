# An inline macro must preserve a caller local even when direct member reads are equivalent

tags: cpp:inline cpp:macro cpp:local cpp:struct | asm:mov asm:push | topic:codegen-idiom topic:regalloc
confidence: 9/10
symptoms: folding a repeated caller block into a macro keeps the call set and CFG,
but several callers lose score; the old block copied a small aggregate to a local
and passed its fields, while the macro reads those fields from the source object
directly

## Finding

Do not erase the local while abstracting the block. Make the local part of the
macro expansion:

```cpp
#define COMMIT_GRUNT_NEIGHBOR_COPY(target, coord)                                                  \
    Coord coord = target->m_lastTilePx;                                                            \
    CommitNeighbor(target->m_playerIndex, target->m_unitIndex, coord.m_x, coord.m_y)
```

The direct-field form is valid only for callers that originally had no copy.
The local is a C1 lifetime/register-allocation input even though C2 can source
the same four call arguments without it.

## Evidence

The source-wide `CommitNeighbor(target)` reconstruction covered 29 same-target
blocks. A representative direct site in `StepScrollGruntBehavior` was byte-identical,
and replacing one copied-`Coord` site in `StepHitAndRunnerBehavior` changed only an object-local
label ordinal. That local A/B was not a sufficient control: the full build moved
seven functions below their current fingerprints:

- `StepPostGuardBehavior`: 86.5443 -> 80.9747
- `UpdateArrival`: 91.3944 -> 89.3169
- `StepDumbChaserBehavior`: 83.9097 -> 81.7118
- `StepObjectGuardBehavior`: 79.8309 -> 77.9113
- `StepMagicWandGruntBehavior`: 88.9729 -> 88.1828
- `StepSmartChaserBehavior`: 94.7843 -> 93.7943
- `StepHitAndRunnerBehavior`: 87.8616 -> 85.8899

Those seven functions contained all 18 old `Coord c = target->m_lastTilePx`
sites. Switching only those sites to the copy-preserving macro restored every
fingerprint and returned the full build to zero fresh regressions; the eleven
originally-direct sites kept the direct macro.

Reverse-audit rule: when a helper/macro sweep regresses only the callers that
previously materialized a small aggregate, preserve that materialization inside
the abstraction before trying register or statement-order variants.
