# docs/domain — Gruntz game semantics (distilled)

What the WWD data model *means*, distilled from the community reference mirrored in
`docs/reference/gooroosgruntz/` (GooRoo's Gruntz level-editor docs) and **mapped onto
our reconstruction** (`src/`). The editor (GLE / `GruntzEd.exe`) and `GRUNTZ.EXE`
share the WAP32 on-disk formats, so the editor's field docs are ground-truth for the
structs the game's loader parses.

| Doc | Covers |
|---|---|
| [toolz.md](toolz.md) | Toolz IDs 0–22 — weapons/utilities a Grunt equips; `Hit` (damage), projectiles, binary `TOOLZ_*` strings, movement-speed table |
| [toyz.md](toyz.md) | Toyz IDs 23–32 — give-away distractions, play durations, cheat codes, `TOYZ_*` strings |
| [powerupz.md](powerupz.md) | Brickz 35–39, PowerUpz 50–60, Cursez 61–64, Miscellaneous 75–99, Spellz 0–6 — the rest of the shared ID space |
| [enemy-ai.md](enemy-ai.md) | Enemy-AI types 1–16 (DumbChaserz … ScrollGrunt) + the enemy-Grunt WWD field semantics |
| [logic-classes.md](logic-classes.md) | WWD `Logic:` names → our `CUserLogic`/`CTileLogic` leaf classes + `LogicTypeId` tags |

## The one big result: the CGameObject +0x114 block is the WWD "user-value" union

The disputed `CGameObject` +0x114 block (six `i32`s) is the object record's
**user-value union**. The Gruntz Level Editor's *Edit Objects → Attributes* dialog
(`editor/EditObjects2.html`) lists these six fields **by name, in this order**:

> **Score, Points, Smarts, Powerup, Damage, Health**

which is exactly the labeling our `src/Wwd/WwdFile.cpp` record scatter already carries.
The **on-disk byte order** (fixed by the linear `*p++` read in `WwdFile.cpp:742`) is:

| Offset | Canonical field (GLE) | Our member | Per-logic reinterpretation (the reason it looked "disputed") |
|---|---|---|---|
| `+0x114` | **Score**   | `m_114` | CoveredPowerup: Megaphone call-order / ToyBox owning-team (0–3) |
| `+0x118` | **Points**  | `m_118` | GruntStartingPoint: **enemy AI type 1–16**; CoveredPowerup(Megaphone): Tool/Toy/Brick ID shown in GruntMachine |
| `+0x11c` | **Powerup** | `m_11c` | GruntStartingPoint: **carried Tool/Toy/Powerup ID**; CoveredPowerup: **covered object ID** (the 0–99 space below) |
| `+0x120` | **Damage**  | `m_120` | melee/projectile damage override |
| `+0x124` | **Smarts**  | `m_124` | GruntStartingPoint: **enemy team number 0–3**; CoveredPowerup: **revealed tile #** when dug/broken |
| `+0x128` | **Health**  | `m_128` | starting/override health |

**This resolves the dispute.** "score/points/powerup/damage/smarts/health" (the
generic WapWorld/libwap names our source uses) and the per-logic labels currently in
`include/Gruntz/UserLogic.h` (e.g. `+0x114` "teleporter spawn coord", `+0x11c`
"SpotLight settings-table index") are **two views of the same physical fields** — the
generic record slot vs. each `CUserLogic` leaf's reinterpretation of it. Both are
correct; neither is wrong. The GLE names are the canonical field names; each logic
class overlays its own meaning (see [logic-classes.md](logic-classes.md) and the
per-logic pages `editor/CoveredPowerUp.html`, `editor/GruntStartingPoint.html`).

Adjacent single fields the GLE dialog also exposes but which live **outside** this
union: `Face Dir` (timed-powerup duration in ms, or Spell ID for Scroll/Wand),
`Direction` (enemy sense range in tiles), `Speed X/Y`, `X Min/X Max/Y Min/Y Max`
(ObjectGuard guarded-object address). Their exact offsets are not yet pinned — see the
rename worklist in each doc.

## The shared "CoveredPowerup" object-ID space (`editor/AppendixB/IDz.html`)

One numeric ID space is reused by the WWD `Powerup:` field (`+0x11c`), the
`CoveredPowerup` logic, and the `InGameIcon` logic:

| Range | Category | Meaning |
|---|---|---|
| 0–22 | **Toolz** | equip a Grunt (0 = bare-handed) |
| 23–32 | **Toyz** | give-away distractions |
| 35–39 | **Brickz** | Brick-Layer construction materials |
| 50–60 | **PowerUpz** | health / enhancements |
| 61–64 | **Cursez** | hinder the player/enemy |
| 75–99 | **Miscellaneous** | Stopwatch/Coin/ToyBox/WarpLetters/TimeBombz |
| (0–6) | **Spellz** | Scroll/Wand spell subtype, held in `Face Dir`, not this field |

Every entry's engine image-set name string (`GAME_INGAMEICONZ_TOOLZ_BOMBZ`,
`GAME_INGAMEICONZ_TOYZ_BABYWALKERZ`, `GAME_INGAMEICONZ_POWERUPZ_COIN`, …) is present
verbatim in `GRUNTZ.EXE` — the catalogue maps 1:1 onto the binary. See the per-doc
"binary evidence" sections.
</content>
