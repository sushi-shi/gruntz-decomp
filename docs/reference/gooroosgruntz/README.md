# gooroosgruntz.info — mirrored community reference

Offline snapshot of the **GooRoo's Gruntz** level-editor documentation
(<https://gooroosgruntz.info>), the community's authoritative reference for the
**game semantics** of the WWD data model that `GRUNTZ.EXE` (this decomp) loads.
Snapshot taken 2026-07-05.

The Gruntz Level Editor (GLE / `GruntzEd.exe`) and the game share the WAP32 on-disk
formats, so the editor's field-by-field docs are a labeled dump of the very structs
the game's loader parses. This makes the site the ground-truth for *what the WWD
object fields mean* — see `docs/domain/` for the distilled tables and the mapping
onto our reconstruction (`src/`).

## What is here (game-semantic pages only)

- `editor/AppendixB/` — the entity catalogues (the shared "CoveredPowerup" ID space):
  - `Toolz/` (IDs **0–22**), `Toyz/` (**23–32**), `Brickz/` (**35–39**),
    `PowerUpz/` (**50–60**), `Cursez/` (**61–64**), `Miscellaneous/` (**75–99**),
    `Spellz/` (**0–6**, the Scroll/Wand spell subtype), plus `IDz.html` (the master
    ID map) and `Toolz/WeaponzGrade.html`.
- `editor/AppendixC/` — the 16 **enemy-AI types** (`01-DumbChaserz` … `16-ScrollGrunt`)
  + `EnemyAITypez.html` index. These are the `Points:` field values 1–16.
- `editor/AppendixD/VoiceTrigger.html`.
- `editor/AppendixA/` — the eight custom-level graphics themes + `GlobalAmbientSound`.
- The **Logic pages** (each is a WWD `Logic:` field value = a `CUserLogic`/`CTileLogic`
  leaf class): `CoveredPowerUp`, `GruntStartingPoint`, `GruntCreationPoint`,
  `InGameIcon`, `Teleporterz`, `RainCloud`, `UFO`, `ObjectDropper`, `SecretLevelTrigger`,
  `CheckPointTrigger`, `ExitTrigger`, `TileTrigger`, `FortressFlag`, `WarpstonePad`,
  `Spotlight`, `WayPoint`, `GuardPoint`, `GiantRock`, `Hazardz`, `StaticHazardz`, etc.
- The WWD record layout pages: `EditObjects.html` / `EditObjects2.html` (the object
  attribute dialog — the canonical `Score/Points/Smarts/Powerup/Damage/Health` field
  names), `ObjectRects.html`, `ObjectFlags.html`, `InGameIcon.html`.
- `editor/Switchez.html`, `editor/TileAttributez.html`, `editor/CheatCodez.html`,
  `hintz/Glossary.html`, `editor/HelpBoxez.html`.

## What was left out

Tile-art catalogue pages (`editor/Tilez*.html`), the walkthrough/hint sub-site
(`hintz/Custom/…`), and all binary media (images, `.wav`, `.zip`, `.exe`, `.wwd`)
were **not** mirrored — they carry no reconstruction-relevant semantics. Pages are
text HTML with image `<img>` refs left as dead relative links (the images live on
the live site). Fetch anything missing from the source URL above.
</content>
</invoke>
