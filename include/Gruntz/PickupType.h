#ifndef GRUNTZ_GRUNTZ_PICKUPTYPE_H
#define GRUNTZ_GRUNTZ_PICKUPTYPE_H

#include <Enums.h>

// The shared object-ID space. `docs/domain/README.md`: "One numeric ID space is
// reused by the WWD `Powerup:` field (+0x11c), the CoveredPowerup logic, and the
// InGameIcon logic." A Grunt's KIND is the id of what it carries, so the
// grunt-sprite roster is a second READING of these same numbers, not a second
// domain - hence the GRUNT_* aliases below rather than the two parallel enums
// that used to live in Enums.h (`GruntType`) and Play.cpp (`GruntTypeId`).
//
// Ranges (docs/domain/README.md, editor/AppendixB/IDz.html):
//   0-22  Toolz     23-32 Toyz      35-39 Brickz
//   50-60 PowerUpz  61-64 Cursez    75-99 Miscellaneous
// SPLIT domain: the game passes it as the 4-byte domain, but
// CGruntzCommand ships it as ONE wire byte (`s->Write(&m_extraByte, 1)`,
// `m_extraByte = *buf++`), so narrow fields declare
// GZ_ENUM_STORAGE(PickupType, i8) and keep retail's width. SIGNED, because
// retail's field is `char` and the domain carries PICKUP_INVALID = -1.
// Names follow retail's own image-set strings (`GAME_INGAMEICONZ_TOOLZ_BOMBZ`,
// `..._TOYZ_BABYWALKERZ`, `..._POWERUPZ_MEGAPHONEZ`) - see docs/strings-analysis.md.
GZ_ENUM_BEGIN_SPLIT(PickupType, i8)
// Written by CGrunt's reset path. Distinct from PICKUP_NONE, which is a
// real id (0 = bare-handed), not an absence.
    PICKUP_INVALID = -1,
    PICKUP_NONE = 0,

    // --- Toolz (0-22): what a Grunt equips; 0 is bare-handed ----------------
    PICKUP_BOMB = 1,
    PICKUP_EQUIPPABLE_FIRST = PICKUP_BOMB,
    PICKUP_BOOMERANG = 2,
    PICKUP_BRICK = 3,
    PICKUP_CLUB = 4,
    PICKUP_GAUNTLETZ = 5,
    PICKUP_GLOVEZ = 6,
    PICKUP_GOOBER = 7,
    PICKUP_GRAVITYBOOTZ = 8,
    PICKUP_GUNHAT = 9,
    PICKUP_NERFGUN = 10,
    PICKUP_ROCK = 11,
    PICKUP_SHIELD = 12,
    PICKUP_SHOVEL = 13,
    PICKUP_SPRING = 14,
    PICKUP_SPY = 15,
    PICKUP_SWORD = 16,
    PICKUP_TIMEBOMB = 17,
    // Half-open end of the range CGrunt::m_moveIcon accepts.
    PICKUP_MOVEICON_END = PICKUP_TIMEBOMB,
    PICKUP_TOOB = 18,
    PICKUP_WAND = 19,
    PICKUP_WARPSTONE = 20,
    PICKUP_WELDER = 21,
    PICKUP_WINGZ = 22,

    // Band boundary, NOT a fifth spelling of Wingz. 73 sites test `> this` to mean
    // "beyond the Toolz band", i.e. a Toy/Brick/PowerUp rather than an equippable
    // tool. Retail compares against 22 with `jle`; the equivalent
    // `>= PICKUP_BABYWALKER` is a DIFFERENT instruction (`cmpl $0x17; jl` vs
    // `cmpl $0x16; jle`, measured), so the boundary has to be named at the value
    // retail actually uses rather than restated as the next band's first member.
    PICKUP_EQUIPPABLE_LAST = PICKUP_WINGZ,

    // --- Toyz (23-32): give-away distractions -------------------------------
    PICKUP_BABYWALKER = 23,
    PICKUP_TOYZ_FIRST = PICKUP_BABYWALKER,
    // The half-open end of the Toolz band - the same boundary as
    // PICKUP_TOYZ_FIRST, seen from below. Retail spells "not equippable" BOTH
    // ways, `> 22` and `>= 23`, and those are different instructions, so each
    // site keeps the marker at the value it actually compares against.
    PICKUP_EQUIPPABLE_END = PICKUP_BABYWALKER,
    PICKUP_BEACHBALL = 24,
    PICKUP_BIGWHEEL = 25,
    PICKUP_GOKART = 26,
    PICKUP_JACKINTHEBOX = 27,
    PICKUP_JUMPROPE = 28,
    PICKUP_POGOSTICK = 29,
    PICKUP_SCROLL = 30,
    PICKUP_SQUEAKTOY = 31,
    PICKUP_YOYO = 32,
    PICKUP_TOYZ_LAST = PICKUP_YOYO,

    // Status-bar-only bare-hands frame. CActionOptionsMenuBar selects this
    // between the Toyz and Brickz bands when a Grunt has no equipped tool.
    PICKUP_BARE_HANDS_ICON = 0x21,

    // --- Brickz (34-38): Brick-Layer construction materials -----------------
    // The plain brick. CTileActionEvent::MorphByTool dispatches the five brick
    // tools 0x22-0x26 side by side, and 0x22's arm is the one that adds a BROWN
    // layer (BRICKTILE_RED_1 -> BRICKTILE_RED_2_LOW, keeping the red brick on the
    // bottom), so the range starts here - which is also what CGrunt::Place's
    // m_brickPickupType seed uses.
    PICKUP_BROWNBRICK = 0x22,
    PICKUP_BRICKZ_FIRST = PICKUP_BROWNBRICK,
    // The colour names come from the break cues (GAME_REDBRICKBREAK, ...); the
    // cheat messages in CGruntzMgr::OnCommand name the same four by FUNCTION,
    // which is what each colour actually does:
    //     red 0x23 = Gauntlet-Breaker   blue 0x24 = Teleport
    //     gold 0x25 = Indestructible    black 0x26 = Bomb
    PICKUP_REDBRICK = 0x23,
    // The four COLOURED bricks; brown (0x22) is the plain one and is excluded.
    PICKUP_COLORBRICK_FIRST = PICKUP_REDBRICK,
    PICKUP_BLUEBRICK = 0x24,
    PICKUP_GOLDBRICK = 0x25,
    PICKUP_BLACKBRICK = 0x26,
    PICKUP_BRICKZ_LAST = PICKUP_BLACKBRICK,

    // --- PowerUpz (50-60) ---------------------------------------------------
    PICKUP_MEGAPHONE = 0x32,
    PICKUP_POWERUPZ_FIRST = PICKUP_MEGAPHONE,
    PICKUP_HEALTH1 = 0x33,
    PICKUP_HEALTH2 = 0x34,
    PICKUP_HEALTH3 = 0x35,
    PICKUP_GHOST = 0x36,
    // The seven TIMED powerups. Megaphone (a call) and the three Zap Colas
    // (instant heal) sit below and are excluded, which is why the scoreboard's
    // powerup row is 7 wide: m_powerupPickupz[type - GHOST + 7 * owner].
    PICKUP_TIMEDPOWERUP_FIRST = PICKUP_GHOST,
    PICKUP_SUPERSPEED = 0x37,
    PICKUP_INVULNERABILITY = 0x38,
    PICKUP_CONVERSION = 0x39,
    PICKUP_DEATHTOUCH = 0x3a,
    PICKUP_ROIDZ = 0x3b,
    PICKUP_REACTIVEARMOR = 0x3c,
    PICKUP_TIMEDPOWERUP_LAST = PICKUP_REACTIVEARMOR,
    PICKUP_POWERUPZ_LAST = PICKUP_REACTIVEARMOR,

    // --- Cursez (61-64) -----------------------------------------------------
    PICKUP_RANDOMCOLORZ = 0x3d,
    PICKUP_CURSEZ_FIRST = PICKUP_RANDOMCOLORZ,
    PICKUP_SCREENSHAKE = 0x3e,
    PICKUP_BLACKSCREEN = 0x3f,
    PICKUP_MINICAM = 0x40,
    PICKUP_CURSEZ_LAST = PICKUP_MINICAM,

    // --- Miscellaneous (75-99) ----------------------------------------------
    PICKUP_STOPWATCH = 0x4b,
    PICKUP_COIN = 0x50,
    // The editor's table lists 85 as the Toy Box, opened to reveal its contents
    // and unable to sit inside a CoveredPowerup. CInGameIcon reads obj->m_points
    // beside it, which is where those contents are named.
    PICKUP_TOYBOX = 0x55,
    PICKUP_W = 0x5a,
    PICKUP_A = 0x5b,
    PICKUP_R = 0x5c,
    PICKUP_P = 0x5d,
    PICKUP_HELPBOX = 0x5e,
    PICKUP_COVEREDTIMEBOMB = 0x63,

    // --- The grunt-sprite reading of the same ids ---------------------------
    // A Grunt carrying object N IS grunt type N: retail's sprite namespace is
    // `GRUNTZ_<TYPE>` over this roster (docs/strings-analysis.md, 36 names).
    // Value-verified aliases, not a second domain.
    GRUNT_NORMAL = PICKUP_NONE,
    GRUNT_BOMB = PICKUP_BOMB,
    GRUNT_BOOMERANG = PICKUP_BOOMERANG,
    GRUNT_BRICK = PICKUP_BRICK,
    GRUNT_CLUB = PICKUP_CLUB,
    GRUNT_GAUNTLETZ = PICKUP_GAUNTLETZ,
    GRUNT_GLOVEZ = PICKUP_GLOVEZ,
    GRUNT_GOOBER = PICKUP_GOOBER,
    GRUNT_GRAVITYBOOTZ = PICKUP_GRAVITYBOOTZ,
    GRUNT_GUNHAT = PICKUP_GUNHAT,
    GRUNT_NERFGUN = PICKUP_NERFGUN,
    GRUNT_ROCK = PICKUP_ROCK,
    GRUNT_SHIELD = PICKUP_SHIELD,
    GRUNT_SHOVEL = PICKUP_SHOVEL,
    GRUNT_SPRING = PICKUP_SPRING,
    GRUNT_SPY = PICKUP_SPY,
    GRUNT_SWORD = PICKUP_SWORD,
    GRUNT_TIMEBOMB = PICKUP_TIMEBOMB,
    GRUNT_TOOB = PICKUP_TOOB,
    GRUNT_WAND = PICKUP_WAND,
    GRUNT_WARPSTONE = PICKUP_WARPSTONE,
    GRUNT_WELDER = PICKUP_WELDER,
    GRUNT_WINGZ = PICKUP_WINGZ,
    GRUNT_BABYWALKER = PICKUP_BABYWALKER,
    GRUNT_BEACHBALL = PICKUP_BEACHBALL,
    GRUNT_BIGWHEEL = PICKUP_BIGWHEEL,
    GRUNT_GOKART = PICKUP_GOKART,
    GRUNT_JACKINTHEBOX = PICKUP_JACKINTHEBOX,
    GRUNT_JUMPROPE = PICKUP_JUMPROPE,
    GRUNT_POGOSTICK = PICKUP_POGOSTICK,
    GRUNT_SCROLL = PICKUP_SCROLL,
    GRUNT_SQUEAKTOY = PICKUP_SQUEAKTOY,
    GRUNT_YOYO = PICKUP_YOYO,
    GRUNT_GHOST = PICKUP_GHOST,
    GRUNT_SUPERSPEED = PICKUP_SUPERSPEED,
    GRUNT_INVULNERABLE = PICKUP_INVULNERABILITY,
    GRUNT_CONVERSION = PICKUP_CONVERSION,
    GRUNT_DEATHTOUCH = PICKUP_DEATHTOUCH,
    GRUNT_ROIDZ = PICKUP_ROIDZ,
    GRUNT_REACTIVEARMOR = PICKUP_REACTIVEARMOR,

    // The two grunt types with no pickup of their own: retail names the sprite
    // sets HAREKRISHNAGRUNT and REAPERGRUNT - the Conversion and Death-Touch
    // grunts (docs/strings-analysis.md grunt roster).
    GRUNT_HAREKRISHNA = PICKUP_CONVERSION,
    GRUNT_REAPER = PICKUP_DEATHTOUCH
GZ_ENUM_END_SPLIT(PickupType, i8)

#endif // GRUNTZ_GRUNTZ_PICKUPTYPE_H
