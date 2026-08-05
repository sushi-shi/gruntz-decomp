# Enum modeling — the numeric-domain campaign

Gruntz is full of small integer domains — object ids, logic tags, AI types,
states, directions, flags — and the reconstruction stores essentially all of them
as `i32` with bare literals at the use sites. This is the plan to give every
proven domain a name and a type.

The measured starting state (`src/` + `include/`):

| | count |
|---|---:|
| numeric `case` labels | 2260 (vs 928 named — 71% raw) |
| `==` / `!=` against a literal, excluding 0/1 | 1286 |
| enum definitions | 82 (51 in `include/`, 31 in `.cpp`s) |
| enumerators declared | 909 |
| enum tags ever used as a declared type | **3** of 82 |
| members typed with an enum | **0** |

The three that are types are `LogicTypeId` (`virtual LogicTypeId GetTypeTag()`),
`GameStateId` (`virtual GameStateId Update()`) and `GruntzCommand`
(`virtual i32 HandleCommand(i32, GruntzCommand, i32)`). Everything else is a
constant namespace over `i32` fields. **Enum adoption here is therefore
mangling-neutral almost everywhere** — there are barely any signatures to change.

This is already project policy, just unexecuted — `AGENTS.md`: *"Use named, typed
enums for proven numeric domains instead of magic macros. Enumerate only values
supported by evidence."*

## What is actually wrong

1. **One value space, several spellings.** The 0–32 grunt/pickup ID space is
   modelled three times: `GruntType` (`include/Gruntz/Enums.h`, dead),
   `PickupType` (the live one, 225 uses) and `GruntTypeId`
   (`src/Gruntz/Play.cpp:92`, a TU-local redeclaration, 50 uses). Two derived
   spaces shadow it without saying so: `enum Tool` is 0-based (`PickupType - 1`)
   and `CURSOR_TOOL_HANDZ = 0xc8` is `0xc8 + PickupType`. Same for warlords
   (`Warlord` vs `WarlordOwner`) and tiles (`TileCollision` vs `TileCollisionKind`).
2. **Named here, raw there.** `SerialMode` (`SERIAL_SAVE = 4`, `SERIAL_LOAD = 7`)
   is named in 5 files and spelled raw in 76 — which is exactly why `case 4:`
   (×145) and `case 7:` (×134) are the two most common case labels in the tree.
   The cheat IDs are dispatched by name in `GruntzMgrCmd.cpp` and registered as
   raw hex in `CheatMgr.cpp`, which never includes `GruntzCommandId.h`.
3. **Width splits.** `CGruntzCommand` stores `char m_commandKind; u8 m_targetType;`
   — serialized with `s->Write(&m_commandKind, 1)`, so the byte width is
   load-bearing — while the setter takes `char` and comparisons cast to `u32`.
   Same for `CButeMgr::m_tokType/m_lexState` (`i16`) and the `NetMgr` packet
   fields. Fragmentation is confined to the serialization/packet/palette
   boundaries; the game-logic core is uniformly `i32`.
4. **~130 dead enumerators.** `Tool`(23), `Toy`(11), `Powerup`(6), `ColorTint`(18),
   `Direction`(9), `LaunchMode`(15), `LaunchModeCode`(5) and ~37 of 40 `GruntType`
   members have zero use sites, while the same domains are spelled as raw
   literals elsewhere.

## The mechanism

`include/Enums.h` — one declaration, two expansions, selected by language level
(`__cplusplus >= 202002L`), never by a build flag. MSVC 5.0 and the existing
clang readers take the retail branch; a C++20 clang pass takes the strict branch
and type-checks the model without ever building the game.

The design is calibrated to what the compiler actually does — see
[`docs/patterns/enum-domains.md`](patterns/enum-domains.md) for the measurements:

- `enum class` and `enum E : u8` **do not parse** on VC5.0 (C2236 / C2059).
- `sizeof(enum)` is **always 4**, so a 1-byte field can never be enum-typed.
- Retyping a member / parameter / return / switch key from `i32` to a real enum,
  and naming the literals, leaves `.text` **byte-identical** — only a changed
  signature's mangling moves.
