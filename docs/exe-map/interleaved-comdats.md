# Interleaved lone methods = out-of-line member defs in the caller's TU

**Symptom.** A single function sits ALONE inside another unit's contiguous `.text`
block — its RVA neighbours on both sides belong to the same *other* unit. Example:

```
0x110c10  tileswitchlogic   CPlayLevelLoad::LoadPyramidBridge   (0xe3f bytes)
0x111ec0  gruntzmgr2        CGruntzMgr::SetCellHeight  <-- lone, surrounded
0x111f10  tileswitchlogic   CTileMultiTriggerSwitchLogic::ctor
```

We currently model these as their own standalone `.cpp` (`GruntzMgr2.cpp`) so each
src TU maps to one contiguous obj. That is byte-correct but the WRONG structure.

**Proof of owner (caller-adjacency).** Under `/INCREMENTAL` the linker keeps each
obj's COMDATs together, so a method compiled INTO obj X is placed within X's block.
Scan `.text` for the call site (direct `E8`, or via the `/INCREMENTAL` ILT `E9`
jmp-thunk): SetCellHeight's SOLE caller is `CPlayLevelLoad::LoadPyramidBridge`
(0x110c10, **tileswitchlogic**) — the function immediately before it. The linker put
the COMDAT right behind its user because the user's TU is where it was compiled.
Tool: `python -m gruntz.analysis.interleavers` (97 such methods, each with its caller;
`caller == host unit` is the high-confidence signal).

**Reproduction (real MSVC 5.0, `/O2 /Gy`).** Two models produce that placement; a
controlled compile distinguishes them (`scratchpad/comdat_repro`):

| source model | symbols emitted in the obj |
|---|---|
| inline method in a header, small | only the callers — **method INLINED away** |
| method defined **out-of-line in the caller's `.cpp`** | callers **+ the method** (COMDAT, called) |

Retail has SetCellHeight out-of-line at 0x111ec0 AND called → it is the **out-of-line
-in-the-caller's-`.cpp`** case. An inline-in-header method this small is inlined by
MSVC5 /O2 (measured), so it would leave NO 0x111ec0 function. Conclusion: the original
defined `CGruntzMgr::SetCellHeight` out-of-line in the tile source file — a class whose
methods are split across several `.cpp`s, one of them the tile TU.

**Fix recipe.** Dissolve the standalone `.cpp`: move the method's out-of-line
definition into its caller's `.cpp` (in ascending-RVA position among that TU's
functions). Byte-neutral for the method (same `/O2` output); it just changes which
obj owns it. Caveat: the method's class must be fully declared in the caller's TU —
if the caller models the class through a lean VIEW (e.g. tileswitchlogic uses
`CGameRegistry`, the dual-view of `CGruntzMgr`), the move is BLOCKED on that
identity fold first. So SetCellHeight → TileTriggerSwitchLogic.cpp waits on the
`CGameRegistry == CGruntzMgr` unification (R16-flagged).

**Contrast with real splits.** A *run of ≥2* foreign methods, or a lone block at the
BOUNDARY between two units, IS a separate obj → its own `.cpp` (the normal drain).
Only a lone method with the SAME unit on BOTH sides is an interleaved COMDAT.
See `docs/exe-map/holding-tu-drain.md` rule (c).
