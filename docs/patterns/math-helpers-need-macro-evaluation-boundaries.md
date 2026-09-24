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
| `CFaderLight::RenderFrame` 0x180640 | 74.8360% | scalar `FADER_DISTANCE` in its inline `Render` callee, separate old-span locals | 90.8721% |
| `CMinimap::Draw` 0xa3820 | 40.1090% | scalar center/scale locals, component rectangle stores and `SET_RECT_COMPONENTS` | 76.3782% |
| `CMinimap::DrawBorderRaw` 0xa3a20 | 74.0460% | separate width/height locals and `RECT_WIDTH`/`RECT_HEIGHT` | 96.7931% |
| `CGrunt::StepGruntMovement` 0x4c170 | 66.4355% | earlier CFG and tile-center macros, with typed `BrickzCell` reads | 76.5600% |
| `CGrunt::FinalizeStep` 0x5ecd0 | 72.9091% | separate direction/move/next locals and existing sort-key macro | 96.0983% |
| `CGrunt::StepCompassMove` 0x51c00 | 54.8401% | scalar `CanCommitMove` tile locals and `RETURN_IF_DIAGONAL_ROUTE_BLOCKED` | 63.0219% |
| `CGrunt::StepHitAndRunnerBehavior` 0xed9f0 | 79.4196% | restored screen-position, recycle, random-extent macros and scalar tile locals | 88.6250% |
| `CPlay::OnKeyDown` 0xcbcc0 | 85.9785% | scalar bookmark, cursor, viewport, and tile expressions at the call sites | 90.3267% |
| `CTileTriggerSwitchLogic::SwitchDown` 0x110570 | 80.7471% | branch-local tile/pixel scalars and tile-center macro | 93.5977% |
| `CTileTriggerSwitchLogic::SwitchUp` 0x1106b0 | 83.2759% | same local scalar and macro boundary | 93.5977% |
| `CTileTriggerLogic::ApplyMove` 0x112590 | 68.24% | branch-local tile scalars and tile-center macro | 97.1496% |
| `CTileActionEvent::BreakTopBrick` 0x112ee0 | 79.3173% | tile-center expressions at each branch and two local pixel-pair macros | 97.9808% |
| `CTriggerMgr::FindNearestUnitForPlayer` 0x77f80 | 76.8772% | scalar tile and distance locals | 89.5263% |
| `CTriggerMgr::HudRect` 0x78060 | 63.5772% | scalar rectangle offset and Win32 `SetRect` | 90.2439% |
| `CTriggerMgr::RemoveCellRecord` 0x78260 | 69.8960% | component identity checks | 93.5840% |
| `CTriggerMgr::ApplyGruntAreaEffect` 0x7b930 | 74.9078% | scalar radius, bounds, per-Grunt tiles, and branch-local positions | 94.5674% |
| `CTriggerMgr::LoadGruntResurrectTuning` 0x7be60 | 61.0939% | scalar tile/pixel locals and Win32 `POINT`/`RECT` | 91.9724% |
| `CTriggerMgr::SpawnGrunt` 0x7c110 | 83.5938% | snapped pixel-pair and pickup macros | 94.3750% |
| `CGrunt::LoadGruntDeathAnimations` 0x60150 | 91.4915% | restored snap macro and scalar screen locals with typed tile reads | 97.1129% |
| `CGrunt::TryTeleportToCell` 0x52fb0 | 85.8178% | snapped pixel-pair, recycle, and tile-center macros with typed cell writes | 93.6778% |
| `CPlay::SaveUnderAndDrawCursor` 0xd0b30 | 67.9018% | scalar cursor and direct RECT locals | 99.9632% |
| `CPlay::HandleDragMove` 0xd0db0 | 78.0153% | scalar drag clamp and world-position locals | 97.4138% |
| `CPlay::ExecuteCommand` 0xd1b60 | 76.1651% | scalar target and component-store order with named flags | 80.9374% |
| `CPlay::ExpandViewport` 0xd8ed0 | 48.5000% | direct RECT and SIZE operations | 91.2653% |
| `CMoviePlayer::Configure` 0x17cfc0 | 79.3618% | direct origin and destination RECT stores | 99.9246% |
| `PolyIsConvexCW` 0x145e30 | 83.3153% | separate scalar FP deltas and cross-product locals | 98.1982% |
| `ImageRotateBlit` 0x145f60 | 53.7126% | scalar image, source-rectangle, and vertex position locals | 79.4671% |
| `ImagePolyClipRect` 0x1461b0 | 92.8471% | four scalar edge locals and direct vertex component stores | 99.2814% |
| `WarpTextureBlit` 0x146a20 | 68.6506% | scalar texture coordinate and step locals | 68.6703% |
| `CInGameIcon::PeekCycle` 0x984b0 | 71.3802% | scalar screen tile locals with typed cell flags | 93.7190% |
| `CInGameIcon::Reposition` 0x98a90 | 59.9549% | scalar screen tiles and typed cell writes | 97.4286% |
| `CInGameText` constructor 0x99110 | 94.0138% | object snap macro | 97.0138% |
| `CInGameText::Update` 0x997c0 | 94.1317% | scalar screen position locals | 96.7066% |
| `CGruntCreationPoint` constructor 0x3e520 | 55.3022% | original object snap macro with typed screen-position fields | 81.9712% |
| `CVoiceTrigger` constructor 0x119b50 | 63.8167% | object snap macro and direct area-edge stores | 92.5083% |
| `CStatusBarMgr::UpdateFallingItemStatusBar` 0x107590 | 35.8545% | four scalar edge locals and direct native `RECT` stores | 97.9091% |
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

