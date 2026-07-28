# /Ob1 inline-budget divergence — the SAME ctor is inlined at one call site and called at another

**Tags:** `topic:wall` `cpp:ctor` `cpp:inline` `asm:call` `topic:eh`

## Symptom

A constructor (or any small member) appears in retail **both ways**:

* it has a real out-of-line body at some RVA (a COMDAT the linker kept), **and**
* some callers `call` that RVA while other callers have its stores expanded inline.

Your reconstruction can only produce one of the two, and which one you get is decided
by where you put the definition:

| definition placement | every call site |
|---|---|
| **in-class** (header) | inline candidate → cl expands it wherever its budget allows |
| **out-of-line** (`.cpp`) | never a candidate → always a `call` |

MSVC 5.0's `/O2` is `/Ogityb1`, i.e. **`/Ob1`**: only `inline`/`__inline` functions and
member functions **defined inside the class declaration** are inline candidates. An
out-of-line definition in the *same* TU is *not* one (unlike `/Ob2`, which VC5 `/O2`
does **not** imply). So placement is a hard, binary switch — and retail's mixed
behaviour is the *inliner's own budget*, not two different definitions.

## Worked example (the wide-object factories, 0x159250 / 0x159440 / 0x159600 / 0x1598d0)

`CGameObject::CGameObject(CDDrawSurfaceMgr*, i32, i32)` has a COMDAT at `0x15b390`.

* `CreateObject_159250` (C kind), `_159440` (F), `_159600` (A) **expand** it whole and
  `call` the sub-object ctors inside it (`0x15b2c0` CResolveNode, `0x15b2a0` grid node,
  `0x15b270` dirty-rect, `??0CString`, `0x15b300` worker).
* `CreateObject_1598d0` (B kind), `CWwdGameObject::CreateObject` (`0x166640`) and
  `CDDrawWorkerHost::ReadPlaneObjects` (`0x162af0`) **call** `0x15b390`.
* Inside `0x15b390` itself all five of those sub-ctors are **expanded** — the same
  functions the factories call.

Consistent reading: every one of these ctors is in-class; MSVC's inliner works with a
per-function accumulated budget, so the *first* expansion at a site consumes it and the
nested ones stay as calls, while in the small standalone COMDAT the budget is fresh.

**Our cl has a more generous budget than retail's build had.** With every ctor in-class
it expands the whole chain and the factories land at 62/59/61/42 %.

## THE FIX — the placement switch is PER-TU, not per-header (2026-07-28)

The table above is not a set of global choices. Inlining is a property of *what the TU
sees*, so a **guard macro** makes it per-TU, and a **`#pragma inline_depth(0)` forcer**
recovers the out-of-line COMDAT of a definition you kept inline. Together they let one
function be inline in the TUs that expand it AND still exist as a labelled COMDAT:

```cpp
// header - inline by DEFAULT (so every TU folds it)
struct WwdRegion : WwdGridNode { WwdRegion(); ~WwdRegion() {} ... };
#ifndef WWDREGION_OOL_CTOR
inline WwdRegion::WwdRegion() { m_object = 0; }
#endif
```

```cpp
// the TU whose retail call sites emit a CALL: declaration only -> it emits calls
#define WWDREGION_OOL_CTOR      // ... and it supplies `RVA(...) WwdRegion::WwdRegion() {...}`
```

```cpp
// the TU that must FOLD it but also OWNS its standalone COMDAT: keep it inline and
// force the copy out. inline_depth(0) makes this one reference non-inlinable; the
// emitted COMDAT still folds ITS own callees normally.
static void* volatile g_forceEmitSink;
#pragma inline_depth(0)
void ForceEmitWwdRegionCtor() { g_forceEmitSink = new WwdRegion; }
#pragma inline_depth()
```
and pin the forced body by mangled name, since it has no definition to hang `RVA()` on:
`RVA_COMPGEN(0x0015b2b0, 0xe, ??0WwdRegion@@QAE@XZ)`.
(The forcer device is `src/Gruntz/UserLogicCtorEmit.cpp`'s; the guard is
`USERLOGIC_OOL_CTOR` in `<Gruntz/UserLogic.h>`.)

Applied to this cluster: `wwdobjmgr` guards `WwdDirtyRect`/`WwdGridNode`/`CResolveNode`/
`AnimWorkerObj` (its factories call all four), `wwdgameobjectrender` + `levelplane` guard
`CGameObject` (their retail sites `call 0x15b390`), and `wwdfactoryobject` guards
`CGameObject` + `WwdRegion` — it supplies those two bodies (0x15b390 / 0x15b2b0) while
forcing out `0x15b2c0` / `0x15b300`. Result: **`0x15b2b0` and `0x15b390` recovered as
labelled functions** (`0x15b2b0` EXACT, `0x15b390` 91.1 %), `??0CResolveNode` @0x15b2c0
60.1 → **EXACT**, `0x166640` 42.4 → 71.7, `0x162af0` 69.9 → 75.0, `_159250`/`_159440`
still EXACT.

## What is still not fixable

Two demands genuinely conflict **inside one TU**, and only there:
`_159250`/`_159440` expand `WwdRegion::WwdRegion()` while `_159600` calls it, and three
of `wwdobjmgr`'s four factories expand `CGameObject::CGameObject` while `_1598d0` calls
it. A guard is per-TU, **MSVC 5.0 has no `noinline`** (`__declspec(noinline)` is VC7+),
and `#pragma inline_depth(n)` is depth- not size-based, so neither can split call sites
*within* a TU. `_159600` stays at 95.7 % and `_1598d0` at 57.0 % for that reason.

## The budget is an EXPANSION COUNT — and when it runs out, cl prunes the DEEPEST

`0x15b390`'s own body needs ~12 nested expansions (`CResolveNode` → `CLoadable` →
`CWapObj` → `CObject`, the `WwdDirtyRect` live + shadow records, `WwdRegion` →
`WwdGridNode`, and `new AnimWorkerObj(...)` with its own 4-deep `CLoadable` chain).
cl 5.0 runs out one short. **Proven, not guessed**: deleting the
`new AnimWorkerObj(...)` statement (probe only) makes the pruning disappear;
`#pragma inline_depth(16)` and `(255)` do **not** move it — the limiter is the
expansion count, not the depth — and MSVC 5.0 rejects `__forceinline` (C2501).

**What it prunes matters more than that it prunes.** Left alone, cl drops the deepest
expansion, `??0CWapObj@@QAE@XZ`, and emits it out-of-line — which drags a
`??_7CWapObj@@6B@` COMDAT into the obj. That vtable is `VTBL_ABSENT` in retail, so it
is a **FATAL `vtbl-absent` violation**, not just a % dip: the failure mode of an
exhausted budget can be a build-integrity break, not a diff row.

The steer is to hand cl the pruning decision by guarding a *shallow* expansion
instead. `#define WWDREGION_OOL_CTOR` in that TU frees two slots (`WwdRegion` + its
`WwdGridNode` base), `??0CWapObj` folds again, and the residue is one
`lea ecx,[esi+0x9c]; call ??0WwdRegion` where retail has three stores: **88.7 % with
the CWapObj prune and a failing gate → 91.1 % with a passing one.**

## Related

* `docs/patterns/base-trio-in-ctor-body-misplaces-vptr.md` — where the vptr stamp lands.
* `docs/patterns/rezalloc-placement-new-no-eh-frame.md` — the wall this work retired:
  `RezAlloc` + placement-new emits no `/GX` ctor-in-flight frame; a real `new T(...)`
  does. Superseded for the wide-object factories.
