# Enemy AI types (1–16)

An enemy Grunt's "nature". Selected by the WWD `Points:` field (`CGameObject +0x118`),
valid only when `Smarts:` (`+0x124`, the team number) is non-zero. Source:
`docs/reference/gooroosgruntz/editor/AppendixC/` (`01-DumbChaserz.html` … `16-ScrollGrunt.html`).

## The enemy-Grunt WWD fields (per `GruntStartingPoint`)

For a `GruntStartingPoint` object (`editor/GruntStartingPoint.html`,
`editor/AppendixC/*.html`):

| WWD field | Offset | Meaning for an enemy Grunt |
|---|---|---|
| `Points` | `+0x118` | **AI type 1–16** (this table) |
| `Smarts` | `+0x124` | **team number** 1/2/3 (`0` = player-controlled). Gruntz attack any Grunt of a *different* team |
| `Powerup` | `+0x11c` | carried Tool/Toy/Powerup ID (see [toolz.md](toolz.md)/[toyz.md](toyz.md)) |
| `Direction` | *(single field, offset unpinned)* | **sense range** — tiles away the Grunt detects another team's Grunt |
| `X Min`/`Y Min` | *(unpinned)* | ObjectGuard (type 6): the guarded object's address |
| Move Rect (L/T/R/B) | `+0x134`–`+0x140` | random-wander bounds (tiles), where the AI type honors it |

## The 16 types

| Points | Type | Default Tool | Behavior |
|---:|---|---|---|
| 1 | **Dumb Chaser** | any | hangs around until it senses your Grunt, then chases & attacks; stopped by killing, an Arrow tile, or leaving sense range |
| 2 | **Smart Chaser** | any | like Dumb Chaser but **ignores/won't chase** Gruntz carrying a stronger Tool (fights back if attacked) |
| 3 | **Hit And Runner** | any | attacks, then retreats to safety before attacking again |
| 4 | **Defender** | any | chases until `Direction` tiles from home, then returns; with a ranged weapon, stands and fires |
| 5 | **Post Guard** | any | stands guard; won't move/attack unless attacked (or handed a mobile Toy). Move Rect **not** used |
| 6 | **Object Guard** | any | paces around a guarded object (`X Min`/`Y Min`); attacks intruders, then resumes |
| 7 | **(Suicide) Bomber** | Bombz (1) | lights bomb and makes a straight-line kamikaze run; can run diagonally between pyramids |
| 8 | **Brick Layer** | Brickz (3) | lays Brickz wherever it can; **uses its Tool** |
| 9 | **Gauntletz Grunt** | Gauntletz (5) | attacks and breaks through breakable objects in the way |
| 10 | **Goo Sucker** | GooberStraw (7) | sucks up nearby Goo Puddlez (race the solver for Goo) |
| 11 | **Digger** | Shovel (13) | digs up nearby Moundz |
| 12 | **Time Bomber** | TimeBombz (17) | uses TimeBombz to break through obstacles / on sensing you |
| 13 | **Tool Thief** | any (usually none) | ignores bare-handed Gruntz; steals a Tool and attacks with it (kill to recover) |
| 14 | **Toyer** | any Tool + a **Toy** | chases to give you its Toy, then stands idle |
| 15 | **Magic Wand Grunt** | Wandz (19) | pacifist; casts spells (health −¼ each cast; 4 casts = death). Only Random Spellz reliably works |
| 16 | **Scroll Grunt** | Scroll (Toy 30) | gives you a scroll to read *and* attacks; only finishes reading if you hand the Scroll Grunt a Toy simultaneously |

Types 8–12, 15, 16 (the "specialized" Gruntz) **use their Tool/Toy** for its function;
types 1–6, 13 use their Tool only to fight; type 14 never uses its Tool unless attacked.

## Mapping to our reconstruction + worklist

- **`CGruntStartingPoint`** (`src/Gruntz/GruntStartingPoint.cpp`, RTTI `CUserLogic`) is
  the placement marker the site's `GruntStartingPoint` Logic page describes — it names
  the bound object `GAME_EXIT` and binds bute node `A`. The AI *behavior* dispatch on the
  `Points` value lives in the CGrunt brain in `src/Gruntz/Grunt.cpp` (**owned by
  matcher-2** — worklist only).
- **`CGruntCreationPoint`** (`src/Gruntz/GruntCreationPoint.cpp`) is the Battlez-map
  equivalent (`editor/GruntCreationPoint.html`; you use it *instead of*
  GruntStartingPoint in a Battlez map).
- **Worklist:** an `EnemyAiType` enum (`AI_DUMBCHASER=1 … AI_SCROLLGRUNT=16`) named from
  this table, applied to the `Points`-value (`+0x118`) switch in the CGrunt brain — a
  16-way dispatch is a strong anchor to look for. Matching-neutral.
</content>