- An **opaque** `enum Kind;` forward declaration compiles, gives the same
  mangling and the same `.text`, and types a header without including the
  domain's definition.

Because gruntz ships **no PDB**, nothing external pins a declared type, so value
domains are **real enums in the matching build** — unlike `homm2-decomp`, whose
CodeView stream forced `typedef i32` on every domain. `cl` itself then rejects
wrong-domain arguments (C2664) and raw-int assignment (C2440); only "domain used
as a raw array index" needs the strict pass.

### The macro family

| Macro | Retail (MSVC 5.0) | Strict (clang C++20) | For |
|---|---|---|---|
| `GZ_ENUM_BEGIN/END(N)` | `enum N { … };` | `enum class N : i32 { … }; using enum N;` | value domain, 4 bytes everywhere |
| `GZ_ENUM_BEGIN/END_SPLIT(N,S)` | `enum N { … };` | `enum class N : S { … }; using enum N;` | value domain **also stored narrow** |
| `GZ_ENUM_CONST_BEGIN/END(N)` | `enum { … };` | *(same)* | constant bag — counts, extents, masks |
| `GZ_ENUM_FLAGS_BEGIN/END(N,S)` | `enum { … }; typedef i32 N;` | `enum class N : S { … }; using enum N;` | bitflag domain |
| `GZ_ENUM_FORWARD(N)` | `enum N` | `enum class N : i32` | opaque forward declaration |
| `GZ_ENUM_STORAGE(N,S)` | `S` | `GzEnumStorage<N,S>` | a field of domain N stored in S bytes |
| `GZ_ENUM_PARAM/RETURN(N,S)` | `S` | `N` | signature where retail's scalar ≠ the domain |
| `GZ_ENUM_BITFIELD(N,S)` | `S` | `N` | `: n` bitfield member |
| `GZ_ENUM_FLAGS_OPS(N)` / `GZ_ENUM_STEPPED(N)` | *nothing* | operator set | flag / sequence domains |
| `IDX(x)` / `HAS(f,b)` / `BIT(x)` | `(x)` / `((f)&(b))` / `(1<<(x))` | typed forms | spell the intent, move no bytes |

A 4-byte field, parameter or return of domain `N` is spelled `N` directly — the
macros exist only where retail's type differs from the domain type.

**Narrow storage is the "two sameish enums" answer.** A domain declared
`_SPLIT(PickupType, u8)` is *one* type; fields declare their audited width:

```cpp
class CGruntzCommand {
    GZ_ENUM_STORAGE(GruntzCommandKind, u8) m_commandKind;  // retail: u8, 1 byte
    GZ_ENUM_STORAGE(PickupType, u8)        m_targetType;
};
i32 SetParams(GZ_ENUM_PARAM(GruntzCommandKind, char) cmdKind, …);
```

In retail that is exactly `u8` and `char` — zero bytes move. In the strict build
`GzEnumStorage<N,S>` presents the domain type, converts implicitly *to* the enum,
converts to an integer only **explicitly**, and carries a cross-width converting
constructor, so a `u8` field of domain N assigns into an `i32` field of domain N
with no cast. That is what makes the byte field and the dword parameter the same
domain instead of two unrelated integers.

**Flag and sequence operators are strict-only on purpose.** Retail combined and
cleared bits with plain arithmetic, and an inline operator in a hot header
perturbs cl 5.0's /Ob1 inline budget.

### Where domains live, and naming

- **Shared domain → its own header** under `include/<Module>/`, one per file —
  already the tree's better half (`LogicTypeId.h`, `PickupType.h`,
  `GameStateId.h`). `include/Gruntz/Enums.h`, a 13-domain grab-bag that is mostly
  dead and whose inclusion perturbs ~47 TUs, is dissolved into these.
- **Single-TU domain → that `.cpp`.** Already the practice.
- **Never inline in a class**, except a genuinely class-scoped slot enum
  (`CGruntCellRec::NameSlot`).
