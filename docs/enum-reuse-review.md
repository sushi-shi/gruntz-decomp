# Enum reuse audit

`gruntz verify enum-reuse` is the reproducible census for named integral domains in `include/` and
`src/`. It lexes every raw enum and `GZ_ENUM_*` block, evaluates members through the real compile
database with libclang, and requires the lexical and AST inventories to agree. The audit therefore
does not silently lose macro-backed enums or unevaluated expressions.

The command writes three derived, git-ignored reports:

- `build/gen/enum_reuse.tsv`: every evaluated declaration, its value, domain, source, and location;
- `build/gen/enum_value_collisions.tsv`: cross-domain equal values, augmented with grouped naked
  function-literal sites from `build/gen/bare_constants.tsv` when that report exists;
- `build/gen/enum_domain_pairs.tsv`: every pair with at least two shared values (or an exact value
  set), ranked by sequence/set agreement and coverage.

Useful focused views are `--duplicates`, `--value <integer>`, and `--json`. `--no-report` performs
the complete inventory and ledger verification without rewriting reports. `--init-ledger` is a
one-time operation and refuses to overwrite an existing review.

## Review result

The starting snapshot contains 276 blocks and 2,257 evaluated members. Every block and every one of
the 8,974 candidate domain-pair rows was reviewed. The durable verdict is
[`enum-reuse-review.tsv`](enum-reuse-review.tsv): 259 domains are retained as distinct, five are the
canonical owners of reused values, and twelve rows redirect one or more old members to a canonical
member. The verifier rejects pending rows, missing reasons, value-changing redirects, disappeared
members, and new current members without starting-ledger provenance.

Equal integers alone were not treated as identity. The retained cases include unrelated flag masks,
resource IDs, tuning percentages, constructor selector tags, independently encoded direction
families, UI state machines, and request/result protocols. Their producers and consumers do not
transport values across the domain boundary even when two complete value sets happen to overlap.

Four duplicate families were proved and consolidated:

| Shared domain | Removed duplicate identities | Evidence |
|---|---|---|
| `ColorTint` | local `TextColorId` | Both independently recovered switches cover the same 17 colors in the same order; the font payload now uses the existing tint IDs. `TextColorRef` remains a distinct mapping from IDs to `COLORREF` values. |
| `SpellId` | `SpellzEffect` | Scroll/wand payloads and the application switch use the same seven Bute-backed spell effects. |
| `PlayerSlot` | `BZ_PLAYER_COUNT`, `NET_SLOT_COUNT`, `NUM_PLAYER_SLOTS`, `TM_PLAYER_COUNT`, `TM_ALL_PLAYERS`, `BATTLEZ_TEAM_COUNT` | The arrays, dialogs, trigger selectors, game statistics, and network records all address the same four player slots; selector 5 means all of those slots. |
| trigger unit capacity | `BATTLEZ_UNIT_SLOT_COUNT`, `STATUSBAR_GRUNT_SLOT_COUNT`, `NET_DEFAULT_MAX_GRUNTZ` | Every consumer indexes or limits the same fifteen entries per player in `CTriggerMgr::m_units`. The canonical dimension is isolated in `TriggerGridDimensions.h`. |

Two single-member aliases were also centralized: `MS_PER_SECOND` now uses `MILLIS_PER_SECOND`, and
both `GetAsyncKeyState` consumers use the neutral `AsyncKeyStateMask` utility declaration. The
current inventory is 269 blocks and 2,222 members; the reduction includes moving the two shared
constants into neutral headers as well as deleting the duplicate declarations.

Naked integral literals remain evidence, not automatic replacements. The collision report exposes
their owning functions beside each value so a future audit can pursue a literal only when its
producer, consumer, storage, or protocol proves a domain. This review replaced the semantically
proved named aliases and did not turn coincidental numeric literals into enum casts.

## Build adjudication

The full MSVC 5.0 build left every directly edited function at its prior score (and kept the exact
spell and async-key consumers exact). Moving the shared declarations through `TriggerMgr.h` did
perturb six source-unchanged functions in dependent translation units: `CMapMgr::LineIsClear`,
`CGruntSelectedSprite::Update`, `CGruntToySprite::Update`, `CRollingBall::Update`,
`CSpotLight::Tick`, and `CSpriteRef::Build`. Their historical MAX values remain preserved.

This is the repository's already-established VC5 front-end TU-state effect: a correct shared-header
declaration change can alter allocation or scheduling in a later unchanged body. Restoring duplicate
semantic enums or adding inert declarations solely to recover those states would violate the source
model. The new baseline therefore records the current state as an adjudicated correctness keep while
retaining each historical MAX for later authentic source work.

## Re-running the gate

```sh
nix develop
gruntz verify enum-reuse --jobs 8
python -m unittest scripts.gruntz.verify.selftest.EnumReuseControls
gruntz build
```

Review rows are keyed to the starting source enum. A removed or split enum stays in the ledger and
maps each old member to `path:domain::member`; this preserves the evidence for why the duplicate was
removed instead of allowing a later inventory to forget it.
