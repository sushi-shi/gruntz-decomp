# Does MSVC 5.0 `/Gy` cause `.text` function scatter? — measured

**Question.** Gruntz's retail `.text` is heavily *scattered*: a source file's functions rarely
form one contiguous run. We claimed this is an **irreducible `/Gy` (function-level COMDAT)
linker-placement effect**, not our source mis-grouping — and that the `scatter_core` dashboard
(which hides `??` special-members + init-fragments + shared inline/template COMDATs + hand-flagged
`@interleaver`/`@orphan`) shows the *true* per-TU layout. This experiment tests that empirically,
because we had been asserting it. Two runs under the real wine **MSVC 5.0** toolchain
(`CL`/`LINK` v5.10.7303): a controlled synthetic program, then real code (LEGO Island / `isledecomp/isle`).

## `/O2` forces `/Gy` (measured)

Counting `.text` sections in a probe `.obj`:

| flags | `.text` sections | `/Gy` |
|---|---|---|
| `/O2` | 8 (each `IMAGE_SCN_LNK_COMDAT`) | ON |
| `/O2 /Gy-` | 8 | **still ON — `/Gy-` is a no-op** |
| `/O1` | 8 | ON (also forces it) |
| `/Od` | 1 (no COMDAT flag) | OFF |

`/O2` and `/O2 /Gy-` produce byte-identical EXEs except **1 byte** (PE `TimeDateStamp`). Only `/Od`
disables COMDATs, so the clean same-source A/B isolation pair is `/Od` (`/Gy` off) vs `/Od /Gy`
(on); the placement result is opt-invariant (`/O2` matches `/Od /Gy`). Retail Gruntz is `/O2` → `/Gy`
is on and cannot be turned off. (The repo's older note said only `/O2` forces it; `/O1` does too.)

## Synthetic run — 80 TUs, ~2200 fns, categories tagged in the mangled name

Per-TU **fragments** (maximal contiguous runs in the linked `/MAP`; 1 = contiguous):

- `UNIQ`/`XREF`/`VIRT`/`CTOR` = **1 fragment, 100% contiguous in every config** — `/Gy` makes
  **zero** difference to a TU's own unique body. Cross-TU callers are *not* pulled toward callees.
- The **only** movers are forced-COMDATs (`??_G`/`??_E` deleting dtors, COMDAT even without `/Gy`)
  and **shared inline/template** functions, which the linker **folds to one copy** (COMDAT dedup —
  MSVC 5.0 has no `/OPT:ICF`) placed at their **first-referencer**, so they appear inside a TU they
  don't belong to (never *interspersed* in another TU's body — they sit at the block edge).

## ISLE run — real code validation

Compiled **181/230 TUs** of LEGO Island under wine MSVC 5.0 at `/O2` (`/Gy` on) and `/Od` (off),
linked each into a real EXE with full `/MAP`. **3057 functions, 173 objs, 302 classes** — genuine
inheritance, vtables, heavily-shared inline math (`Vector2`/`Vector3`/`Matrix4`, `MxPresenter` family).

| median frags / % perfectly contiguous | `/O2` (`/Gy` ON) | `/Od` (`/Gy` OFF) |
|---|---|---|
| per-TU, ALL functions (by owning obj) | **1 / 100.0%** | **1 / 100.0%** |
| per-TU, unique core body (pooled excluded) | 1 / 59.9% | 1 / 74.3% |
| per-class, unique (non-special, non-shared) | 1 / 61.7% | 1 / 83.5% |
| shared COMDATs (`i`-flag inline/template/deduped) | 1339 = **44%** | 3526 = **67%** |
| `??` specials (of which deleting dtors) | 989 (563) | 1774 (640) |

1. **Every TU is 100% contiguous — one `.text` block — under `/Gy` both on and off (173/173).**
   The linker keeps each TU's whole footprint together; `/Gy` never scatters TUs relative to each
   other. Clean confirmation of the synthetic result on real code.
2. **The scatter is the shared/special COMDAT set, and it is enormous in real code:** 44% (`/O2`)
   to 67% (`/Od`) of *all* functions are `i`-flag shared COMDATs, plus 563–640 deleting dtors. Real
   inline-heavy classes displace to their first-referencer exactly as predicted (`Vector2`: 20 inline
   methods → 3 fragments across 2 owner objs).
3. **After excluding the pooled set, the unique core body is median 1 fragment.** Because each TU is
   100% contiguous as a block, all of a TU's unique fragments live *within its own region*; the
   residual (60% perfectly contiguous at `/O2`) is intra-TU interspersing with the TU's own
   dtors/inlines, never cross-TU scatter.
4. **`/Gy`'s marginal effect is modest, expected direction:** `/Od` gives higher unique-body
   contiguity (74%/83%) than `/O2 /Gy` (60%/62%) — per-function COMDAT placement interleaves ~15–20%
   more of a TU's own methods with its own COMDATs. Median stays 1 either way. Cross-TU displacement
   of shared inlines happens under both (it's COMDAT dedup, not `/Gy`); `/Od` has *more* (no inline is
   inlined away).

## Conclusion — the working claim, scoped precisely

`/Gy` does **not** scatter a TU's unique methods; the linker lays each object's `.text` down as one
contiguous run. What COMDAT placement moves is only the **forced-COMDAT (`??_G`/`??_E`) + shared
inline/template** set, which dedups and lands at its first-referencer — appearing in a TU it wasn't
written in. In real code that set is **44–67% of all functions**, so it dominates the apparent
scatter. Gruntz's whole-binary scatter = (this irreducible pooled/shared displacement) **+** our
analysis attributing those folded copies to the class's nominal TU rather than the obj where they
landed.

**This validates `scatter_core`'s `is_pooled()` predicate on ground truth:** excluding `??`
special-members + deleting dtors + shared inline/template COMDATs is exactly what's needed —
*necessary* (it's most of the functions) and *largely sufficient* (median-1 unique body). The
real-world residual (intra-TU interspersing) is what the hand-annotated `@interleaver`/`@orphan`
flags mop up. It also explains why **mass eviction was the wrong tool**: the "interlopers" in a
scattered file's span are the whole program between two of that file's own pooled COMDATs, not a
foreign cluster to move.

## Reproduce

Wine MSVC 5.0 from `nix develop .#build`. Scripts + maps were kept in the session scratchpad
(`gy-exp/`: `gen.py`/`build.py`/`analyze.py` synthetic; `isle_build.py`/`isle_analyze.py` on a
`isledecomp/isle` clone). Method: generate/clone → `cl /c /O2 /MT` and `/c /Od` → `link /MAP` →
per-TU fragments metric (same as `docs/exe-map/scatter.py`) on each `.map`, split by category.
ISLE build note: 49/230 TUs didn't compile — only 9 are truly missing SDK (`subwtype.h`); the rest
are minor MSVC-5.0-vs-4.20 dialect frictions. 79% is ample for the statistics.
