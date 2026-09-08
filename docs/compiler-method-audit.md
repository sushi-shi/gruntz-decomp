# Compiler-method review queue

The ongoing audit searches all owned function definitions, including unclaimed
and unannotated functions, and independently inventories actual VC5 special-member
and template emissions. Collection names and empty bodies rank the review; they
do not limit coverage or prove compiler generation. The earlier record-only
audit is in [template-model-audit.md](template-model-audit.md).

After a full build, run:

```sh
nix develop -c python3 scripts/audit-template-models.py
```

The generated `build/audits/template-models/review-queue.tsv` joins source USRs,
owner layouts, constructor parameters and initializers, call/allocation sites,
retail bindings, and real COFF emissions. Every owned definition has a row,
including an unremarkable method with no candidate signal. The complete record
and template catalogs remain alongside it. Multiple definition forms of one USR,
inconsistent owner shapes, explicit lifetime expressions, and constructor
selectors such as `INLINE_BASE` receive the first review priority.

[compiler-method-review.tsv](compiler-method-review.tsv) contains evidence and
dispositions, not a second manually maintained worklist. Reapply it without
parsing again using `--queue-only --output <census-directory>`. A source, binding,
object or compilation-configuration change invalidates that saved census.
Concurrent changes during harvesting also reject the result.

Reviews are scoped to the complete definition, observed macro/flag environments,
and owner/base/member declaration fingerprints. An emission review fingerprints
its actual code and ordered symbolic relocations, not a timestamp or its section
offset. A mismatching fingerprint becomes `stale-review`; a removed or renamed
key must be reconciled explicitly. Neither an unjoined emission nor an existing
template definition is automatically certified as an implicit special member or
an authentic recovered template. The queue statuses describe different evidence
surfaces; they are not all completed reviews.

## First special-member pass

The starting census at `8c76e8b5f` covers 282 TUs, 4,785 owned definitions and
31,551 use sites without parse errors or uncovered source files. It contains
294 manually claimed constructors/destructors, including 44 empty bodies.
This pass reviews those 44 bodies and reopens the two Brickz pool owners.
The remaining special members, ordinary methods, and owner candidates remain
in the generated queue. This is not completion of the whole template audit.

Six manually written destructors are replaced by implicit methods and
`RVA_COMPGEN` bindings:

| Owner | Retail RVA | Generated size | Evidence |
| --- | --- | ---: | --- |
| `CFileImageSurface` | 0x142360 | 83 | Same complete cleanup body and ordered relocations. |
| `CDDrawOverlaySurface` | 0x142820 | 83 | Same complete cleanup body and ordered relocations. |
| `CDDrawPrimarySurface` | 0x142a40 | 83 | Same complete cleanup body and ordered relocations. |
| `CDDrawZBufferSurface` | 0x142d40 | 83 | Same complete cleanup body and ordered relocations. |
| `CDDrawFrontSurface` | 0x1591b0 | 25 | Same base member resets and final `CObject` vptr store. |
| `CRezMgr::CRezItmChunkList` | 0x13abb0 | 1 | The pinned `845119c` `rezmgr.h` declares this concrete list without a destructor; the shared base generates its teardown. |

The stream list also becomes `CLTList<StreamVoice>`. Every insertion allocates a
`StreamVoice`; teardown, stopping and ticking recover that same type. Typed
getters replace the repeated erased-pointer downcasts. Its owner layout remains
unchanged. Five affected stream methods pass a direct raw-byte/ordered-referent
audit, including the import-library-resolved `timeGetTime` IAT slot.

The negative controls retain thirteen authored empty constructors. Omitting
the nine trigger constructors removes every constructor definition and reference
from both real TUs, whereas four decoded retail factory bodies make eighteen
calls to those constructors through verified linker thunks. Omitting the four
remaining default constructors likewise loses the four calls from `CFaderMgr`,
`CDDrawSurfaceMgr`, `WinMain` and `CGruntzApp`.

Empty destructors also have distinct results. Implicit `CFaderMesh` teardown
shrinks from 107 to 101 bytes; `CFaderSine` and `SoundStream` shrink from eleven
to five bytes. All three lose a retail derived-vptr store, so their authored
empty bodies remain. Previously established sound controls, source-proven
authored bodies, parameterized constructors, and trivial-record lifetimes are
recorded individually in the review table. The two default constructors using
the inferred `INLINE_BASE` overload remain open with their complete base family.

The Brickz audit follows both constructor/allocation thunks and actual consumers.
The 12-byte owners begin at `CMapMgr +0x30` and `+0x3c`. Node allocation/free uses
storage at owner +4, and recycling changes its free head at `CMapMgr +0x30`.
Cell allocation/free uses storage at owner +0, while cell pop/recycling changes
its free head at `CMapMgr +0x40` (owner +4). These independent reads corroborate
the reversed roles. They leave the common owner/node policy open; they do not
certify either reconstructed concrete name or an invented layout specialization.

The reusable compiler controls are in
[empty special-member boundaries](patterns/empty-special-member-calls-and-vptr-stores.md).
The definition-context check also exposes a real declaration inconsistency:
`CDibPal` was forward-declared as a struct while its sole complete definition
is a class. That made the typed `GetPalette` AST vary with include order. The
forward declaration now agrees with its canonical class. Three remaining
header/out-of-line definition pairs are explicit visibility-review items.

Local artifacts under `build/audits/compiler-queue-controls/` retain the complete
retail disassembly, source A/B snapshots, actual control objects, call-site
resolution, raw audits and build logs. Ten queue controls exercise exhaustive
fallback, initializer detection, source/owner/macro invalidation, conflicting
definitions, orphan reviews, independent emission coverage and stale-census
rejection. The parser control verifies that a missing whole-declaration token
range cannot hide a body or constructor-initializer edit; the selector controls
distinguish explicit lifetimes from bitwise complement and ordinary qualified calls.

The resulting census has 5,297 rows: 4,779 owned definitions, 99 record-family
candidates, and 419 emissions without an owned source body. Its 50 fingerprinted
decisions comprise six recovered implicit methods, 36 retained authored methods,
and eight open family/visibility questions. No review is stale or orphaned.
The remaining rows are unreviewed evidence surfaces, including 33 existing
template definitions; automatic classification does not close them.

Validation: the full `gruntz build` passes every gate, all ten queue controls
pass, and the eleven affected methods pass the raw-byte and ordered-referent
audit. All 4,429 historical RVA maxima are preserved. Current aggregate fuzzy
is 93.78114% across 1,167,465 bytes and 7,466 functions, with 6,626 current exact
functions; those aggregate counts do not replace the per-function MAX gate.
