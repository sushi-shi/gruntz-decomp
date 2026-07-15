// PickupType.h - the shared object / pickup / grunt-kind type id space.
//
// ONE id space drives every loader that dispatches on an object type, all
// VERIFIED against the retail MSVC switch jump tables to AGREE byte-for-byte:
//   - CGrunt::LoadPickupSprites        @0x65e80  (entrance cue, GRUNTZ_PICKUPS_*)
//   - EngineLabelBacklog::LoadPowerupIconSprites @0x7c620 (map icon, GAME_INGAMEICONZ_*)
//   - CGrunt::LoadVehicleGruntSprites @0x50ce0 (toy-grunt sprite, <NAME>GRUNT)
// The same id is stored in CGrunt::m_gruntKind (+0x258): a grunt "kind" IS an object
// type - VERIFIED 0x37=SUPERSPEED halves TimePerTile, 0x36=GHOST, and
// 0x38=INVULNERABILITY / 0x39=CONVERSION / 0x3a=DEATHTOUCH gate combat/tile logic.
//
// Graduated to one header so every user shares a single definition (the loaders had
// previously drifted - an icon<->pickup HEALTH1/HEALTH3 + INVULNERABILITY/REACTIVEARMOR
// transposition, reconciled from the retail jump tables). Enumerator names are
// matching-neutral: each is the same immediate as the bare literal.
#ifndef GRUNTZ_GRUNTZ_PICKUPTYPE_H
#define GRUNTZ_GRUNTZ_PICKUPTYPE_H

enum PickupType {
    PICKUP_NONE = 0,
    // Toolz (1..22, alphabetical)
    PICKUP_BOMB = 1,
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
    PICKUP_TOOB = 18,
    PICKUP_WAND = 19,
    PICKUP_WARPSTONE = 20, // = 0x14 (excluded from the weapon stat counter)
    PICKUP_WELDER = 21,
    PICKUP_WINGZ = 22, // = 0x16
    // Toyz (23..32, alphabetical) - the "vehicle grunt" kinds (0x17..0x20)
    PICKUP_BABYWALKER = 23, // = 0x17
    PICKUP_BEACHBALL = 24,
    PICKUP_BIGWHEEL = 25,
    PICKUP_GOKART = 26,
    PICKUP_JACKINTHEBOX = 27,
    PICKUP_JUMPROPE = 28,
    PICKUP_POGOSTICK = 29,
    PICKUP_SCROLL = 30,
    PICKUP_SQUEAKTOY = 31,
    PICKUP_YOYO = 32, // = 0x20
    // Colored brickz
    PICKUP_REDBRICK = 0x23,   // = 35
    PICKUP_BLUEBRICK = 0x24,  // = 36
    PICKUP_GOLDBRICK = 0x25,  // = 37
    PICKUP_BLACKBRICK = 0x26, // = 38
    // Megaphone (announces a unit-type count via the inner switch)
    PICKUP_MEGAPHONE = 0x32,
    // Health
    PICKUP_HEALTH1 = 0x33,
    PICKUP_HEALTH2 = 0x34,
    PICKUP_HEALTH3 = 0x35,
    // Powerupz (gameplay: 0x36..0x3c count toward the powerup stat)
    PICKUP_GHOST = 0x36,
    PICKUP_SUPERSPEED = 0x37,
    PICKUP_INVULNERABILITY = 0x38,
    PICKUP_CONVERSION = 0x39,
    PICKUP_DEATHTOUCH = 0x3a,
    PICKUP_ROIDZ = 0x3b,
    PICKUP_REACTIVEARMOR = 0x3c,
    // Powerupz (screen fx: 0x3d..0x40, set the force-cue flag)
    PICKUP_RANDOMCOLORZ = 0x3d,
    PICKUP_SCREENSHAKE = 0x3e,
    PICKUP_BLACKSCREEN = 0x3f,
    PICKUP_MINICAM = 0x40,
    PICKUP_STOPWATCH = 0x4b,
    PICKUP_COIN = 0x50,
    // Warp letters (spell "WARP")
    PICKUP_W = 0x5a,
    PICKUP_A = 0x5b,
    PICKUP_R = 0x5c,
    PICKUP_P = 0x5d,
    PICKUP_HELPBOX = 0x5e,
    // In-game-icon-only object type (no entrance/pickup sprite): the covered timebomb.
    PICKUP_COVEREDTIMEBOMB = 0x63, // = 99
};

#endif // GRUNTZ_GRUNTZ_PICKUPTYPE_H
