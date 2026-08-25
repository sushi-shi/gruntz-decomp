# The "dead 8-byte `Coord` temp" IS a source local: an UNNAMED by-value accessor result

tags: cpp:local cpp:struct cpp:temporary | asm:sub asm:mov | topic:codegen-idiom
symptoms: retail's whole frame is `sub esp,0x8`, written by exactly two `mov [esp+k],reg`
stores and **never read**; our base has no frame at all, so every `[esp+N]`, the register
assignment and one callee-saved `push`/`pop` all differ
confidence: 10/10
supersedes: this file's own 2026-08-08 and 2026-08-10 verdicts, which said no source
construct reaches the frame

**THE PREVIOUS VERDICT WAS WRONG AND IS RETRACTED.** This file said the two stores were
"a spill pair the allocator emitted and then never needed", rated a source-level fix
0/10, and told three lanes to hand the rows to the permuter. They are a **source local**:
the unnamed temporary of a by-value accessor, read one call per field at the use site.

```cpp
// no frame at all - cl deletes an ordinary local's field stores
this->ApplySwitch(cell, cell->m_lastTilePx.m_x, cell->m_lastTilePx.m_y);
Coord pt = cell->LastTilePx();          // ALSO no frame - naming it is the defeat

// retail: sub esp,0x8, both field stores emitted, both reads folded back to the member
g_gameReg->m_triggerMgr->HandleTargetSelection(cell->LastTilePx().m_x, cell->LastTilePx().m_y, ...);
```

The precondition - that the result must never be **named** - is the whole content, and it
has its own file: [by-value-accessor-must-be-an-unnamed-temporary.md](by-value-accessor-must-be-an-unnamed-temporary.md).
It is why the 2026-08-10 pass recorded "a by-value inline accessor `CGrunt::LastTilePx()`
added to `Grunt.h` for exactly this purpose ... cl elided the local": that cell named the
result, and so did every other aggregate cell in the 38-probe campaign below.

## The controlled A/B (2026-08-23, `CTriggerMgr::HandleActionOptionsPointer` 0x7b1b0, one build)

| spelling | insns | size | frame |
|---|---|---|---|
| retail | 102 | 0x12b | 0x8 |
| `cell->LastTilePx().m_x`, `.m_y` (unnamed) | **102** | **0x12b** | **0x8** |
| `Coord pt = cell->LastTilePx();` then `pt.m_x` | 96 | 0x11a | none |
| `Coord tmp = cell->LastTilePx();` then `tmp.m_x` | 96 | 0x11a | none |
| `cell->m_lastTilePx.m_x`, `.m_y` (direct member) | 96 | 0x118 | none |

Three of this file's own rows closed on it, in one commit:

| fn | rva | before | after |
|---|---|---|---|
| `CTriggerMgr::ToggleToolTargeting` | 0x7d450 | 85.71 | **100.00 EXACT** |
| `CTriggerMgr::HandleActionOptionsPointer` | 0x7b1b0 | 91.19 | 99.95 |
| `CTriggerMgr::UnregisterUnit` | 0x79fb0 | 86.48 | 94.19 |

## What is still true, and it is the useful half

**The lever is SITE-SPECIFIC. Do not sweep it.** Ten further single-site conversions,
each picked because the function reads `p->m_lastTilePx.m_x/.m_y` through a pointer, were
measured on the same instrument and **every one was flat or worse**: `StepDumbChaserBehavior` 0xef6b0,
`UpdateArrival` 0xf0130, `StepObjectGuardBehavior` 0xf1c70, `StepMagicWandGruntBehavior`
0xf8240, `StepSmartChaserBehavior` 0xf42f0, `StepHitAndRunnerBehavior` 0xed9f0, `GruntInRadius` 0x67b00,
`TmDeflectStep` 0x6f2f0, `FindNearestUnitForPlayer` 0x77f80, `FindNearestEnemy` 0x77df0.
`StepScrollGruntBehavior` 0xf2b20 and `ClaimSwitchTile` 0x52c70 likewise did not move, and on
`ClaimSwitchTile` the receiver is `this`, where the temp CSEs with the other member reads
and never reaches a home.

**So the entry condition is the FRAME, not the source text.** Convert a site only where
the pair is already proven:

    retail frame == ours + 8   AND   the extra 8 bytes carry two stores that are never read

`gruntz walls diagnose` prints both frames; a sweep of the sub-100 queue for
`retail frame > ours` is what found all three rows here (45 rows carry that signature,
509 agree).

## The old negative evidence, which still stands for the forms it tested

38 probe cells under `/O2 /MT /GX /GR` deleted the local: copy-init, field-wise,
address escaped via `this`, a user copy ctor, a by-value inline param, a user
`operator=`, in-place construction, a local array, writing through a pointer to itself, a
union, aggregate init, `*(double*)&p = ...`, declare-then-assign, an inline out-parameter
accessor, a by-value struct argument to an out-of-line callee, and passing by value to an
inline that uses it. **Every one of them names its local**, which is exactly why they all
died; they do not test the unnamed form and they say nothing about it.

Three constructs keep the frame for the wrong reason and are still not the answer: a real
out-of-line call taking `&p` (emits a `call` retail lacks), a user-declared destructor
(drags in the `/GX` EH frame), and `volatile` (forcing a store is its definition). A user
copy ctor additionally does not build - it makes `Coord` non-POD and `GruntWanderStep.cpp`
then fails C2362, which is weak evidence retail's `Coord` was a POD.

TU declaration count is dead flat here: 50 island cells per function on `UnregisterUnit`,
`ToggleToolTargeting` and `StepPostGuardBehavior` were 50/50 identical, and a 16-cell sweep
above `TriggerMgr.cpp`'s first include moved none of the unit's 21 sub-100 functions. That
remains correct and is the reason the axis was mis-read as "unreachable" rather than
"reachable, on an axis nobody had tried".

## Reverse-audit signature

A `bounded`/`wall` verdict whose evidence is "N probe cells all deleted the local" is
only as wide as the cells. Check whether every cell shares a property the target does not
have - here, a NAME on the temporary. A negative that large is a statement about the
search axis, not about the binary.

related: [by-value-accessor-must-be-an-unnamed-temporary.md](by-value-accessor-must-be-an-unnamed-temporary.md),
[coordinate-pair-read-order-picks-the-preserved-register.md](coordinate-pair-read-order-picks-the-preserved-register.md),
[shrink-wrapped-prologue-needs-one-tail-return.md](shrink-wrapped-prologue-needs-one-tail-return.md)