The light-fader caller gives a direct call-set control. Replacing four scalar
`FADER_DISTANCE` expansions in its inline `Render` body with `Coord::Mag` added
three `Coord::Dot`/`__ftol` call pairs to `RenderFrame`; retail has none. Restoring
the distance macro raises the caller from 74.8360% to 90.1292% and the emitted
`Render` from 90.9443% to 98.8538%. Separating the old span's start and end
locals instead of constructing a `CRange<i32>` raises the caller to 90.8721%.
The `Coord` and `CRange` APIs remain in the shared headers. The same TU's exact
`CFaderSine::GetFrameCount` moves to 99.5% after the inline change despite
unchanged source and call set; reversing its operands or introducing a local
does not restore its earlier compiler state. Its historical exact MAX remains
the correct record of that source shape.

The minimap has two more local controls. `Draw` gained four nonretail calls
(`CRect::MulDiv`, `CRect::InflateRect`, `OffsetRect`, and `SetRect`) when scalar
center/scale locals and rectangle component stores became CPoint/CSize/CRect
operations. Restoring its earlier local census and component stores recovers
the retail call set and raises 40.1090% to 76.3782%. `DrawBorderRaw`'s single
`CSize`/`CRect` temporary replaced separately timed width and height locals;
restoring those locals raises 74.0460% to 96.7931%. `SET_RECT_COMPONENTS`,
`RECT_WIDTH`, and `RECT_HEIGHT` are byte-flat against direct component stores
in the controlled VC5 unit. The typed classes stay available elsewhere, and
the unchanged palette siblings' dips do not move with these caller fixes.

`StepGruntMovement` has the same 25 calls and 99 relocations in both current and
retail objects, but the branch and return skeleton differs. Restoring the
complete earlier body, including its tile-center macro calls and local/exit
order, recovers the earlier 76.5600% from 66.4355% without changing any other
scored function in its owner TU. The two historical flat-array cell reads map
to `BrickzCell::m_flags` and `m_occupantId`, preserving the typed 28-byte cell
layout. This is a complete source-shape control; replacing only arithmetic
fragments cannot prove the CFG.

`FinalizeStep` has equal call, branch, and return counts in both objects, but
the rewrite's aggregate `DoubleVector2` direction/position and `Coord` next
position change the FP local homes and store order. Restoring the earlier
scalar local census in both movement arms, while using the current typed
members, recovers 72.9091% to 96.0983%. The existing
`SET_SORT_KEY_IF_CHANGED` macro remains at the call site. The shared vector
and coordinate operations are retained for their other users.

`StepCompassMove` exposes a nested call-budget boundary. The new
`DiagonalRouteBlocked` inline function remains a valid helper, but its four
`Coord` temporaries and `CellFlagsAt` calls leave one out-of-line call in the
large caller that retail lacks. Restoring scalar tile locals and pointer-based
directional checks inside the existing `CanCommitMove` inline member removes
that call and raises 54.8401% to 63.0219%, beyond the pre-rewrite 62.65%.
Wrapping the four checks in `RETURN_IF_DIAGONAL_ROUTE_BLOCKED` is byte-flat
against their direct spelling in the VC5 unit. The helper function and the
`Coord` API remain defined; the macro chooses the caller's expansion and early
return structure.

`StepHitAndRunnerBehavior` has the same source-shape issue across several nested
helpers. The new typed helpers remain defined, while the caller uses the earlier
macro expansion where retail has the corresponding `PtInRect` and node-pool
call paths. Restoring the complete caller body raises 79.4196% to 88.6250%;
the residual is measured against that recovered base.

`OnKeyDown` is a large caller where several small aggregate rewrites collectively
change VC5 evaluation order. Restoring the earlier scalar bookmark and cursor
locals, rectangle checks, and tile expressions raises 85.9785% to 90.3267%
without changing the shared coordinate APIs. The same-unit
`LoadScrollSpeedOptions` body and source hash are unchanged, yet VC5 moves its
current score from 97.4950% to 97.3700%; its historical MAX remains banked.
This is a caller-order compiler-state perturbation, so the recovery is judged
by the edited caller and the unchanged function retains its best evidence.