- **Type names** `PascalCase`, singular: `PickupType`, `LogicTypeId`, `EnemyAiType`.
- **Enumerators** `SCREAMING_SNAKE` with a domain prefix. The retail branch is
  unscoped and the strict branch re-exports with `using enum`, so the prefix
  carries all disambiguation and cannot be dropped.
- Suffixes: `_NONE`/`_INVALID` for a −1 sentinel, `_COUNT`, `_VALID_BEGIN` (so
  nothing compares a domain value against literal `0`), `_MASK`.
- **Every enumerator gets an explicit `= value`** — implicit numbering is how
  `enum Tool` silently ended up one off from the documented ID space.

## Naming evidence

**Retail's own strings are the oracle.** `docs/strings-analysis.md` §11 and
`docs/domain/` carry the developers' names straight out of the binary —
`GAME_INGAMEICONZ_TOOLZ_BOMBZ`, `TOYZ_BABYWALKERZ`, `POWERUPZ_MEGAPHONEZ`,
`STATEZ_{SPLASH,MENU,…}`, `WARLORDZ_{KING,NAPOLEAN,PATTON,VIKING}`. Name from
these, not from invention. `docs/domain/README.md` holds the shared object-ID
range map (Toolz 0–22 / Toyz 23–32 / Brickz 35–39 / PowerUpz 50–60 / Cursez
61–64 / Misc 75–99) that `PickupType` half-models today.

## Rollout

Per the standing ruling, **`%` reshuffles are accepted**; gate on BUILD, not on
the score. Literal→enumerator substitution under `/O2` is far tamer than it was
for homm2 under `/Od` — in practice the whole first wave came out byte-neutral.

**Status: all eight steps executed. The strict-view drain is a standing
ratchet, not a finished job — see "What the strict count means".**

1. **Probe** — DONE; recorded in `docs/patterns/enum-domains.md`.
2. **`SerialMode` first** — `case 4:`/`case 7:` → `SERIAL_SAVE`/`SERIAL_LOAD`.
   33 of the 76 files already include `SerialArchive.h`, so for them this is pure
   constant substitution in a TU that already has the header — provably
   byte-neutral, and it clears the two most common magic case labels in the tree.
3. **Declare** — convert to `GZ_ENUM_*`, dissolve `Enums.h`, fold the duplicate
   ID spaces, hoist cross-TU domains stranded in `.cpp`s, delete the dead
   enumerators, add the new domains from `docs/domain/`. Headers only, so nothing
   moves.
4. **Apply** — literal → enumerator, then `i32` → domain type on members,
   params and returns, module by module. Prefer `GZ_ENUM_FORWARD` over a new
   `#include` wherever the TU only needs the type.
5. **Narrow the split domains** — `GZ_ENUM_STORAGE` on `CGruntzCommand`,
   `CButeMgr`, the `NetMgr` packet fields. `class_sizes` is the safety net.
6. **Strict pass** — add the C++20 clang mode and drive its errors to zero.
   homm2's equivalent pass surfaced 71 real defects (conflated domains, flags
   cleared by subtraction, typed values compared against `0`, enums used as raw
   indices). Treat each as a modeling bug, not a cast to add.
7. **Bitflag domains.** DONE for the domains whose bits are recoverable:
   `PidFlags` and `FileImageFormat` (all values powers of two, combined with `|`
   and tested with `&`) are now `GZ_ENUM_FLAGS_*` domains with the strict-only
   operator set. **The WWD object flags stay unmodelled on purpose** — the
   community docs (`editor/ObjectFlags.html`) list names (No Hit, Always Active,
   Safe, Mirror, Invert, Flash, User Flag 1-12) but no bit positions, and our
   source barely touches `m_addFlags`/`m_dynamicFlags`/`m_userFlags`, so there is
   nothing to recover a mapping from. Naming those bits would be invention, which
   the project's own rule forbids. The live `m_flags` bitfield on the tile cells
   (0x4, 0x8, 0x2000, 0x939, …) IS heavily used and is recoverable, but each bit
   needs its own behavioural proof — that is per-bit reverse engineering, not a
   mechanical pass.

## What landed

