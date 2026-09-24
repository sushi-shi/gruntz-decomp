# Math helper ownership

The math layer keeps value operations in shared headers. Callers normally use the
inline API. Where VC5's inline budget, temporary storage, or evaluation order
requires it, a component macro exposes the same operation at the caller boundary.
The macro is an alternative spelling of the operation; it does not replace the
value type or remove its public API.

| Family | Canonical header | Operations |
| --- | --- | --- |
| Integer coordinates | `Gruntz/CoordNode.h` | construction, indexing, arithmetic, dot/length/distance, absolute value, min/max/clamp, tile conversion |
| Floating vectors | `Gruntz/DoubleVector.h` | float and double 2D vectors, double 3D vectors, arithmetic, conversion, distance, normalization, cross product |
| Tile geometry | `Wap32/TileGeometry.h` | pixel/tile component conversions, tile-size shifts, squared distance |
| Rectangle/point/size expressions | `RectMacros.h`, `MakeRect.h` | ordered stores, dimensions, centers, independent edge offsets, construction |
| Rectangle interpolation | `RectInterpolation.h` | scalar and full-rectangle interpolation |
| Direction offsets | `Gruntz/CardinalDirectionOffset.h`, `Gruntz/DirectionRingOffset.h`, `Gruntz/GruntDirectionOffset.h` | direction-domain-specific coordinate offsets |
| Motion | `Gruntz/MotionInline.h`, `Gruntz/MotionState.h` | arrival velocity, per-axis motion, bounds initialization |
| Random extent points | `Gruntz/RandomExtentPoint.h` | typed selection within an extent |
| Slot/scroll flags | `Gruntz/PlayerSlotFlags.h`, `Gruntz/ScrollEdgeFlags.h` | named flags and slot operations |
| Placement and clipping | `Image/ImageClipMacros.h`, `Wwd/WwdObjMgrInline.h` | image placement, clipping, world-space rectangle translation |
| Text and display geometry | `Gruntz/TextBounds.h`, `Gruntz/VideoModeInline.h` | text bounds and standard-mode checks |
| Grunt movement | `Gruntz/GruntMovementInline.h`, `Gruntz/GruntMovementMacros.h`, `Gruntz/GruntMoveCollisionInline.h` | saved positions, direction/arrival operations, vehicle regions and diagonal collision checks |

Class-owned operations remain members: screen-position accessors belong to
`CResolveNode`, grid/viewport setters to `CDDrawWorkerHost`, tile probes to
`CGameLevel`, and frame lookup/insertion to `CDDrawWorker`. Their shared macro
alternatives live beside the declaration or in that owner's existing macro header.
Pinned retail methods retain real definitions even when callers use a macro.

Native drawing boundaries use `POINT`, `RECT`, and `SIZE` with their SDK widths.
`Coord` remains the game-coordinate value type. The dirty-rectangle blit API also
accepts `Coord`, converting at the native boundary. The base worker's blitter and
the derived surface pair's no-op remain distinct methods.

Macros evaluate arguments at their textual uses. This matters for independent
member reloads and configuration reads: an edge-offset macro with two delta
arguments must not be replaced by one cached delta unless the caller permits it.
Statement macros belong in a braced control-flow body. Component expressions keep
the caller's signedness, arithmetic grouping, and conversion point; use explicit
casts where the retail operation changes width.

For measured examples and the validation procedure, see
[math-helpers-need-macro-evaluation-boundaries.md](patterns/math-helpers-need-macro-evaluation-boundaries.md).
