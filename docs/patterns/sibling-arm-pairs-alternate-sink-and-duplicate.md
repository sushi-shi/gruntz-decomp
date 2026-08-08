# Two sibling `x == NULL ? fallback : field` pairs: retail SINKS one and DUPLICATES the other — and the choice is made UPSTREAM
tags: cpp:branch cpp:if cpp:local | asm:jmp asm:mov | topic:codegen-idiom topic:tail-merge
symptoms: a `*::ApplyInit`/`*::Setup` that opens with two or three back-to-back `if (desc->m_x == NULL) m_a = m_default; else m_a = desc->m_x;` pairs plateaus in the high 80s / low 90s; `--blocks --diff --lite` shows one pair's arms as `Ni [jcc] / 1i [fall]` (SUNK: one store in the join) and the other as `Ni [jcc] / 3i [jmp] / 1i [fall]` (DUPLICATED: a store in each arm) - and the base has the SAME two shapes as the target but assigned to the WRONG pair
confidence: 9/10
variants: trailing-statement-blocks-arm-tail-sink.md

Do not read the shape of one pair off the spelling of that pair. cl 5.0 decides
sink-vs-duplicate for a pair using state the PRECEDING pair left behind, so the
lever for pair 2 is how you spelled pair 1.

Two source forms are available for each pair and they are semantically identical:

```cpp
// ALIAS form
CDDSurface* a = s->m_targetSurface;
if (a == NULL) { a = m_timerA; }
m_desc04 = a;

// IF/ELSE form
if (s->m_targetSurface == NULL) { m_desc04 = m_timerA; }
else                            { m_desc04 = s->m_targetSurface; }
```

Measured on four functions in `src/DDrawMgr/FaderEffects.cpp` (the residue in each
case was purely these two pairs; every other block already agreed):

| function | retail wants | alias + if/else | if/else + if/else | if/else + alias |
|---|---|---|---|---|
| `CFaderFlat::ApplyInit` 0x17f5e0 | sink, dup | **89.74** (sink, sink) | **100.00 EXACT** | - |
| `CFaderLight::ApplyInit` 0x1804a0 | sink, dup | **92.25** (sink, sink) | **96.12** | - |
| `CFaderSine::ApplyInit` 0x17fe00 | **dup, sink** | 87.09 (sink, sink) | 93.73 | **96.50** |
| `CFaderRadial::ApplyInit` 0x17fa40 | sink, dup | - | already correct | - |

Read the table twice. `CFaderSine` wants the OPPOSITE assignment from the other
three, and gets it from the OPPOSITE spelling — so this is not "always write
if/else". It is: **read the two block shapes off the target, then pick the pair of
spellings that produces them**, four cells, one build each.

The mechanism is the statement-list sink of
[trailing-statement-blocks-arm-tail-sink](trailing-statement-blocks-arm-tail-sink.md)
seen from the other side. Both arms of either form end in a store to the same
member, so the sink is always *available*; what varies is whether the value is
still live in a register when cl gets to the second pair, and an alias temp
upstream is what keeps it there.

## It also runs BACKWARDS — identical LEADING statements get HOISTED

`CPlay::ResetPlayState` 0xd60b0 has

```cpp
if (fm->m_currentMs != 0) {
    fm->m_startStamp.m_v = g_frameTime;   // <-- identical FIRST statement
    fm->m_accum.m_v      = fm->m_currentMs;
    fm->m_baseTime.m_v   = g_frameTime;
    fm->m_running        = 1;
} else {
    fm->m_startStamp.m_v = g_frameTime;   // <-- identical FIRST statement
}
```

and cl hoists those three instructions into the common predecessor, emptying the
else arm entirely (base 35 blocks against retail's 36). The knob is symmetric:
give the arms different FIRST statements. Ladder, one build per cell:

| arm-1 statement order | score |
|---|---|
| `startStamp, accum, baseTime, running` (retail's EMITTED order) | 92.08 - hoisted |
| condition inverted, short arm first | 94.28 - not hoisted, arms swapped |
| `baseTime, startStamp, accum, running` | 96.49 |
| `accum, startStamp, baseTime, running` | 95.90 |
| **`running, startStamp, accum, baseTime`** | **96.62**, 36/36 blocks |

**The emitted order is not the source order.** Retail emits startStamp, accum,
baseTime, running; writing that order is exactly what triggers the hoist. So do
not "fix" an arm's statement order back to the disassembly's order without
re-measuring — the two are different questions.

## Screening

`gruntz sema disasm <rva> --blocks --lite` on BOTH sides and compare the
arm/join triples directly. `--diff` alone will not show it: the instruction totals
are equal, only the placement moves, so the diff reads as a register-colouring
wobble.
