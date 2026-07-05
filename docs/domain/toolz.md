# Toolz (object IDs 0–22)

Weapons/utilities a Grunt equips. The ID is a value of the WWD `Powerup:` field
(`CGameObject +0x11c`) and of the shared CoveredPowerup ID space. Source:
`docs/reference/gooroosgruntz/editor/AppendixB/Toolz/Toolz.html`.

`Hit` is the melee/projectile damage. `Image Set` names below are the exact strings
the engine uses — all present verbatim in `GRUNTZ.EXE` (see "Binary evidence").

| ID | Engine name (`GAME_INGAMEICONZ_`) | Tool | Hit | Projectile (speed / range) |
|---:|---|---|---:|---|
| 0 | *(IMPOSSIBLE — no icon)* | Bare-handed | 1 | — |
| 1 | `TOOLZ_BOMBZ` | (Suicide) Bombz | 20 | — |
| 2 | `TOOLZ_BOOMERANGZ` | Boomerangz | 6 | 200 ms/tile · 5+ tiles |
| 3 | `TOOLZ_BRICKZ` | Brick(layer)z | 4 | — |
| 4 | `TOOLZ_CLUBZ` | Clubz | 8 | — |
| 5 | `TOOLZ_GAUNTLETZ` | Gauntletz | 5 | — |
| 6 | `TOOLZ_GLOVEZ` | (Boxing) Glovez | 1 | — |
| 7 | `TOOLZ_GOOBERZ` | Goober (Straw)z | 2 | — |
| 8 | `TOOLZ_GRAVITYBOOTZ` | Gravity Bootz | 3 | — |
| 9 | `TOOLZ_GUNHATZ` | Gun Hatz | 10 | 1500 ms · 6 tiles |
| 10 | `TOOLZ_NERFGUNZ` | Sponge ('Nerf') Gunz | 1 | 1500 ms · 5 tiles |
| 11 | `TOOLZ_ROCKZ` | Rockz | 8 | 1000 ms · 4 tiles |
| 12 | `TOOLZ_SHIELDZ` | Shieldz | 1 | — |
| 13 | `TOOLZ_SHOVELZ` | Shovel | 6 | — |
| 14 | `TOOLZ_SPRINGZ` | Springz | 5 | — |
| 15 | `TOOLZ_SPYZ` | Spy Gear | 4 | — |
| 16 | `TOOLZ_SWORDZ` | Swordz | 10 | — |
| 17 | `TOOLZ_TIMEBOMBZ` | Time Bombz | 20 | 1 tile |
| 18 | `TOOLZ_TOOBZ` | Toobz | 2 | — |
| 19 | `TOOLZ_WANDZ` | (Magic) Wandz | 0 | spell range 7+ tiles · will not fight |
| 20 | `TOOLZ_WARPSTONEZ`, `…Z1`–`…Z4` | Warpstone Grunt | 0 | cannot fight (4 collectible pieces) |
| 21 | `TOOLZ_WELDERZ` | Welder's Kitz | 20 | 1500 ms · 4 tiles |
| 22 | `TOOLZ_WINGZ` | Wingz | 2 | fly 17 tiles · 500 ms/tile · range 4 |

The `Wandz`/`Warpstone` "cannot fight" pair have `Hit 0`. Warpstone occupies one ID
(20) but has four collectible sub-piece image-sets (`TOOLZ_WARPSTONEZ1..4`).

## Movement-speed table (Powerup ID → ms per tile)

The carried Tool/Toy also sets the Grunt's walk speed (`editor/GruntStartingPoint.html`).
This is a per-ID speed table; the binary very likely holds it as an array indexed by
the `+0x11c` powerup id. Non-default rows:

| ID | Tool | ms/tile (normal) | with SuperSpeed |
|---:|---|---:|---:|
| 0 | Bare-handed *(default for most)* | 600 | 300 |
| 1 | Bombz (when lit) | 600 → **200** lit | 300 → 100 |
| 11 | Rockz | **800** | 400 |
| 14 | Springz (when jumping) | 800 → **400** jumping | 400 → 200 |
| 15 | Spy Gear | **500** | 250 |
| 20 | WarpStone | **800** | 400 |
| 23 | Baby Walker (Toy) | **1200** | 600 |
| 58 | Death Touch (Powerup) | **800** | Impossible |

All other tools default to 600/300.

## Emergent / community-observed behavior

- **Renewable Wingz** (community-reported, likely unintended): certain actions renew a
  Wingz-Grunt's remaining flying time — e.g. playing with a Toy, or consuming Zap Cola.
  The Wingz flight timer is tracked by `CGruntWingzTimeSprite` (`LOGIC_GRUNTWINGZTIMESPRITE
  = 0x417`, `GetTypeTag @0x121a0`); the toy-play / Zap-Cola-consume paths presumably reset
  it without clearing the Wingz state. A concrete lead for whoever matches the Wingz-timer
  and Zap-Cola/toy-consume methods.

## Binary evidence

All 22 `TOOLZ_*` image-set names are literal strings in `GRUNTZ.EXE`, both bare and
`GAME_INGAMEICONZ_`-prefixed:

```
$ strings GRUNTZ.EXE | grep -oE 'GAME_INGAMEICONZ_TOOLZ_[A-Z0-9]+' | sort -u
GAME_INGAMEICONZ_TOOLZ_BOMBZ … _WELDERZ _WINGZ   (26 incl. WARPSTONEZ1..4)
```

`gruntz sema strings --find` does **not** surface them (its DB only indexes strings
Ghidra attached to a function xref); confirm with a raw `strings` grep on `$GRUNTZ_EXE`.

## Mapping to our reconstruction + rename worklist

- The `Hit` (damage) values and the movement-speed table are **data** the engine reads
  per tool id; find the tables (arrays indexed by the `+0x11c` powerup id) and pin them.
- Projectile-tool behavior lives in the `Projectile*` TUs (**owned by matcher-4** — do
  not edit here). The tool-id switch/dispatch in `CGrunt` lives in `src/Gruntz/Grunt.cpp`
  (**owned by matcher-2**). Both are worklist, not for this doc's author to rename.
- **Worklist (for the campaign's naming pass):** introduce a `ToolId` enum
  (`TOOL_BAREHANDED=0 … TOOL_WINGZ=22`) named from this table; apply to the tool-id
  literals in `Grunt.cpp`/`Projectile*`/the equip + speed paths — matching-neutral.
</content>
