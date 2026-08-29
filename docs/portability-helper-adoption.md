# Helper adoption after the matching build

The retail-matching tree remains the source of truth, but a future playable
cross-platform port will no longer use fuzzy percentage as a source-shape
constraint. This file records semantic helper conversions that are valid for
that port even when retail symbol identity, serialized access shape, Win32 call
relocations, or VC5 TU state make them inappropriate today. LithTech lineage
decisions and matching exceptions remain canonical in
`config/lithtech_lineage.tsv`.

## Adopted in the matching tree

- The coordinate-form `PtInRect(const RECT*, int, int)` is the global inline
  owner for the half-open predicate: `left <= x < right` and
  `top <= y < bottom`. The former inferred `CGameLevel::PointInRect` owner was
  removed, while the retail-emitted `CGameLevel::PointInBounds` remains its
  separate out-of-line wrapper.
- `CRandomAmbientSound` stores its play and silence intervals as two
  layout-identical `CRange<i32>` members and uses the surviving
  `Set`/`GetMin`/`GetMax` API.
- Direct half-open tests in `CGrunt::RectContains`,
  `CGrunt::VehicleContactContains`, `CTriggerMgr::UseToyAt`, and
  `CGiantRockLogic::BuildRockBreakInGameText` use the shared predicate. The two
  Grunt controls and the giant-rock control stayed exact in the isolated A/B;
  the complete shared-header migration later rotated current C1 state while
  preserving every historical MAX.

## Port conversions

These are semantic conversions to make when the portable runtime is split from
the retail-matching build:

1. Replace the remaining Win32 two-argument `PtInRect(RECT*, POINT)` calls with
   the project rectangle API. Keep their point-object form distinct until the
   Win32 import boundary is removed.
2. Consolidate the four sequential outside-rectangle returns in
   `TileLogicPump.cpp` through the half-open predicate. Do not convert inclusive
   hit boxes in `Play.cpp` and `ImageSets.h`, or the strict-interior ambient
   regions in `WorldSoundSet.cpp`; those have different boundary semantics.
3. Model `CDDrawWorker::m_minIndex/m_maxIndex` as a closed frame-index range and
   route its public consumers through range accessors. The initial invalid
   sentinel `{99999, 0}` must remain representable.
4. Model `CLogicRecord::m_minX/m_maxX` and `m_minY/m_maxY` as two axis ranges
   while preserving their serialized order. This is a better fit than `RECT`,
   whose field order is different.
5. Consolidate `g_panMinX/g_panMaxX` into a closed pan-delay range after retail
   data-symbol identity and raw save/load addresses are no longer constraints.
6. Consider a dedicated 3D bounds type for
   `CMotionState::m_minBounds/m_maxBounds`; `CRange<DoubleVector3>` has the same
   storage but its by-value accessors are the wrong per-axis API.

Do not convert `CFontConfig`'s low/high scroll thresholds: they are selected by
list-size mode, not endpoints of one interval. Do not convert the reverse-ordered
animation endpoints in `CSBI_ImageSetAni`, interpolation endpoints in
`RezBufferObject`, or interleaved on-disk/file rectangles in WWD and Image
records. `CARange` remains the separately attested Bute double-range type until
that subsystem is deliberately modernized.

`MinAbs` and `MaxAbs` currently have no semantic use site. The source tree has
axis-dominance comparisons in `CMapMgr::LineIsClear` and
`CGrunt::StepGruntMovement`, but neither returns the selected signed operand;
forcing either helper there changes tie handling or hides the actual branch.
Use these helpers in the port only when an operation genuinely selects one of
the original signed values by absolute magnitude.

## Additional surviving math-helper queue

The broader LithTech audit found the following source-backed A/B candidates for
the matching tree. They are also preferred abstractions for the port:

- Blood2 `INTERPOLATE` in `CShadeTableCache::HueRampTable`.
- `CRect`/`CMoRect` normalization and `Width`/`Height` in
  `CWwdGrid::Setup`, preserving the retail `RECT` parameter ABI.
- `DIFF`/`LTDIFF` in dirty-rectangle, pathfinding, and movement-step code.
- `SIGN` inside the existing `SignedStepToward` helper.
- `SQR` at squared-distance sites, including exact controls already using a
  locally reconstructed square macro.
- `IsRandomChance` at direct `rand() % 100 < percent` sites.
- layout-flat vector `Init`, assignment, arithmetic, dot, and magnitude-squared
  operations for `DoubleVector2`/`DoubleVector3`.
- scalar `LTLERP` at the four-field image-clip intersection sites, without
  inventing a vector owner for `ClipVtx`.

For the port, extend those into explicit rectangle, vector, angle-conversion,
array-length, swap, and deterministic random APIs. Preserve inclusive versus
exclusive random bounds and inclusive versus half-open rectangle conventions in
their names. Do not import Lith's macro `FLOOR`/`CEIL`, approximate normalization,
renderer x87 rounding, broken `LTBBox`, fixed 565 packers, or a differently
defined `atan2` merely because a same-named helper survives.
