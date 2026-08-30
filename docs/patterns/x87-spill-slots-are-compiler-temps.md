# x87 spill slots are compiler temps, but the authored value-lifetime DAG steers them
tags: cpp:local cpp:float cpp:scope | asm:fld asm:fstp asm:fild asm:sub-esp | topic:regalloc topic:codegen-idiom
symptoms: integer scaffolding, CFG, calls, and FP mnemonic counts agree while
`[esp+N]` x87 operands are permuted; a dense expression was flattened instead
of preserving its named product intermediates
confidence: 10/10
variants: local-slot-order-is-declaration-order.md, od-local-slot-ordering.md,
folded-local-frame-slot-roles.md, local-declaration-order-steers-commuting-instructions.md

An enregistered `double` does not own a fixed source frame slot. It lives on the
x87 stack and only gets an `[esp+N]` home when C2 spills it, so the visible slot
is a compiler temp. That does **not** make source locals inert: their distinct
value lifetimes define the FP DAG which C2 schedules and spills.

The old version of this pattern incorrectly parked
`CBoomerang::AdvanceMotion` at 71.52 after testing declaration order only inside
one flattened-expression family. Four named rotation-product values exposed the
missing source layer and reached 99.95349; hoisting the `sin` result's declaration
to function scope then selected retail's spill coloring and reached 100% exact.

## Detection signature

Use this sequence only after `gruntz walls diagnose` reports
REGALLOC/SCHEDULING and the following all agree:

* byte extent, instruction count, call set, CFG, returns, and relocations;
* FP, displacement, store, immediate, and mnemonic multisets;
* the ordered referent sequence and all general-purpose register roles.

If the first divergence is an x87 stack operation, read the value DAG rather
than assigning semantic meaning to stack offsets. Ask whether one dense pair of
expressions erased independently named products or results.

## Controlled closure

Retail rotates two coordinates and advances a phase. The flattened reconstruction
had the correct arithmetic but reduced each axis immediately:

```cpp
m_posX = m_originX + (vy * s - vx * c);
m_posY = m_originY + (vx * s + vy * c);
m_phase += amp * m_velScale;
```

That source scored 71.5194. A same-game rotation family established the real
`s`, `c`, `vx`, `vy`, and `phaseDelta` census. Using members as the two result
temporaries restored retail's `0x1de` extent, 132 instructions, six calls, seven
branches, two returns, nine relocations, and every semantic multiset, raising the
function to 92.4031. A fresh 64-state mixed TU campaign was flat, proving that
the remaining schedule was inside the body.

Four explicit product lifetimes were the missing layer:

```cpp
double xSinTerm = vy * s;
double xCosTerm = vx * c;
double ySinTerm = vx * s;
double yCosTerm = vy * c;
m_posX = xSinTerm - xCosTerm;
m_posY = ySinTerm + yCosTerm;
```

This made C2 retain all four products before the reductions, exactly as retail,
and raised the score to 99.95349. The only residue was a paired spill-home
permutation (`vx`/`vy` and `sin`/`cos`). Declaring `double s;` at function scope
and assigning it after the early-exit logic changed C1's local-handle order;
VC5 then emitted the retail slot map and the function became byte-exact. Semantic
product names reproduce the same exact object, so no codegen-shaped identifiers
are required.

## Reverse-use heuristic

Do not reorder source declarations in hopes that `[esp+N]` directly follows
declaration order; it does not. First restore the authentic value census and
lifetimes: independent product locals, result locals, and the scope in which a
value is declared. Recompile after each structural layer and compare from the
first real divergence. Only after the DAG, topology, counts, and referents agree
should declaration scope be used to select the final spill coloring.

A flat declaration-order or TU-state search proves only that the current source
family cannot reach retail. It is not evidence that x87 source locals are inert.