All byte-neutral: the score returned to its pre-campaign value (3260/4272 exact,
84.76% fuzzy), with the per-function MAX ledger preserved throughout.

- `include/Enums.h` — the `GZ_ENUM_*` layer; both branches compile-tested.
- **39 domains declared**, zero raw `enum` left in `include/` outside
  single-enumerator tag types.
- `SerialMode` recovered from a 2-value guess to the real **10-phase protocol**:
  `SERIAL_SNAPSHOT_BEGIN(1)`, `SERIAL_RESTORE_BEGIN(2)`, then
  pre/main/post per direction (3-5, 6-8), plus the two construction callbacks
  (`SERIAL_CREATE(9)`, `SERIAL_CREATE_BY_SERIAL_ID(10)`). Proven by
  `CDDrawSurfaceMgr::SnapshotChildren`/`RestoreChildren`'s call order and by
  `SerialObjectFactory`'s arms.
- **~380 serialize-family signatures** retyped: the `mode` parameter to
  `SerialMode` and the `typeId` parameter to `LogicTypeId`, including the
  `HP_Callback` archive callback typedef.
- `Enums.h` dissolved; three duplicate ID spaces folded into `PickupType` with
  name-preserving aliases; `GruntzCommand` → `GruntzCommandId` with 202
  enumerators re-cased; ~130 dead unproven enumerators deleted.
- **~460 magic literals named** (134 serialize case labels, 123 compiler-flagged
  comparison operands, 46 grunt-kind comparisons, 32 death causes, and the rest).
- Members typed across `CGrunt`, `CWwdGameObject`, `CButeValue`, and the two
  per-area death globals; `PickupType` declared `GZ_ENUM_BEGIN_SPLIT(…, u8)` on
  the evidence that `CGruntzCommand` ships it as one wire byte.

Findings the type system produced (each evidenced, not guessed):

- **`g_areaPageSize` is not a page size** — it is the per-world death cause for
  pit tiles. Renamed `g_areaPitDeath`; sibling → `g_areaHazardDeath`.
- **`GruntDeathType` slot 13 = `DEATH_EXIT`** — the only value `CellDispatch`
  routes to `BuildGruntExitAnimation()`.
- Death causes cross-check against the game: RainCloud→`DEATH_ELECTROCUTE`,
  RollingBall→`DEATH_SQUASH`, KitchenSlime→`DEATH_MELT`, SpotLight→`DEATH_KAROKE`.
- **`m_moveMode` carries a `PickupType`**, range-dispatched by pickup boundaries.
- **The Brickz range starts at `0x22`**, not `0x23` as the community docs say.
- **`InvokeCallback`'s type-id parameter carries two domains** — a `LogicTypeId`
  in phase 9, a record serial id in phase 10. Recorded, not merged.

## The strict drain

`config/cleanliness/strict-enums-baseline.tsv` stands at **89** distinct defects across 29
units, down from **1326**. Units with any error: 217 -> 29.

The number does not fall monotonically — it MEASURES how much of the tree still
treats domains as ints, so declaring a domain raises it until that domain's
consumers are typed:

| after | count |
|---|---:|
| first declaration wave | 1163 |
| members typed | 1339 |
| header roots fixed (PickupType storage, GetClassId, IDX subscripts) | 1084 |
| serialize `mode` + `typeId` typed (~380 signatures) | 970 |
| two parallel lanes drained their file sets | 74 |
| fourcc/flag/state domains unified, `ReportError` reverted | **89** |

### What the residue is

- **`other` 38 / `domain-to-int` 32** — a domain flowing into an untyped local or
  a Win32-shaped parameter.
- **`int-into-domain` 14** — a raw value entering a domain at an ingest point.
- **`wrong-domain` 4** — real conflations, the highest-value bucket.
- **`domain-as-index` 1**.

Reaching literal zero is bounded by evidence, not effort. Several remaining sites
are variables that genuinely carry two things, and typing them would be a lie:

- `CGrunt::m_moveMode` — a movement mode AND a pickup id (range-dispatched by
  pickup boundaries).
