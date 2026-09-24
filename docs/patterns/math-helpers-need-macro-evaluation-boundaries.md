# Math helpers need the original evaluation boundaries

A useful math API and a particular inline expansion are separate claims. Keep the
coordinate, vector, rectangle, direction, and flag operations when a caller fails
to match; test a shared macro form at the failing boundary. Do not replace the
whole helper family with scalar fields or delete it to recover a score.

The September 2026 integration of PR #72 compared the complete helper family
against the verified `56a2a7afe` tree and retail, using MSVC 5.0 SP3 `/O2 /MT`.
The first pass found both extra out-of-line calls and changed register/lifetime
shapes in previously exact functions. Controlled component-macro trials recovered
the following examples without removing their typed APIs:

| Caller | Inline/aggregate trial | Retained boundary | Macro trial |
| --- | ---: | --- | ---: |
| `CGrunt::ComputeFacing` 0x57060 | 22.1515% | double subtraction, existing scalar locals, `VECTOR2_MAG_COMPONENTS` | 100% |
| `CDDrawWorkerHost::SetTileSize` 0x161f00 | 24.4615% | rectangle stores and `TILE_SHIFT_INTO` loops | 100% |
| `CDDrawWorkerHost::Read` 0x161640 | 72.3841% | component stores and `APPLY_WORKER_HOST_BOUNDS` | 97.3555% |
| `CWwdGrid::Setup` 0x1915c0 | 58.1966% | `NORMALIZE_RECT_COMPONENTS` and size-component macros | 99.9658% |
| `CWwdGrid::Query` 0x1918c0 | 68.8938% | four-exit disjoint and ordered clamp macros | 99.9250% |
| `CWwdGridIter::Init` 0x191b10 | 55.3966% | same disjoint/clamp macros | 95.5000% |
| `CGrunt::RectContains` 0x51850 | 46.2177% | tile component conversion, native rectangle copy, offset/extent macros | 100% |
| `CGrunt::VehicleContactContains` 0x51a20 | 58.6220% | same rectangle family | 100% |
| `CGrunt::SetArrivalTarget` 0x52ed0 | 59.6000% | component stores and snap expressions | 100% |
| `RgbToHsv` 0x14fcc0 | 86.2832% | original nested min/max macros | 100% |
| `CTileTriggerContainer::SetCell` | 93.6029% | player-slot loop macro | 100% |

The same controls restored the affected `CPlay` UI methods, tile-probe callers,
image placement and frame insertion, status-bar geometry, and movement checks.
These are measured caller results, not a claim that every macro spelling is
byte-flat. Small changes to macro grouping and earlier declarations can still
perturb C1 state; see
[macro-origin-can-perturb-later-c1-state.md](macro-origin-can-perturb-later-c1-state.md).

The shared `Coord::Clamp` boundary has a second, larger control. Retail and the
earlier source use component `if`/`else if` clamps in level-tile lookups, while a
`Coord` temporary followed by `Clamp` adds nested `Min`/`Max` candidates. Keeping
the `Coord` API and expanding `CLAMP_PIXEL_TO_PLANE` or `CLAMP_TILE_TO_PLANE` at
the lookup sites gave these real-TU results:

| Caller | `Coord::Clamp` | Component macro |
| --- | ---: | ---: |
| `CPlay::ValidateLevelTiles` | 77.4235% | 89.7939% |
| `CTileTriggerLogic::Tick` | 76.5396% | 90.4371% |
| `CMapMgr::ComputeCellFlags` | 87.5152% | 97.6586% |
| `CPlay::ScanBuildTiles` | 86.201% | 97.930% |
| `CTileTriggerContainer::DeserializeLogic` | 83.5079% | 91.0582% |
| `CTriggerMgr::LoadTileArrivalFx` | 76.8545% | 78.2998% |

The first five recover their earlier caller score or most of its gap; the last
remains dominated by independent sound-call and branch differences. Compiling
all affected TUs also left their other scored functions unchanged. The macro
restores scalar input mutation before coordinate construction, which changes
the inliner candidate set and local census without removing the typed helper.

## Distinguish three different failures

1. **A nested call no longer expands.** Classify with `walls diagnose` and inspect
   `walls inline-model --gap`. A local COMDAT proves eligibility; an undefined
   symbol alone does not disprove visibility. Preserve the inline API and use a
   macro at the measured caller if its expansion restores retail's call set.
   The tile probes, frame insertion, and rectangle containment cases exposed
   this directly.
2. **A temporary changes arithmetic or lifetime.** A vector construction can
   cache both coordinates, change their first-use order, or round an x87 result
   into a `float` member before conversion to integer. A component macro can
   preserve the actual expression and its conversion boundary. Do not call two
   forms equivalent just because their algebra agrees over real numbers.
3. **The proposed helper changes behavior.** Fix the semantic error before
   ranking code generation. A percentage cannot authorize a different receiver,
   signedness, source read, or coordinate calculation.

