# A guard-proven pointer local needs no defensive initializer
tags: cpp:local cpp:pointer cpp:guard | asm:xor asm:cmp asm:test | topic:codegen-idiom topic:regalloc
symptoms: an optional pointer local is initialized to NULL before a guard, retail
has no corresponding zero store, and the zero value remains live across the
function as a register carrier that changes later compares from `test` to `cmp`
confidence: 9/10

A defensive `T* p = NULL` can change much more than the instruction that writes
zero. If the pointer is consumed only on paths where another object invariant
proves it was assigned, the initializer creates a value that no valid execution
uses. cl 5.0 can keep that zero in a register across the function, changing
register ownership and turning later zero tests into comparisons against the
carrier.

Do not remove such an initializer merely because it appears dead locally. Prove
the complete writer/use invariant across the owning methods:

1. identify every path that can consume the pointer;
2. prove the state predicate guarding those paths;
3. find the method that establishes the pointed-to object under the same
   predicate; and
4. confirm retail has no initializer store before the assignment guard.

`CFaderLight::RenderFrame` (0x180640) is the integration case. Its clear-mode
arm locks `m_overlay` into the local `ovlBits`; `CFaderLight::BeginFade`
constructs `m_overlay` whenever `m_spanCount > 0 && m_clearMode != false`, and
`CFaderLight::Render` returns before using `ovlBits` when `m_spanCount <= 0`.
Therefore every path that consumes `ovlBits` has passed the state under which
`BeginFade` supplied the overlay. Retail has no zero store before the overlay
guard.

Changing `u8* ovlBits = NULL` to the evidence-backed uninitialized local removed
the long-lived zero carrier, eliminated five downstream compare/xor deltas, and
moved the current reconstruction from 89.86% to 90.74% without changing its
15-call, 67-branch, one-return topology or ordered referents.

Negative rule: if any use is reachable without the establishing predicate, or
if object construction and consumption can observe different state, retain the
initializer. The absence of an immediate read is not proof.