- `kindId` in `TriggerMgrGrid` — AI-derived tool ids, then overwritten with a
  colour index.
- `reason` in `GruntArrivalScan` — a pickup, then reused for a cell x. Splitting
  it into two locals was tried and **changes `.text`**, so retail reused the slot.
- `CGruntzMgr::ReportError`'s first parameter — takes both 0x8xxx resource ids
  and the 0x4xx ids the `WARP` macro passes. Two spaces, one slot.
- `CGrunt::m_toyBlendPct` — a blend percentage in one file, a Brickz id in another.
- `CHash::FindInt` — a generic integer lookup; one caller passes a fourcc, another
  an arbitrary symbol key.

Each is recorded at the site with what the two readings are.

### IDX and AT

`AT(array, value)` indexes an array whose index space is a domain; `IDX(x)` is
what remains — a genuine numeric conversion at a boundary the type system cannot
follow. 84 subscripts moved to `AT`. The `IDX` sites that remain are real domain
exits (a command id into a Win32 `WPARAM`, a pickup id summed into a checksum,
the load-bearing toy-ordinal jump-table bias in `GruntSteps`), and two of them
are annotated with the measurement showing the shape is byte-evidenced.

## Gates

- **Cleanliness board** (`scripts/gruntz/cleanliness/board.py`) — three ratcheted
  rows: `magic case labels`, `unnamed domain compares`, `.cpp-local enums`. The
  third closes an existing blind spot: `_TYPEDEF_DEF` matches `struct`/`class`
  only, so enums defined in `.cpp`s were invisible to `.cpp-local views`.
- **`gruntz audit enum-domains`** (`scripts/gruntz/audit/enum_domains.py`,
  `--gate` at the `normal` tier) — a `_SPLIT` domain's declared storage must match
  every `GZ_ENUM_STORAGE` width used for it (FATAL); no bare `enum X {` outside
  the macros; every enumerator has an explicit value; `config/cleanliness/enum-review.tsv`
  states are consistent.
- **`config/cleanliness/enum-review.tsv`** — a durable per-file `pending` / `reviewed` /
  `third-party` checklist. A file cannot be `reviewed` while it still has an
  unexplained code literal.
- **Worklist** — `readability-magic-numbers` in `config/cleanliness/tidy-audit.yaml`, read via
  `gruntz audit tidy`. Enabling it reverses that file's standing "matching-neutral
  floods are intentionally left OUT" note; the flood is now the queue.

## Verification

- **Probe / neutrality claims:** `llvm-objdump -dr -s -t` on the base obj, before
  vs after. Object identity, not `%` — a value-identical edit renumbers `$L`/`$T`
  labels, which objdiff scoring cannot see.
- **Per edit:** `gruntz build --fast`, read the touched unit's `%`.
- **Per commit:** `gruntz build --normal` — cleanliness ratchet, `label_style`,
  `include_order`, `verify_unique_names`, and the per-function MAX report.
- **After any member retype:** `gruntz build --full` runs `class_sizes`, which is
  what catches an accidental 1→4 byte widening.

## Traps

- **Retyping a param or return rewrites the mangling** — it flows into
  `build/gen/symbol_names.csv` → synth PDB → delink, so the `RVA_COMPGEN` pins
  must be updated in the same commit. `LogicTypeId` already shows the shape:
  `?GetTypeTag@CMovingLogic@@UAE?AW4LogicTypeId@@XZ`.
- **Assignment is the error; comparison is not.** `m_kind = 0x36;` stops
  compiling once `m_kind` is a domain, but `if (m_kind == 0x36)` still does.
  Conversions concentrate at ingest points, which is where they belong.
- **Adding a definition header to a TU that lacked it is not neutral** — that is
  the regalloc butterfly, unrelated to the enum. Prefer `GZ_ENUM_FORWARD`.
- **Serialization width is load-bearing.** `Write(&m_commandKind, 1)` takes the
  member's address with an explicit byte count. Never widen such a field.
- **`.clang-format` must know the macros** (`Macros:` / `StatementMacros:`), or
  it reflows every domain block. `Standard: c++03` stays.
