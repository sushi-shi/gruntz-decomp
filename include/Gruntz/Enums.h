#ifndef GRUNTZ_GRUNTZ_ENUMS_H
#define GRUNTZ_GRUNTZ_ENUMS_H

#include <Ints.h>

enum GruntType {
    GRUNT_NORMAL = 0, // NORMALGRUNT      (unarmed; m_gruntKind default 0)
    // Tool grunts (== PickupType tool band 1..22, alphabetical)
    GRUNT_BOMB = 1,         // BOMBGRUNT
    GRUNT_BOOMERANG = 2,    // BOOMERANGGRUNT
    GRUNT_BRICK = 3,        // BRICKGRUNT
    GRUNT_CLUB = 4,         // CLUBGRUNT
    GRUNT_GAUNTLETZ = 5,    // GAUNTLETZGRUNT
    GRUNT_GLOVEZ = 6,       // GLOVEZGRUNT
    GRUNT_GOOBER = 7,       // GOOBERGRUNT
    GRUNT_GRAVITYBOOTZ = 8, // GRAVITYBOOTZGRUNT
    GRUNT_GUNHAT = 9,       // GUNHATGRUNT
    GRUNT_NERFGUN = 10,     // NERFGUNGRUNT
    GRUNT_ROCK = 11,        // ROCKGRUNT
    GRUNT_SHIELD = 12,      // SHIELDGRUNT
    GRUNT_SHOVEL = 13,      // SHOVELGRUNT
    GRUNT_SPRING = 14,      // SPRINGGRUNT
    GRUNT_SPY = 15,         // SPYGRUNT
    GRUNT_SWORD = 16,       // SWORDGRUNT
    GRUNT_TIMEBOMB = 17,    // TIMEBOMBGRUNT
    GRUNT_TOOB = 18,        // TOOBGRUNT
    GRUNT_WAND = 19,        // WANDGRUNT
    GRUNT_WARPSTONE = 20,   // WARPSTONEGRUNT
    GRUNT_WELDER = 21,      // WELDERGRUNT
    GRUNT_WINGZ = 22,       // WINGZGRUNT
    // Toy / "vehicle" grunts (== PickupType toy band 0x17..0x20, VERIFIED)
    GRUNT_BABYWALKER = 0x17,   // BABYWALKERGRUNT
    GRUNT_BEACHBALL = 0x18,    // BEACHBALLGRUNT
    GRUNT_BIGWHEEL = 0x19,     // BIGWHEELGRUNT
    GRUNT_GOKART = 0x1a,       // GOKARTGRUNT
    GRUNT_JACKINTHEBOX = 0x1b, // JACKINTHEBOXGRUNT
    GRUNT_JUMPROPE = 0x1c,     // JUMPROPEGRUNT
    GRUNT_POGOSTICK = 0x1d,    // POGOSTICKGRUNT
    GRUNT_SCROLL = 0x1e,       // SCROLLGRUNT
    GRUNT_SQUEAKTOY = 0x1f,    // SQUEAKTOYGRUNT
    GRUNT_YOYO = 0x20,         // YOYOGRUNT
    // Powerup grunts (== the PickupType powerup band; docs/domain/powerupz.md ids
    // 54..59). Cross-verified in the bodies: SUPERSPEED (0x37) halves TimePerTile
    // (ReadConfigFromButeMgr; toolz.md speed table), INVULNERABILITY (0x38) gates
    // the melee conversion-dispatch and ROIDZ (0x3b) zeroes the AttackDowntime
    // (both in the attack-fire step @0x61cb0).
    GRUNT_GHOST = 0x36,        // GHOST (invisibility)
    GRUNT_SUPERSPEED = 0x37,   // SUPERSPEED (halved ms/tile)
    GRUNT_INVULNERABLE = 0x38, // INVULNERABILITY
    GRUNT_CONVERSION = 0x39,   // CONVERSION
    GRUNT_DEATHTOUCH = 0x3a,   // DEATHTOUCH
    GRUNT_ROIDZ = 0x3b,        // ROIDZ (no attack downtime)
    // REACTIVEARMOR (== PickupType 0x3c, powerupz.md id 60): CGrunt::m_gruntKind==0x3c
    // scales the outgoing hit by g_dtScale in the attack step (CGrunt combat @0x61cb0).
    GRUNT_REACTIVEARMOR = 0x3c,
    // Grunt-only appearances with NO recovered object-type id (no pickup/vehicle
    // equivalent traced): TOOBWATERGRUNT, REAPERGRUNT, HAREKRISHNAGRUNT. Left
    // unnumbered rather than fabricate an id that would collide with the space above.
};

