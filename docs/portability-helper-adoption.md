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

## Matching A/B results for the surviving math-helper queue

The complete queue was tried against retail. A current-score movement is not a
MAX failure: every retained or reverted trial below preserved historical MAX.
The decisions are based on source ownership and retail instruction topology.

Retained in the matching tree:

- Blood2's `INTERPOLATE` expression in
  `CShadeTableCache::HueRampTable`, with the loop's ratio cached once. The
  isolated function moved from 97.4468 to 98.1560, reached retail's exact
  extent and instruction count, and moved its first divergence from `+0x5a`
  to `+0x130`.
- The surviving `SQR` macro at every genuine squared-distance use, including
  the former local `SQUARE` family. Exact controls remained semantically flat;
  shared-header C1 movement elsewhere preserved every MAX.
- Blood2's prototype-plus-inline `IsRandomChance`, retaining the historical
  `char`/`DBOOL` result width, at the two direct `rand() % 100 < percent`
  sites. The new abstraction moved current compiler islands but preserved MAX.
- The surviving `ROUND` helper at both health-display conversions. Their
  current scores moved from exact to 98.2353 and 99.1304, while MAX 100 stayed
  banked; this is accepted source restoration, not a regression.
- `DoubleVector2::Init` and `DoubleVector3::Init` across the coherent vector
  initialization family. This includes `CMotionState` bounds initialization,
  `CMovingLogic`'s uniform maximum step, and the proven SpotLight sites. The
  shared declaration rotates several current constructor states, but all MAX
  values hold.
- The scalar `WwdRect::Init(left, top, right, bottom)` boundary at the six
  spatial-manager rectangle constructions. `CWwdGrid::Setup` separately uses
  one fixed-size `memcpy` from its layout-identical Win32 `RECT` parameter;
  that preserves the whole-object copy while eliminating the former
  overlapping `RECT` union view.
- In `CSpotLight::Tick`, the first one-step vector-helper spelling descended
  from 84.2097 to 81.6411. Composing the authentic helper boundary with two
  named rotated-coordinate locals returned to 84.2097. This is a concrete
  example of a lower helper-shaped island exposing the next source-shape fact.

Audited but not imported into the matching tree:

- `CMoRect` normalization in `CWwdGrid::Setup` was a valid trial and MAX held
  throughout. It was not removed because of a score or bank failure. All three
  member/order spellings kept a larger branch/temporary topology than retail
  and failed to produce retail's stack home, while the baseline already had
  exact calls, CFG, extent, and instruction count. MFC `CRect` is also excluded
  because its out-of-line `NormalizeRect` call is absent from retail. A
  portable rectangle owner remains preferred once retail code shape no longer
  constrains the implementation.
- `DIFF`/`LTDIFF` would introduce branches where retail uses the branchless VC5
  integer-absolute-value sequence. `SIGN` would add subtraction and
  multiplication around the already attested `SignedStepToward` branch shape.
- `LTLERP` would share an interpolation ratio, but retail's image clipper emits
  three independent divisions in each pass. The current scalar expressions
  preserve that authored/evaluated topology.
- A vector assignment operator makes `CMotionState::Step` copy the three fields
  contiguously, while retail interleaves the copy with the time-step x87 work.
  This site therefore retains memberwise statements.
- SpotLight's candidate `operator+=` reloads freshly stored members instead of
  consuming the retained rotated values. The candidate `operator-` in the
  surviving LTVector2 family returns through a two-argument constructor; adding
  that constructor is incompatible with Gruntz's proven anonymous aggregate
  use under VC5. An adapted constructor-free operator added five instructions
  over retail, so neither adapted operator is claimed as surviving Gruntz
  source.
- `Dot` and `MagSqr` have no vector-owned Gruntz consumer. The apparent sites
  belong to scalar fields, `Coord`, or `ClipVtx`; introducing temporary vectors
  would invent an owner rather than restore one.
- `TRect2<T>::Offset` is semantically useful but belongs to the vector-backed
  rectangle family, not every Win32 `RECT`. NOLF itself translates its plain
  `LTRect` instances with four explicit additions. Introduce a portable
  rectangle translation operation only after those Win32 fields acquire one
  real portable owner; do not graft the method onto the matching types now.

For the port, reconsider every semantically valid item in the second list when
the retail compiler constraint is removed. Extend them into explicit rectangle,
vector, angle-conversion, array-length, swap, and deterministic-random APIs.
Preserve inclusive versus exclusive random bounds and inclusive versus
half-open rectangle conventions in their names. Do not import Lith's macro
`FLOOR`/`CEIL`, approximate normalization, renderer x87 rounding, broken
`LTBBox`, fixed 565 packers, or a differently defined `atan2` merely because a
same-named helper survives.
