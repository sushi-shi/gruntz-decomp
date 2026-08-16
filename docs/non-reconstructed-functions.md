# Confirmed non-reconstructed functions

This is the conservative implementation worklist for functions whose compiled body is
demonstrably missing a substantial part of the retail body. It is not a list of every
function below 100%: a complete reconstruction can remain non-exact because of source
shape or compiler state.

The current worklist is empty. The 2026-08-01 audit reports zero functions below 60%
whose complete compiled body contains fewer than 60% of the retail instructions, and
zero complete bodies below 60% of a reliable retail prefix.

The instrument that produced it is retired (see [tooling-map](tooling-map.md));
its criterion is reproducible per function from `gruntz walls diagnose <rva>`,
which prints both sides' instruction counts and byte lengths.

The thresholds intentionally favor false negatives over false positives. A zero result
means no function meets this mechanical proof of a missing body; it does not prove that
every reconstruction is semantically complete. There are currently no `@stub` markers.

## Truncated switch-table exclusions

The audit names rows it cannot classify because linear disassembly enters embedded
switch-table data in the compiled object. The current two exclusions were reviewed
manually and are not short bodies:

| RVA | Function | Retail bytes | Compiled bytes | Disposition |
|---|---|---:|---:|---|
| `0x000810f0` | `CGruntzMapMgr::LoadAttributes` | 2228 | 2735 | Full tile-grid, attribute-switch, neighborhood, and exit-trigger passes are present; the compiled sparse switches are larger than retail. |
| `0x0006a060` | `CGrunt::LoadGruntMovingDeathConfig` | 1085 | 1568 | Both level-family direction switches and all movement outcomes are present; the compiled sparse switches are larger than retail. |

These remain matching work, but neither is evidence of omitted logic. Any newly excluded
row still requires manual control-flow and size review before the missing-body worklist
can be called empty.

## Marker state

`@early-stop` and `@identity-TODO` are state markers, not completeness evidence. The
early-stop cleanup removed stale exact markers, duplicate markers, and markers attached
to bodies that were still missing logic. After the current reconstruction pass, the
stale-marker audit has 857 mapped sub-100% markers and no exact or unmapped marker. A
future exact match must have its stale marker removed.

The identity audit reduced 16 markers to 10. Existing xrefs resolved the filename
parameter of the page-image resolver, the `CEyeCandyAni` action-table owner, the
boomerang dispatcher registration, the `CMenuSparkle` action receiver, and the
RTTI-backed `CSplashState` methods. The remaining markers have no present evidence path:

- three concern original TU ownership without a file anchor or contribution boundary;
- two concern static template/source names absent from all surviving symbols;
- three are zero-reference orphan owners without RTTI, allocation, or construction;
- one is an untyped `CGruntzMgr` field seen only in initialization and deletion;
- one is a callback-object class absent from RTTI, allocation sites, and the factory;

Those markers should be revisited only when new evidence appears; inventing identities
from proximity or behavior would not resolve them.

When the missing-body audit finds a function, reconstruct its full semantics before
attempting permutations. An `@early-stop` is appropriate only after the full logic is
present and the remaining non-exact result meets the proof contract in
[comment-markers.md](comment-markers.md).
