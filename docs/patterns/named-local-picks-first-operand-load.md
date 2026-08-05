# `a + b` over two member loads is CANONICALISED — a named local for the FIRST operand picks the load order
tags: cpp:local cpp:member | asm:mov asm:add asm:imul | topic:codegen-idiom topic:regalloc
symptoms: base and retail differ ONLY in which of two `mov reg,[obj+off]` comes first, everything else byte-identical; swapping the source operands is a byte-identical no-op; 99.9x% with one two-line hunk
confidence: 9/10

For `x = a + b` where **both** operands are plain member/field loads, cl5 /O2 emits

```asm
mov  eax,[obj+offA]
mov  ebx,[obj+offB]
add  eax,ebx
```

and the choice of which offset lands in the `mov` is **canonicalised** — it does not follow
source order. Writing `b + a` instead of `a + b` produces *byte-identical* output (measured
both ways on four functions). That is why this row is repeatedly filed as a wall.

The lever is a **named local for the operand you want loaded FIRST**:

```cpp
// NO - canonicalised, cl loads t->m_extent.right (the higher offset) first:
i32 limit = t->m_screenX + t->m_extent.right;

// YES - `sx` is a statement of its own, so its load is emitted first:
i32 sx    = t->m_screenX;
i32 limit = sx + t->m_extent.right;
```

Three spellings of the local all work — `i32 sx = A; x = sx + B;`, the compound form
`i32 x = A; x += B;`, and two locals `i32 a = A, b = B; x = a + b;`. A local on the
**second** operand does **nothing** (measured): it is the first operand that has to become
its own statement.

**Scope and non-scope.** This is about the *load order of two memory operands*, so it also
shows up on `imul`. But the lever is not universal: `CDDrawWorkerHost::Save`'s
`m_gridW * m_gridH * 4` would not move for **any** of six spellings (both orders, one and
two named locals, the `*=` chain, `(w*h) << 2`) even though a standalone replica of the same
expression *does* flip with the operand order — there the pick was a function of the TU's
cumulative optimizer state at that site and it closed only when an unrelated body earlier in
the TU changed. So: try the named local first (it is cheap and it is the common case); if the
expression refuses to move at all, the pick is TU-state, not source.

## Evidence

`src/Wwd/GameLevelMove.cpp` (2026-07-29) — `CGameLevel::ResolveRightX` / `ResolveLeftX` /
`ResolveBottomY` / `ResolveTopY`, all four parked at **99.98%** with the identical two-line
hunk (`mov eax,[t+0x13c]; mov ebx,[t+0x5c]` where retail has `mov eax,[t+0x5c];
mov ebx,[t+0x13c]`). Operand swapping changed nothing; the named local took all four to
**100% EXACT**. `CDDrawWorkerHost::CenterScrollB`'s `(right + left)` hunk closed the same way.

## The lever runs BOTH ways — removing a local can flip the operand order too

The same knob decides which register a *derived* value ends up in, and therefore which way
round a later commutative operator is written. `CStatusBarMgr::DrainGauge` @0x105780 is a
three-line clamp:

```cpp
// NO  - the named difference lands in edx, cl emits `and edx,eax` / stores edx
i32 v = m_gauge - delta;
m_gaugeTarget = v > 0 ? v : 0;

// YES - written inline the value stays in eax: `and eax,edx` / stores eax  (retail)
m_gaugeTarget = m_gauge - delta > 0 ? m_gauge - delta : 0;
```

98.50 -> 100 EXACT. Inverting the ternary (`v <= 0 ? 0 : v`) was byte-identical, which is
the usual tell that the residue is an operand-order canonicalisation rather than a
comparison-polarity one. So when a commutative operator is transposed, test BOTH
directions of the local: introduce one on the operand you want first, and remove any local
that is holding the value you want in the accumulator.

related: derived-value-local-forces-in-place-arith.md, commutative-operand-order-is-canonical.md
