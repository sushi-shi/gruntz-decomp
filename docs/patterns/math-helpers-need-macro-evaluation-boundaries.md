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