The four tile-switch methods show the lifetime effect directly. Hoisting a
`Coord` and applying `TileCenter` before the branch tree retained the same
call and control-flow counts but shifted register allocation from the first
instruction. Restoring branch-local scalar tile coordinates and the existing
`DECLARE_TILE_CENTER_PIXEL_PAIR` macro recovered each method to its earlier
score while keeping the typed `m_tile` member and all coordinate helpers.

Six TriggerMgr methods confirm the same rule across CFG and call-set walls.
Their aggregate rewrites changed the source-visible local census, point and
rectangle call identities, or the branches from component tests. Restoring
the earlier scalar and macro boundaries recovers each prior score; the typed
fields and all shared math helpers remain in place.

`LoadGruntDeathAnimations` retains typed `BrickzCell` reads while restoring
the earlier object snap macro and scalar screen locals. The restored caller
recovers its prior 97.1129% and the retail nested animation call set, showing
that the tiled data model and caller macro boundary can coexist.

`TryTeleportToCell` restores one branch by keeping scalar tile and snap-pixel
locals at their earlier call sites. The existing recycle and tile-center macros
can expand beside typed `BrickzCell` flag/occupant writes and named cell flags;
the source restores its prior 93.6778% while retaining every shared helper.
The same-unit `StepCompassMove` body is unchanged but moves from 63.0219%
to 62.6506% under this compiler state. Its historical 63.0219% MAX stays
banked, and the whole-engine score rises.

Four Play callers retain the typed coordinate and rectangle APIs while
restoring direct scalar and Win32 struct operations where retail's call set
and local lifetimes require them. `SaveUnderAndDrawCursor` removes surplus
`SetRect`/`CopyRect` calls; `HandleDragMove` reaches 97.4138%, above its
pre-rewrite 93.58%. In the same owner unit, unchanged
`LoadScrollSpeedOptions` returns from 97.3700% to 97.4950%, demonstrating
that a neighboring authentic source restoration can recover C1 state.

`CMoviePlayer::Configure` recovers its earlier 99.9246% by assigning the
typed origin point's components and destination rectangle fields directly.
The aggregate APIs remain defined; removing the extra `SetRect` call restores
the retail call set.

The image polygon unit shows that scalar FP lifetimes matter even without a
call-set difference. Restoring distinct delta, clip-edge, and vertex component
locals raises three methods substantially. `WarpTextureBlit` changes only
68.6506% to 68.6703% in the current TU state, so its pre-rewrite 74.45%
remains historical headroom rather than a claim of present recovery.

The InGameIcon unit retains typed `BrickzCell` fields and the same shared
coordinate helpers. Scalar screen tile locals remove the extra
`CUserLogic::GetScreenPos` calls in `PeekCycle` and `Reposition`; the existing
object snap macro and scalar screen locals recover both InGameText methods.

Two placement constructors show the same expansion boundary. Replacing the
object snap macro with a `Coord` copy and setter adds a branch in
`CGruntCreationPoint`, although its call and relocation counts stay equal.
Restoring the macro recovers its previous 81.9712% without changing the named
flag or typed screen-position storage. `CVoiceTrigger` also needs four direct
area-edge stores in place of aggregate `Coord` rectangle arithmetic. That
restores its previous 92.5083%; the diagnosed reciprocal call-target difference
between `BuildLogicTypeTable` and `RegisterLogicTypesOnce` disappears from the
constructor's changed C1 state. Other source methods in both units are flat.

`UpdateFallingItemStatusBar` gives a direct call-set control: an aggregate
`CRect` copy, member offset, and Win32 `SetRect` add three calls absent from
retail. Four edge locals and direct native `RECT` stores restore the earlier
97.9091% while the coordinate and rectangle helpers remain available to other
callers. The later chip-grinder update also becomes exact in the local TU
comparison, consistent with a changed VC5 compiler state.

`CMovingLogic::InitOwner` gives a counterexample to adding an aggregate merely
because the component values are related. Its four min/max record reads have
distinct local lifetimes and conditional stores into the typed vectors. A pair
of `Coord` temporaries followed by `DoubleVector3::Init` preserves the values
but changes the earlier inline helper's C1 state. Restoring the four scalar
locals and stores leaves both affected `CGrunt`/`CProjectile` constructors at
their current scores, yet raises `CGrunt::StepArrivalDrop` from 0% to 60.3640%
and makes three other functions exact in the full VC5 build. A named macro
wrapping the same statements was byte-flat in the three direct consumer TUs,
so the simpler direct spelling is retained. The coordinate and vector APIs
remain available. This is a TU-wide optimizer-state effect: inspect the full
owner family after changing an inline header even when the intended caller is
flat.

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
