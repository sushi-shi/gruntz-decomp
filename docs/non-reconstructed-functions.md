# Confirmed non-reconstructed functions

This is the conservative implementation worklist for functions whose compiled body is
demonstrably missing a substantial part of the retail body. It is not a list of every
function below 100%: a complete reconstruction can remain non-exact because of source
shape or compiler state.

The current worklist is empty. The 2026-08-01 audit reports zero functions below 60%
whose complete compiled body contains fewer than 60% of the retail instructions, and
zero complete bodies below 60% of a reliable retail prefix.

## Current completeness census

The 2026-08-19 derived census has 4,367 reconstruction-target bodies. Of those,
3,595 are currently exact and 772 are currently below 100%. Historical MAX separates
the latter into 56 bodies already proven exact under an earlier compiler state and 716
that have never reached exact. These 716 are matching walls, not 716 missing functions:
the source bodies exist, and the missing-body criterion above still finds no truncated
body. `gruntz walls inventory` is the authoritative worklist. Its 992 displayed rows are
the 772 ordinary bodies plus 220 EH-band funclets, which are scored but are not separate
reconstruction targets.

`gruntz verify status --all` also reports three known-absent baseline rows. They are
retired source labels, not missing retail code: two names were formerly attached to
file-scope `CRect` dynamic initializers, and the third to the initializer of
`s_gruntDirSpare`. Their retail bodies remain attributed through `RVA_DYNINIT` on the
owning data.

The live census is reproducible without maintaining another ledger:

```sh
gruntz walls inventory
gruntz verify status --all
rg -n "@stub|@identity-TODO|@early-stop" src include
```

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
to bodies that were still missing logic. The current audit has 673 live markers, no
marker on an exact function, and one understood unmapped marker: the inline `CPlay`
constructor is defined in `Play.h` but its retail copy is emitted and pinned in
`gruntzmgr`. A future exact match must have its stale marker removed.

Joining unique marker RVAs to the 716 never-exact reconstruction targets parks 662
complete bodies and leaves 54 unparked candidates. Marker occurrences cannot be
subtracted directly from the target count because a few bodies carry duplicate markers
and some live markers sit on current dips that have already reached 100% historically.
The unparked set is derived from the same inventory/Model join, ordered by historical
MAX; its first row is currently `CStaticHazard::CStaticHazard` at 89.78%.

There are 35 `@identity-TODO` occurrences, split into three different evidence queues:
20 incremental-thunk-oracle annotations, five functions whose original TU owner is not
yet proved, and ten unresolved semantic identities. The semantic identity audit reduced
16 markers to those ten. Existing xrefs resolved the filename
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

## Data completeness is a separate audit

The live retail-side access sweep has 27,272 references over 2,278 claims. Its gate is
green: no accessed datum is currently proved to have a wrong width, stride, count,
extent, adjacency, or source-vs-IAT identity. The only accepted high-confidence finding
is `g_panTable`: retail deliberately indexes backward from its one-element symbol into
the adjacent 100-element volume table, so enlarging it would model two retail objects as
one.

This does not mean all retail data has been named. The claim-side coverage sweep finds
3,490 uncovered interior ranges totalling 145,834 bytes. They include linker/compiler
tables, static-library territory, zero-filled contribution gaps, and padding. Of that
surface, retail code touches 634 nonzero/pointer bytes and 571 zero/padding bytes, but no
touched nonzero/pointer gap lies between two claims of the same live source unit; that is
why `gruntz verify data-coverage --gate` is green. Cross-unit and library-frontier rows
remain an attribution worklist, not evidence that an adjacent source object should be
invented.

The latest [linked-image snapshot](image-diff.md) adds the coverage view that per-object
matching cannot: 271 retail data regions had no candidate symbol. That dated number must
not be mixed with the live access-map findings; it includes unreferenced and library-owned
regions. Refresh the candidate link before treating it as a new measurement. The
reproducible live audit is:

```sh
gruntz verify data-access --build
gruntz verify data-access --gate
gruntz verify data-access --findings
gruntz verify data-coverage --gate
gruntz verify data-coverage --tsv
```

Therefore the next data campaign is coverage attribution at the cross-unit/library
frontier, while the next function campaign is discovery of never-carved retail code plus
the 716 derived walls. Neither campaign should convert its raw gap count directly into
fabricated declarations or function bodies.