typedef enum GruntDeathKind {
    // Warped out: StepWarpExit (@0x64540) posts the secret-level switch
    // ("WORLDZ\LEVEL%i", level+100) only on this kind.
    GRUNT_DEATH_WARPOUT = 0xc,
} GruntDeathKind;

typedef enum RezTypeTag {
    REZ_TAG_WWD = 0x575744, // 'WWD' - level world-data file
    REZ_TAG_WAV = 0x574156, // 'WAV' - sound sample file
} RezTypeTag;

enum Tool {
    TOOL_BOMBZ,        // TOOLZ_BOMBZ
    TOOL_BOOMERANGZ,   // TOOLZ_BOOMERANGZ
    TOOL_BRICKZ,       // TOOLZ_BRICKZ
    TOOL_CLUBZ,        // TOOLZ_CLUBZ
    TOOL_GAUNTLETZ,    // TOOLZ_GAUNTLETZ
    TOOL_GLOVEZ,       // TOOLZ_GLOVEZ
    TOOL_GOOBERZ,      // TOOLZ_GOOBERZ
    TOOL_GRAVITYBOOTZ, // TOOLZ_GRAVITYBOOTZ
    TOOL_GUNHATZ,      // TOOLZ_GUNHATZ
    TOOL_NERFGUNZ,     // TOOLZ_NERFGUNZ
    TOOL_ROCKZ,        // TOOLZ_ROCKZ
    TOOL_SHIELDZ,      // TOOLZ_SHIELDZ
    TOOL_SHOVELZ,      // TOOLZ_SHOVELZ
    TOOL_SPRINGZ,      // TOOLZ_SPRINGZ
    TOOL_SPYZ,         // TOOLZ_SPYZ
    TOOL_SWORDZ,       // TOOLZ_SWORDZ
    TOOL_TIMEBOMBZ,    // TOOLZ_TIMEBOMBZ
    TOOL_TOOBZ,        // TOOLZ_TOOBZ
    TOOL_WANDZ,        // TOOLZ_WANDZ
    TOOL_WARPSTONEZ1,  // TOOLZ_WARPSTONEZ1
    TOOL_WELDERZ,      // TOOLZ_WELDERZ
    TOOL_WINGZ,        // TOOLZ_WINGZ
    TOOL_COUNT         // = 22
    // (unverified) exact integer values / ordering unverified against the binary
};

enum Toy {
    TOY_BABYWALKERZ,   // TOYZ_BABYWALKERZ
    TOY_BEACHBALLZ,    // TOYZ_BEACHBALLZ
    TOY_BIGWHEELZ,     // TOYZ_BIGWHEELZ
    TOY_GOKARTZ,       // TOYZ_GOKARTZ
    TOY_JACKINTHEBOXZ, // TOYZ_JACKINTHEBOXZ
    TOY_JUMPROPEZ,     // TOYZ_JUMPROPEZ
    TOY_POGOSTICKZ,    // TOYZ_POGOSTICKZ
    TOY_SCROLLZ,       // TOYZ_SCROLLZ
    TOY_SQUEAKTOYZ,    // TOYZ_SQUEAKTOYZ
    TOY_YOYOZ,         // TOYZ_YOYOZ
    TOY_COUNT          // = 10
    // (unverified) exact integer values / ordering unverified against the binary
};

enum Warlord {
    WARLORD_KING,     // WARLORDZ_KING     = 0
    WARLORD_NAPOLEAN, // WARLORDZ_NAPOLEAN = 1  (sic - spelled this way in the binary)
    WARLORD_PATTON,   // WARLORDZ_PATTON   = 2
    WARLORD_VIKING,   // WARLORDZ_VIKING   = 3
    WARLORD_COUNT     // = 4
    // VERIFIED: ordering matches CFortressFlag's m_124 switch (GAME_FORTRESSFLAGZ_*).
};

enum Powerup {
    POWERUP_COIN,       // POWERUPZ_COIN
    POWERUP_GHOST,      // POWERUPZ_GHOST
    POWERUP_MINICAM,    // POWERUPZ_MINICAM
    POWERUP_ROIDZ,      // POWERUPZ_ROIDZ
    POWERUP_CONVERSION, // POWERUPZ_CONVERSION  (help text at string id 0x81ef)
    POWERUP_COUNT       // = 5
    // (unverified) exact integer values / ordering unverified against the binary
};

