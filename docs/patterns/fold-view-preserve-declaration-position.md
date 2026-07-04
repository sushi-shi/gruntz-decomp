# Folding a per-TU view onto its shared header: preserve the declaration position

**Tags:** `topic:tu-layout` `topic:codegen-idiom` `topic:dedup`
**Confidence:** c8 (build-verified: CPlay.cpp / CBankMgr fold)

## Symptom

You de-view a TU by deleting a local view struct that duplicates a shared header
(e.g. `struct CBankMgr { CResSource* Lookup(char*); };` in `CPlay.cpp`, already
canonical in `<Gruntz/CBankMgr.h>` and used by other TUs) and add the
`#include` **at the top** of the file. The fold is layout-identical (zero-field
method-only views, same offsets), yet a *previously-matched* function in the SAME
TU wobbles a fraction of a percent:

```
cplay  ?LoadScrollSpeedOptions@CPlay@@QAEHXZ   86.22 -> 85.92  (Δ -0.30)
```

No exact function is dropped, but the change is **not matching-neutral** — so under
the de-view mandate it must be reverted… unless you keep the declaration order.

## Cause

MSVC 5.0 `/O2` codegen is sensitive to the **source position** of type
declarations in a TU, not just their content. Moving the two struct decls from
their original mid-file spot (~line 2900) up to the include block (~line 70)
reorders the compiler's internal type/symbol tables enough to re-tie one
function's regalloc/schedule. Same root cause family as the fat-`Globals.h`
regression (force-including reshuffles regalloc) — here it is triggered purely by
**declaration position**, even though the folded type is byte-for-byte the same.

## Fix — include the shared header *where the local view was*

Put the `#include` in-place, at the exact line the deleted local view occupied,
so declaration order (and thus codegen) is preserved:

```cpp
// CResSource / CBankMgr are the shared CState bank/source facet - one definition
// in <Gruntz/CBankMgr.h> (also used by CSplashState/BacklogStateLoaders). Included
// here in-place (declaration order preserved) rather than at the top so the fold
// stays codegen-neutral for this TU.
#include <Gruntz/CBankMgr.h>
```

Also delete the now-duplicate `SIZE_UNKNOWN(...)` metadata lines the header brings
(they are `__COUNTER__`-guarded so they never *conflict*, but they should not be
double-counted). Verify with a full `gruntz build`: **no regressions vs baseline**.

## When it applies / doesn't

- The header must be the **already-canonical** shape (included by ≥1 other TU) and
  the local view **layout-compatible** (usually a lighter subset). The clean case
  is a *lone local shadow of a shared header* — not a divergent complementary view.
- A mid-file `#include` is unusual; use it only for **codegen-sensitive** TUs
  (large matched files). In small / lightly-matched TUs a normal top-include is
  often already neutral — test both.
- Does **not** rescue divergent views (different base/ctor/slot-signatures, e.g.
  `CTeleporter` UserLogic-ctor-facet vs Begin/Update-facet) or blob-vs-heavy-header
  folds (`CSymParser`/`CSymTab` pulling a fat polymorphic header) — those change the
  emitted shape/symbol set and must stay per-TU or be reconciled deliberately.

## Evidence

`CPlay.cpp`: top-include → `LoadScrollSpeedOptions` 86.22→85.92 (reverted);
in-place include → identical build (1866/3299 exact, cplay 30/71 unchanged, no
regressions vs baseline). See commit `de-view: fold CBankMgr/CResSource -> CBankMgr.h`.
