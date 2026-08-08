# A function-scope `T* p = m_member;` pins a callee-saved register retail never spends

tags: cpp:local cpp:member | asm:mov asm:push | topic:codegen-idiom topic:regalloc
symptoms: extra push ebx/edi in prologue, retail reloads [this+N] at every use, base loads it once
confidence: 9/10

A long method opens by caching one of its own members in a local
(`CDDrawSurfaceMgr* mgr = m_world;`) and then reaches everything through it.
The base obj loads `[this+0xc]` **once** into a callee-saved register and keeps
it live for the whole body; retail re-emits `mov <reg>,[esi+0xc]` in front of
every single use and has one fewer `push` in the prologue.

## Why it reads as noise and is not

The diff looks like pure register renaming, because it is — but the renaming is
*downstream*. The extra long-lived pseudo takes a callee-saved register, which
pushes every other value one slot along, so a whole function's worth of
`eax/ecx/edx/ebx` assignments rotate and a two-line source difference scores as
ten-plus points. The one *structural* tell is the prologue: base pushes a
register retail does not, or spills something retail keeps.

## Fix

Delete the local and spell the member at each use.

```cpp
// base: mov edi,[esi+0xc] once, edi live across the body  -- NOT retail
CDDrawSurfaceMgr* mgr = m_world;
mgr->m_level->VisitVisible(mgr->m_drawTarget->m_backPair, mgr->m_childGroup);
...
mgr->m_workerList->PruneWorkers(...);

// retail: mov eax,[esi+0xc] in front of every use
m_world->m_level->VisitVisible(m_world->m_drawTarget->m_backPair, m_world->m_childGroup);
...
m_world->m_workerList->PruneWorkers(...);
```

The re-read costs nothing at /O2 in scheduling terms — cl re-emits the load into
a scratch register it already needed — so the local buys nothing but the
register pin.

## Not a blanket rule — decide per member

The same function keeps a *different* cached pointer and matches:
`CStatusBarMgr* fx = m_guts;` stays, because retail also loads `[esi+0x2dc]`
once and then reads `[eax]` / `[eax+0x10c]` off it. Read the target: if the
`mov <reg>,[this+N]` appears once, keep the local; if it appears at every use,
delete it. The FIELD reads off the cached pointer are a separate question —
retail re-reads `fx->m_position` twice where cl CSEs it, and no spelling changes
that (see `retail-recomputes-a-shift-we-cse.md`).

## Evidence (2026-08-08)

`CMulti::PumpB` @0xb6e90 **83.38 → 92.36** on deleting `mgr` alone (prologue
`push ebx` disappears, matching retail). A second, independent edit in the same
function — restoring two dead `rc.top`/`rc.left` stores and spelling the SetRect
arguments as direct `g_gameReg->m_modeSize.cx/cy` reads instead of two locals,
which is the same lever at expression scale (retail loads `cx` twice and `cy`
twice) — took it **92.36 → 94.62**.

## Related

- [`loop-bound-local-vs-inline-invariant.md`](loop-bound-local-vs-inline-invariant.md)
  — the loop-scoped form of the same effect (a named bound outbids `this`).
- [`pointer-chain-hoist-intermediate-local.md`](pointer-chain-hoist-intermediate-local.md)
  — the opposite direction: one specific link of an `a->b->c` chain *wants* a local.
- [`one-use-local-is-a-regalloc-knob.md`](one-use-local-is-a-regalloc-knob.md)
  — single-use scratch version; try both directions per site.
