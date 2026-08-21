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

`@early-stop` and `@identity-TODO` are inherited state markers, not independent
completeness evidence. The current mechanical audit has 708 live early-stop markers,
no stale exact marker, and no unmapped marker; the header-inline `CPlay` constructor is
joined by its qualified source name. This proves marker ownership and currency only. It
does not prove that an inherited early-stop decision was correct.

Codex therefore uses `gruntz walls inventory --todo` for its explicit campaign queue.
The queue starts from all 700 never-exact, non-EH reconstruction targets and subtracts
only reviews Codex personally recorded as `bounded` or `exact` in
`config/codex_wall_reviews.tsv`. Each review is keyed by RVA and the MAX ledger's exact
function source hash, so editing the function makes the review stale automatically.
`exact` means the attempt reached 100%; `bounded` means the exact attempt exhausted its
current evidence-backed source/compiler axes below 100%. An `open` review stays in the
queue with its class and next evidence-bearing action.
Inherited early-stop markers do not remove anything from this queue.

There are 34 `@identity-TODO` occurrences. Twenty-four mark zero-reference leaves whose
body and ABI are bounded but whose original semantic name cannot be recovered; two of
those also retain an incremental-link thunk proving only external linkage. The remaining
ten are structural identity questions with no present evidence path:

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

## Never-claimed code is a small, separate queue

The current same-file interior-gap scan finds 90 gaps containing 6,958 bytes after
trimming leading and trailing `0x90`/`0xcc`. The scan must sort all `RVA`,
`RVA_COMPGEN`, and `RVA_DYNINIT` claims together; omitting the owner-side dynamic-init
pins manufactures 42 false gaps in the static-initializer bands.

Eight repeated compiler/runtime bands account for 5,611 bytes. The remaining 82 gaps
contain only 1,347 bytes: 23 are five-byte `E9` thunks, 15 are trivial returns of at
most eight bytes, and 44 contain another 1,171 bytes of accessor, forwarder, destructor,
or still-unclassified code. This is the high-confidence discovery queue for retail code
that has no claim. It is not evidence for 82 missing source functions: compiler-generated
copies may need only `RVA_COMPGEN`, and each of the 44 substantive rows still needs raw
disassembly, caller, and emitted-object review before reconstruction.

The 44-row address-dossier pass narrows that queue further. Two rows are already known
static-library bodies despite having no source claim: `CRect::CRect` at `0x00029ac0`
and `CRect::SetRect` at `0x0008c380`. The other 42 rows have no admitted census identity.
Some contain multiple padding-separated tiny bodies, so 42 is a discovery-row count,
not a function count. A 32-byte table at `0x00074518` is an embedded switch-table
false positive and is excluded from the 90-gap total.

## Data completeness is a separate audit

The live retail-side access sweep has 27,272 references over 2,278 claims. Its gate is
green: no accessed datum is currently proved to have a wrong width, stride, count,
extent, adjacency, or source-vs-IAT identity, and it has no accepted exceptions. The
former `g_panTable` exception was a false split: the three operands at `0x253c48` are
`g_volumeTable + 100`, inside the proven 101-entry volume curve.

This does not mean all retail data has been named. The claim-side coverage sweep finds
3,490 uncovered interior ranges totalling 145,834 bytes. They include linker/compiler
tables, static-library territory, zero-filled contribution gaps, and padding. Of that
surface, retail code touches 634 nonzero/pointer bytes and 571 zero/padding bytes, but no
touched nonzero/pointer gap lies between two claims of the same live source unit; that is
why `gruntz verify data-coverage --gate` is green. Cross-unit and library-frontier rows
remain an attribution worklist, not evidence that an adjacent source object should be
invented.

The touched surface is small enough to state precisely. Nine `POINTER` and three
`NONZERO` gaps account for the 634 meaningful touched bytes, and all twelve cross a
unit or library frontier. Two cross-frontier `ZERO-GAP` ranges account for another 563
touched bytes: 9,352 bytes between `g_appResHandle` and `g_dinputLogEnabled` at
`0x0025161c`, and 5,376 bytes between two header-inline random-number statics at
`0x002c128c`. Five accepted padding ranges account for the final eight touched bytes.
There is therefore no current evidence-backed source-global declaration to add; the
remaining job is to attribute those fourteen non-padding frontier ranges before deciding
whether any contains an original game datum.

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
