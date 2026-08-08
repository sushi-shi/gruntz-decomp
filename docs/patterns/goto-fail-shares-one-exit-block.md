# `goto fail;` is the ONLY spelling that shares SOME exits and duplicates the rest
tags: cpp:branch cpp:goto cpp:return | asm:xor asm:jcc asm:ret | topic:codegen-idiom
symptoms: base has MORE ret blocks than target, DUP-EXIT, retail's leading guards `je` a shared exit while ours fall through to their own copy, missing `xor eax,eax` in a guard exit, elided xor, cross-jump, tail merge, exit block placement
confidence: 9/10

cl 5.0 /O2 has exactly **three** exit-merging regimes, selected by the SOURCE
construct that reaches `return 0;`. Nothing else moves them - not `/GX`, not body
size, not jump distance, not statement order.

| source | result |
|---|---|
| separate `if (c) return 0;` | **no merging.** One inline copy per site, each the fall-through of its own inverted `jcc`. |
| `goto fail;` … `fail: return 0;` | **partial.** The goto sites share ONE block; every OTHER `return 0` in the function keeps its own copy. |
| `\|\|`, `&&`, a positive-gate nest, or `if (a) goto L; if (b) goto ok; L:` | **total.** EVERY same-valued `return` in the whole function collapses into ONE block, sunk past the last instruction. |

Retail's guarded `Init`/`Build`/`Setup` functions are the *partial* regime, so
transcribing them as separate `if`s leaves us with 2-4 surplus `ret` blocks, and
"fixing" it with `||` overshoots to one. `goto fail` is the only spelling that lands
in between. Where the shared block ENDS UP is also mechanical: cl inverts the LAST
goto's branch and makes the shared block that branch's fall-through, so the guard you
send to `fail` last decides its position.

```cpp
// NO - six ret blocks; retail has four
if (host == NULL)  return 0;
if (owner == NULL) return 0;
...
if (key == NULL)   return 0;
...
if (tbl == NULL)   return 0;

// NO - one ret block; the || swallows the key guard too (88.49 -> 81.75)
if (host == NULL || owner == NULL) return 0;

// YES - the leading pair and the LAST guard share one block, `key` keeps its copy
CObject* found;                 // hoist: a goto may not skip an initialisation (C2362)
CDDrawWorker* tbl;
if (host == NULL)  goto fail;
if (owner == NULL) goto fail;
...
if (key == NULL)   return 0;    // deliberately NOT a goto - retail duplicates here
...
if (tbl == NULL)   goto fail;
...
return cel != NULL;
fail:
    return 0;
```

**The `xor eax,eax` elision is a CONSEQUENCE, not the cause.** cl drops the `xor` from
an exit block that has ONE predecessor and reaches it with the tested value already in
`eax` (`if (p == NULL) return 0;` on a value in eax lowers to `pop esi; ret`). That
makes the copy 2 bytes, which is what tips cl into duplicating rather than jumping - so
the elision and the duplication are the same decision seen twice. Give the block a
second predecessor with `goto fail` and the `xor` comes back on its own; there is no
separate lever for it, and none of `int z = 0; return z;`, `(BOOL)0`, `FALSE`, `NULL`,
a `volatile` local, a named result local, `== 0` instead of `!`, or an intervening
store changes it (all seven measured on a `if (!m_p) goto fail; if (!m_p->IsLoaded())
goto fail; ... fail: return 0;` probe).

Screen: `python -m gruntz.audit.exit_merge_sieve --dup` (base ret count > target's).
27 more candidates tree-wide after the four below.

Measured 2026-08-08:
`CSBI_ImageSetAni::Init` 0xe7980 88.49 -> 96.43 (rets 6->4, exits byte-exact),
`CSBI_StatzTabGruntBar::BuildMultiplayerTabStatusBar` 0xea1f0 88.56 -> 94.68 (10->8),
`CSBI_GruntMachine::BuildResourceTabStatusBar` 0xe8a70 85.91 -> 93.17 (7->5),
`CDDrawSubMgrPages::TransEnter` 0x158e40 88.21 -> 97.31 (6->5, one `xor` left).
`CGameInfo::SetNames` 0x118040 already used this spelling and is EXACT - it is where
the recipe was found, not invented.

**The mirror direction is NOT solved.** When retail has MORE exits than we do
(`exit_merge_sieve --over`, 47 functions) our source has a `||`/`&&` guard collapsing
the others, and there is no spelling that keeps one solo `return 0` while a *sunk*
shared block exists: the goto form always hoists its block in front of the body
instead of past the success return. Measured on `CSBI_MenuItem::SetupImage` 0xe80e0
(92.17 -> 84.24 with goto, reverted) and `CDDrawSurfacePair::InitFromSurface` 0x163db0
(77.50 -> 56.74, reverted).

`/Os` and `/O1` DO enable a real machine-level cross-jump pass (a 6-ret probe collapses
to 1), so the pass exists - but it is all-or-nothing and everything else about the
codegen changes: `#pragma optimize("s", on)` around `CSBI_ImageSetAni::Init` scored
88.49 -> 45.07. Not a lever.

related: [trailing-error-block-is-a-crossjump-magnet.md](trailing-error-block-is-a-crossjump-magnet.md) (the wall this replaces), [one-shared-return-tail-is-a-positive-gate-nest.md](one-shared-return-tail-is-a-positive-gate-nest.md) (the TOTAL-merge regime, when retail really has one tail), [shrink-wrapped-prologue-needs-one-tail-return.md](shrink-wrapped-prologue-needs-one-tail-return.md) (the prologue half of the same layout decision)