enum ColorTint {
    TINT_BLACK,    // BLACK
    TINT_BLUE,     // BLUE
    TINT_CYAN,     // CYAN
    TINT_DKBLUE,   // DKBLUE
    TINT_DKGREEN,  // DKGREEN
    TINT_DKRED,    // DKRED
    TINT_DKYELLOW, // DKYELLOW
    TINT_GREEN,    // GREEN
    TINT_GREY,     // GREY
    TINT_HOTPINK,  // HOTPINK
    TINT_ORANGE,   // ORANGE
    TINT_PINK,     // PINK
    TINT_PURPLE,   // PURPLE
    TINT_RED,      // RED
    TINT_TURQ,     // TURQ
    TINT_WHITE,    // WHITE
    TINT_YELLOW,   // YELLOW
    TINT_COUNT     // = 17
    // (unverified) exact integer values / ordering unverified against the binary
};

enum Direction {
    DIR_NORTH,     // NORTH
    DIR_NORTHEAST, // NORTH-EAST
    DIR_EAST,      // EAST
    DIR_SOUTHEAST, // SOUTH-EAST
    DIR_SOUTH,     // SOUTH
    DIR_SOUTHWEST, // SOUTH-WEST
    DIR_WEST,      // WEST
    DIR_NORTHWEST, // NORTH-WEST
    DIR_COUNT      // = 8
    // (unverified) numbering/winding order unverified against the binary
};

enum Statez {
    STATEZ_SPLASH,  // STATEZ_SPLASH    -> CSplashState
    STATEZ_MENU,    // STATEZ_MENU      -> CMenuState
    STATEZ_HELP,    // STATEZ_HELP      -> CHelpState
    STATEZ_CREDITZ, // STATEZ_CREDITZ   -> CCreditsState
    STATEZ_ATTRACT, // STATEZ_ATTRACT   -> CAttract
    STATEZ_PREVIEW, // STATEZ_PREVIEW
    STATEZ_BOOTY,   // STATEZ_BOOTY     -> CBootyState
    STATEZ_MULTI,   // STATEZ_MULTI     -> CMulti / CMultiBootyState
    STATEZ_COUNT
    // (unverified) exact integer values / ordering unverified against the binary
};

enum LaunchMode {
    LAUNCH_PLAY,        // "PLAY"        single-player launch
    LAUNCH_MULTI,       // "MULTI"       multiplayer launch
    LAUNCH_DEMO,        // "DEMO"        demo playback
    LAUNCH_ATTRACT,     // "ATTRACT"     attract loop
    LAUNCH_SELECT,      // "SELECT"      level/area select
    LAUNCH_EDIT,        // "EDIT"        editor mode
    LAUNCH_HOST,        // "HOST"        MP host
    LAUNCH_JOIN,        // "JOIN"        MP join
    LAUNCH_LOAD,        // "LOAD:"       load saved game (LOAD:<name>)
    LAUNCH_LOADGAME,    // "LOADGAME"    load game
    LAUNCH_NOLOGO,      // "NOLOGO"      skip logo movie
    LAUNCH_NOMOVIES,    // "NOMOVIES"    skip all movies
    LAUNCH_LOBBYLAUNCH, // "LOBBYLAUNCH" launched from a DirectPlay lobby
    LAUNCH_QUICKSTART,  // "QUICKSTART"  quick start
    LAUNCH_MODE_COUNT
    // (note) these are flags/modes parsed from the command line, not a strict
    //       sequential enum in the binary - TODO confirm representation.
};

enum LaunchModeCode {
    LAUNCHCODE_DEFAULT = 2, // no recognized launch token
    LAUNCHCODE_PLAY = 3,    // "PLAY"
    LAUNCHCODE_DEMO = 7,    // "DEMO"
    LAUNCHCODE_SELECT = 16, // "SELECT"
    LAUNCHCODE_MULTI = 17   // "MULTI"
    // (unverified) codes for the remaining tokens (ATTRACT/HOST/JOIN/...) not recovered
};

enum Resolution {
    RES_640x480 = 1, // 640 x 480  (default; the only mode the engine inits)
    RES_800x600 = 2, // 800 x 600
    RES_1024x768 = 3 // 1024 x 768
};

enum Commands {
    // Go to level <lParam>: the AREAS menu posts it with the stage's level index
    // (MainMenuBuilder), StepWarpExit with the secret-level index (level+100);
    // handled in CGruntzMgr's command switch (GruntzMgrCmd case 0x807f).
    GOTOLEVEL = 0x807F,
    // Posted when the game is launched from a DirectPlay lobby (LOBBYLAUNCH).
    LOBBYLAUNCH = 0x80B7
};

struct GruntzVolumeAttenuation {
    // In-class enum constants (MSVC 5.0 rejects `static const T x = N;` initializers).
    enum {
        kSize = 101,
        kFloor = -10000, // table[0]   (silence floor)
        kFull = 0        // table[100] (full volume)
    };
    // table[i] = -1000 * log2(100 / i), for i in 1..100
};

#endif // GRUNTZ_GRUNTZ_ENUMS_H
