# `volatile` on a member reproduces retail's preserved redundant member-store

**Confidence: 7/10** · tags: `cpp:member` `cpp:ctor` | `asm:mov` | `topic:codegen-idiom`

## Symptom

Retail writes the *same* member offset twice (a dead store followed by the live
one) with no intervening read — or writes one member three times to the same
value — and **keeps every store**. A straight reconstruction (`m_30 = 0; …; m_30 =
this;` or three `m_20 = X; m_38 = -1;` pairs) is dead-store-eliminated by our
MSVC5, dropping the redundant writes and cascading the constant materialization /
register choice (e.g. one `mov edx,0x80000000` reused for `-1` instead of a held
`or eax,-1`).

```
; retail (CRemusReadStream init, 0x1396f0)
mov [eax+0x1c], offset vtbl
mov [eax+0x30], ecx        ; m_30 = 0   <- DEAD, but kept
mov [eax+0x34], ecx
mov [eax+0x10], ecx
mov [eax],     ecx
mov [eax+0x30], eax        ; m_30 = this
```

## Cause

These come from the devs hand-writing redundant resets (or an inline `Clear()`
folded in). **The same toolchain** kept them in retail, so it is a *source-shape*
difference, not an optimizer-version one: our DCE only fires because the plain
`m = a; … m = b;` is provably dead. Marking the member `volatile` makes every
access an observable side effect — DCE can't drop it, and (crucially) volatile
accesses are emitted **in program order**, so the redundant stores land exactly
where retail put them.

## Fix (STEERABLE)

Make *only* the redundantly-written member(s) `volatile`; keep the value literal
so the constant still hoists (`mov edx,0x80000000` once, reused):

```cpp
struct C1396f0 {
    void* volatile m_1c;  // pin order: m_1c first
    void* volatile m_30;  // 0 then `this` — volatile keeps the dead store
};
m_1c = &g_vtbl; m_30 = 0; m_34 = 0; m_10 = 0; m_0 = 0; m_30 = this;  // -> 100%
```

## Caveats

- Mark the *minimal* set. Over-volatiling neighbours (e.g. the worker reset's
  `m_78`/`m_5c`) pins THEM in source order too but leaves the truly-redundant
  pair floating between the volatiles, and can move the constant materialization
  — it regressed `CDDrawWorkerA::Reset` 90%→72%. Volatilize the duplicated
  offsets, leave the rest plain.
- Inline-helper folding (a `Clear()`/`resetTimer()` member called N times) does
  **not** survive DCE — our cl inlines then eliminates. Only `volatile` holds.
- Distinct from [[dead-global-read-spill-dce]] (a non-steerable *global* read
  whose dead spill our DCE drops and whose value-register cascades): there the
  problem is a load+spill, here it is pure stores and `volatile` cleanly wins.

## Evidence

`C1396f0::Init` (0x1396f0) 75%→**100%** with `m_1c`/`m_30` volatile.
`CDDrawWorkerA/B::Reset` (0x1570d0 / 0x157240): the three redundant
`m_20`/`m_38` timer-reset pairs reproduce with `m_20`/`m_38` volatile (90%, the
residual is the float of the *non*-volatile `m_78`/`m_5c` — a scheduling wall).
