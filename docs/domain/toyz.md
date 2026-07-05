# Toyz (object IDs 23–32)

Give-away distractions. A Grunt handed a Toy stops and plays with it for the Toy's
`Play duration` (a Suicide Bomber will even abort a kamikaze run to play). The ID is a
value of the WWD `Powerup:` field (`CGameObject +0x11c`). Source:
`docs/reference/gooroosgruntz/editor/AppendixB/Toyz/Toyz.html`.

| ID | Engine name (`GAME_INGAMEICONZ_TOYZ_`) | Toy | Play duration | Mobile? | Cheat code |
|---:|---|---|---:|:--:|---|
| 23 | `BABYWALKERZ` | Baby-Walker | 9 | Yes | `gerber` |
| 24 | `BEACHBALLZ` | Beach Ball | 26 | No | `theball` |
| 25 | `BIGWHEELZ` | Monster Wheel | 27 | Yes | `wheeliez` |
| 26 | `GOKARTZ` | Go-Kart | 28 | Yes | `malibugrandprix` |
| 27 | `JACKINTHEBOXZ` | Jack-In-The-Box | 24 | No | `ultimatecheeseburger` |
| 28 | `JUMPROPEZ` | Jumprope | 19 | No | `getinshape` |
| 29 | `POGOSTICKZ` | Pogo-stick | 8 | Yes | `boingee` |
| 30 | `SCROLLZ` | Scroll | *varies* | No | `magicpaper` |
| 31 | `SQUEAKTOYZ` | Squeak Toy | 19 | No | `mousey` |
| 32 | `YOYOZ` | Yo-Yo | 8 | No | `lametoy` |

- **Monster Wheel** is `TOYZ_BIGWHEELZ` in the engine (the display name and the
  asset name diverge — worth remembering when grepping).
- **Scroll (30)** is a Toy that casts a Spell; the Spell subtype (1–6) is held in the
  object's `Face Dir` field, not the Toy ID. See [powerupz.md](powerupz.md) → Spellz.
- `Mobile?` = whether the Grunt can move while playing.

Design note (from the catalogue): short-duration Toyz are given to *enemy* Gruntz for
*your* Gruntz to hand back; long-duration/mobile Toyz are for the solver to give *away*.

## Binary evidence

All 10 `TOYZ_*` names are literal strings in `GRUNTZ.EXE`:

```
$ strings GRUNTZ.EXE | grep -oE 'GAME_INGAMEICONZ_TOYZ_[A-Z]+' | sort -u
GAME_INGAMEICONZ_TOYZ_BABYWALKERZ  _BEACHBALLZ  _BIGWHEELZ  _GOKARTZ
_JACKINTHEBOXZ  _JUMPROPEZ  _POGOSTICKZ  _SCROLLZ  _SQUEAKTOYZ  _YOYOZ
```

The **cheat codes** (`gerber`, `boingee`, …) are **not** in the EXE (data-file /
obfuscated); do not expect to grep them.

The "play with a Toy" timer is `CGruntToyTimeSprite` (`LOGIC_GRUNTTOYTIMESPRITE =
0x411`, `GetTypeTag @0x120e0`) — see `include/Gruntz/LogicTypeId.h`.

## Mapping + rename worklist

- The Toy-play state machine and the toy-give path live in `src/Gruntz/Grunt.cpp`
  (**owned by matcher-2**) — worklist only.
- **Worklist:** extend the `ToolId` enum from [toolz.md](toolz.md) with the Toyz range
  (`TOY_BABYWALKER=23 … TOY_YOYO=32`); apply to the toy-id literals once the owning TUs
  are free. Matching-neutral.
</content>
