# PowerUpz, Cursez, Brickz, Miscellaneous, Spellz

The rest of the shared CoveredPowerup ID space (`+0x11c`), beyond Toolz/Toyz. Sources:
`docs/reference/gooroosgruntz/editor/AppendixB/{PowerUpz,Cursez,Brickz,Miscellaneous,Spellz}/`.

## PowerUpz (50–60)

`Designer controlz duration` = the timed ones read their duration from the object's
`Face Dir` field (ms; `0` = game default).

| ID | Engine name (`GAME_INGAMEICONZ_POWERUPZ_`) | PowerUp | Effect |
|---:|---|---|---|
| 50 | `MEGAPHONEZ` | Megaphonez | calls for a Tool/Toy/Brick into the GruntMachine (ID in `Points`, order in `Score`) |
| 51 | `HEALTH1` | Zap Cola — Can | +5/20 health |
| 52 | `HEALTH2` | Zap Cola — Bottle | +10/20 health |
| 53 | `HEALTH3` | Zap Cola — Keg | +20/20 health (full) |
| 54 | `GHOST` | Invisibility | timed |
| 55 | `SUPERSPEED` | Super Speed | timed (halves ms/tile — see [toolz.md](toolz.md)) |
| 56 | `INVULNERABILITY` | Invulnerability | timed |
| 57 | `CONVERSION` | Conversion | timed; a successful conversion adds to health |
| 58 | `DEATHTOUCH` | Death Touch | timed (slows to 800 ms/tile) |
| 59 | `ROIDZ` | Roidz | timed |
| 60 | `REACTIVEARMOR` | Reactive Armor | timed |

## Cursez (61–64)

| ID | Engine name (`GAME_INGAMEICONZ_POWERUPZ_`) | Curse |
|---:|---|---|
| 61 | `RANDOMCOLORZ` | Random Colorz |
| 62 | `SCREENSHAKE` | Screen Shake |
| 63 | `BLACKSCREEN` | Black Screen |
| 64 | `MINICAM` | Mini Cam |

## Brickz (35–39)

Brick-Layer construction materials (the `Score` field of a covered Brick has no special
meaning). A Bricklayer Grunt always carries a supply of Breakable (Brown) Brickz.

| ID | Brick | Effect on a passing enemy |
|---:|---|---|
| 35 | Gauntletz-breaker (Red) | strips the enemy's Tool |
| 36 | Unbreakable (Gold) | permanent wall |
| 37 | Teleporter (Blue) | teleports the enemy away |
| 38 | Bomb (Black) | removes the enemy from the game |
| 39 | Breakable (Brown) | ordinary brick (Bricklayer default) |

## Miscellaneous (75–99)

| ID | Engine name | Object | Effect |
|---:|---|---|---|
| 75 | `POWERUPZ_STOPWATCH` | Stopwatch | +1 minute to the level timer |
| 80 | `POWERUPZ_COIN` | Coin | counted toward a `PERFECT!` score |
| 85 | *(no `POWERUPZ_`; uses NOTE icon)* | Toy Box | opened to reveal contents — **cannot** be placed inside a CoveredPowerup |
| 90 | `SECRETW` | Warp Letter W | secret-level prize |
| 91 | `SECRETA` | Warp Letter A | secret-level prize |
| 92 | `SECRETR` | Warp Letter R | secret-level prize |
| 93 | `SECRETP` | Warp Letter P | secret-level prize |
| 99 | *(TimeBomb icon)* | Time Bombz | uncontrolled explosives on a preset timer |

## Spellz (0–6) — Scroll/Wand spell subtype (held in `Face Dir`)

Cast by a Scroll (Toy 30) or Magic Wand (Tool 19). `0` = random.

| ID | Spell | Effect (on all Gruntz near the reader) |
|---:|---|---|
| 0 | Random Spellz | one of 1–6 at random |
| 1 | Freeze | turns nearby Gruntz to Gruntziclez (even friends) |
| 2 | Health | restores full health (even enemies) |
| 3 | Resurrection | restores nearby Grunt Puddlez to life |
| 4 | Random Toyz | gives nearby Gruntz a random Toy |
| 5 | Teleport | teleports nearby Gruntz to random locations |
| 6 | Rolling Ballz | Giant Ballz roll out in the four cardinal directions |

## Binary evidence

The 17 `POWERUPZ_*` and 4 `SECRET[WARP]` image-set names are literal strings in
`GRUNTZ.EXE`:

```
$ strings GRUNTZ.EXE | grep -oE 'POWERUPZ_[A-Z0-9]+' | sort -u
POWERUPZ_BLACKSCREEN COIN CONVERSION DEATHTOUCH GHOST HEALTH1 HEALTH2 HEALTH3
INVULNERABILITY MEGAPHONEZ MINICAM RANDOMCOLORZ REACTIVEARMOR ROIDZ SCREENSHAKE
STOPWATCH SUPERSPEED
$ strings GRUNTZ.EXE | grep -oE 'GAME_INGAMEICONZ_SECRET[WARP]'   # 90-93
```

## Mapping to our reconstruction

- **`CGruntPowerupSprite`** (`include/Gruntz/GruntPowerupSprite.h`) is the "grunt has a
  powerup" indicator sprite: `m_powerupId @+0x5c`, `InitActReg` builds `g_powerupActReg`
  over the **act-handler** range `[2000,2010]` (a *different* id namespace from these
  object IDs — the per-powerup update handler table). `Serialize` re-resolves the
  powerup's bute-set record from `g_gameReg->m_78`.
- **`CCoveredPowerupLogic`** — the CoveredPowerup logic (id 26 in the tile-trigger
  factory, `??0CCoveredPowerupLogic @0x112240`, `src/Gruntz/TileTriggerFactory.cpp`).
- Zap Cola / health restore, timed-powerup duration, and Megaphone call-for logic live
  in the CGrunt powerup paths (`src/Gruntz/Grunt.cpp`, **matcher-2**) — worklist only.
- **Worklist:** a `PowerupId`/`CurseId`/`MiscId` enum from the tables above, applied to
  the object-id literals; and confirm the `[2000,2010]` act-handler ids against the
  per-powerup effect handlers.
</content>
