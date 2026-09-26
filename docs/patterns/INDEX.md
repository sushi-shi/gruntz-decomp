# MSVC 5.0 pattern reference

Short observations, not a wall database. [Scope and admission rules](README.md).

## Compiler output

- [Template inline eligibility](vc5-template-members-inline-without-inline-keyword.md) — an unmarked template member can expand under /Ob1.
- [Explicit-only template arguments](vc5-explicit-only-template-arguments-collapse.md) — `F<X>()` with `X` absent from the parameters collapses instantiations within a TU.
- [Mixed inline and out-of-line calls](inline-budget-emits-ool-comdat.md) — inspect each call site; symbol presence is not an expansion census.
- [EH frames and lifetimes](eh-frame-presence-is-a-source-fact.md) — unwind records are evidence, not an object counter.
- [Local-static guards](function-local-static-dynamic-init-guard.md) — recognize dynamic initialization without inventing flag globals.
- [Scopes and stack slots](switch-arm-locals-overlay-only-when-scoped.md) — sibling scopes can change stack reuse.
- [Store scheduling](emitted-store-order-is-not-the-source-order.md) — emitted order need not be source order.
- [Constant hoisting before tail merging](constant-hoisting-precedes-tail-merging.md) — a statement duplicated per arm and merged later can hoist a constant that the same statement after the join does not.
- [Translation-unit context](tu-state-probe-family-decides-reachability.md) — unchanged function text can emit different code.
- [Signed remainder](signed-modulo-pow2-abs-restore.md) — sign correction around a power-of-two mask.

## Limits of external evidence

- [Sibling binaries](cross-game-binary-oracle-proves-shared-source-family.md) — shared implementation evidence is not recovered source text.
- [Surviving source and debug objects](surviving-source-lineage-restores-typed-layers-and-order.md) — recover complete families; check revision differences against retail.
