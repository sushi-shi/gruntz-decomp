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