## Controls that exposed semantic differences

`ComputeFacing` originally subtracts coordinates after conversion to double.
Subtracting integer `Coord` values first changes the overflow boundary. Correcting
that width was necessary but insufficient: a complete typed-vector construction
trial scored 0%, and typed component storage plus the magnitude macro scored
59.6970%. Keeping the scalar local lifetimes and using the magnitude macro closed
the function. The full vector API remains available.

`SetTileSize` uses the width in **both** retail shift loops. The candidate used
height in the Y loop and added a `SetRect` import. The shared loop macro and
component rectangle stores recover the retail behavior, including that asymmetry.

`CDDrawWorkerHost::Read` exposed a separate call-set boundary. Constructing
`CSize` values and invoking `SetRect`/`SetViewportRect` gave 72.3841%. Restoring
the earlier component stores and `APPLY_WORKER_HOST_BOUNDS` while retaining the
typed fields and all helper APIs gave 97.3555%; the retail call set has `CopyRect`
and a second `UpdatePlaneViewRect` where the aggregate form had `SetRect` and
`SetViewportRect`. The sibling `Save` also became exact without a source edit.
In the same TU, `ReadPlaneObjects` recovers its historical score when the packed
record is walked from its `m_fields` array and its coordinates, rectangle edges,
and sprite components are assigned directly. The record still retains its named
semantic fields and fixed layout; the pointer walk expresses the serialized
source order. A prior trial labelled "historical body" was invalid: it copied a
snapshot containing the current body, so its flat score was no evidence against
the pointer walk. The actual old-body A/B raises 70.8123% to 95.0877%.

The WwdGrid family shows why a boolean inline helper cannot always replace a
macro expansion. `WwdRect::Intersects` groups four independent early exits into
one result, changing `Query` and iterator `Init` from retail's five returns to
two. `WwdRect::Intersect` also tests the bounds in a different order. Shared
`WWD_RECT_RETURN_IF_DISJOINT` and `WWD_RECT_CLAMP_COMPONENTS` expand the four
guards and four stores in their measured order at both sites, while the inline
member methods remain available. In `Setup`, `CRect::NormalizeRect` and a
`CopyRect` import are absent from retail; `NORMALIZE_RECT_COMPONENTS` and
`SET_SIZE_COMPONENTS` recover its earlier call set. The macro forms are
byte-identical to the direct historical component controls in the real VC5 TU.

`LoadChipMachineConfig` has two independent edge-offset reads. The second key is
`"(FallingItemSpeed"`, including its opening parenthesis. Computing one offset and
applying it to the whole rectangle changes both the key and read count.
`OFFSET_RECT_Y_EDGES` retains the two evaluations.

The typed `CDDrawSurfacePair::BlitDirtyRect` adapter must preserve the derived
receiver. Retail's method at 0x164650 is exactly `ret 0xc`; dispatching to
`CDrawSubWorker` instead invokes its real blitter. Retyping the three arguments to
semantic references preserves the stack ABI while keeping that no-op identity.
The sprite caller at 0x1508a0 is byte-exact under the corrected fresh label model.
Dirty-rectangle positions use native `POINT` beside their native `RECT` and `SIZE`;
the `Coord` convenience overload remains. Using `Coord` storage introduced an
empty constructor inside the already-nested dirty-state constructor and made
`CGameObject` call that constructor out of line. The native point keeps the full
aggregate and restores `CGameObject` 0x15b390 from 90.7595% to 100%.

A complete candidate link also caught three unavailable MFC bodies: the `CSize`
two-component constructor, default `CRect` constructor, and `CRect::IntersectRect`.
The affected TUs deliberately disable MFC inline definitions. Native `SIZE`/`RECT`
locals, the shared component macros, and the retail Win32 `IntersectRect` call
retain the geometry without adding unavailable SDK member calls. The
`undefined-closure` gate excludes known library classes; it does not replace
this actual link check.

## Reverse-use procedure

- Compare the complete caller and the supplying helper against their prior
  source, then inspect the first instruction and ordered-relocation divergence.
- Keep proven owners, types, layout, serialization order, and public operations.
  Change the expansion boundary before changing the underlying model.
- For coordinate conversions, test component expressions separately from
  aggregate construction and copy. Preserve independent X/Y reads when calls
  between them can change the object or when retail demonstrably reloads it.
- For rectangles, distinguish Win32 calls, native aggregate copies, MFC
  constructors/members, and macro stores. They are separate source hypotheses.
- For FP expressions, preserve source operand order, storage width, and the
  float/double/integer conversion point. An earlier narrow store is a behavioral
  change as well as an instruction difference.
- Rebuild the label model and retail partition after signature changes. A trial
  against the old partition can show a spurious relocation-name penalty.
- Finish with a full build and link. Keep all historical MAX evidence, and reopen
  old source-specific review certificates whose instruction/call claims no
  longer hold.
