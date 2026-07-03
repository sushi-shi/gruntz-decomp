// Enums.h - Gruntz game-taxonomy enums. This is the authoritative home (the old
// comprehension header src/Stub/types/enums.h has been consumed into here and
// axed). Lifting the enum definitions into a real include/ header is matching-NEUTRAL: enumerator
// names do not change /O2 codegen. Where an enumerator's INTEGER value is recovered
// (Resolution, LaunchModeCode, Commands, GruntType), the reconstructed src/ may use the
// name in place of the magic constant; for the rosters whose binary numbering is still
// unverified (Tool/Toy/...), only the NAME set is authoritative - do NOT rely on the
// implicit 0..N values for matching until verified against the binary. GruntType is now
// recovered: grunt kinds ARE object-type ids (the shared PickupType space in
// <Gruntz/PickupType.h>), so its values are the real id space (see below).
#ifndef GRUNTZ_GRUNTZ_ENUMS_H
#define GRUNTZ_GRUNTZ_ENUMS_H

#include <Ints.h>

/* ------------------------------------------------------------------ *
 * GruntType - the playable-grunt roster (sprite namespace GRUNTZ_<TYPE>).
 * A grunt "kind" IS an object type: CGrunt::m_gruntKind (+0x258) holds a value
 * from the shared PickupType id space (<Gruntz/PickupType.h>). So GruntType is
 * numbered in that real id space, NOT the old 0-based roster guess:
 *   - toys  0x17..0x20 VERIFIED (CGruntCmdObj::LoadVehicleGruntSprites loads
 *           "<NAME>GRUNT" at the toy pickup id, e.g. 0x17 = BABYWALKERGRUNT);
 *   - tools 1..22 + NORMAL=0 INFERRED from the same identity (PickupType's
 *           alphabetical tool band; m_gruntKind==0 is the unarmed default), and
 *           consistent with the powerup kinds m_gruntKind takes (0x36 GHOST,
 *           0x37 SUPERSPEED, 0x38 INVULNERABILITY, 0x39/0x3a CONVERSION/DEATHTOUCH).
 * ------------------------------------------------------------------ */
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
    // Grunt-only appearances with NO recovered object-type id (no pickup/vehicle
    // equivalent traced): TOOBWATERGRUNT, REAPERGRUNT, HAREKRISHNAGRUNT. Left
    // unnumbered rather than fabricate an id that would collide with the space above.
};

/* ------------------------------------------------------------------ *
 * Tool - 22 Toolz. Sprite namespace TOOLZ_<NAME>.
 * ------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------ *
 * Toy - 10 Toyz. Sprite namespace TOYZ_<NAME>.
 * ------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------ *
 * Warlord - 4 enemy bosses. Sprite namespace WARLORDZ_<NAME>.
 * ------------------------------------------------------------------ */
enum Warlord {
    WARLORD_KING,     // WARLORDZ_KING     = 0
    WARLORD_NAPOLEAN, // WARLORDZ_NAPOLEAN = 1  (sic - spelled this way in the binary)
    WARLORD_PATTON,   // WARLORDZ_PATTON   = 2
    WARLORD_VIKING,   // WARLORDZ_VIKING   = 3
    WARLORD_COUNT     // = 4
    // VERIFIED: ordering matches CFortressFlag's m_124 switch (GAME_FORTRESSFLAGZ_*).
};

/* ------------------------------------------------------------------ *
 * Powerup - 5 powerupz. Sprite namespace POWERUPZ_<NAME>.
 * ------------------------------------------------------------------ */
enum Powerup {
    POWERUP_COIN,       // POWERUPZ_COIN
    POWERUP_GHOST,      // POWERUPZ_GHOST
    POWERUP_MINICAM,    // POWERUPZ_MINICAM
    POWERUP_ROIDZ,      // POWERUPZ_ROIDZ
    POWERUP_CONVERSION, // POWERUPZ_CONVERSION  (help text at string id 0x81ef)
    POWERUP_COUNT       // = 5
    // (unverified) exact integer values / ordering unverified against the binary
};

/* ------------------------------------------------------------------ *
 * ColorTint - ~17 ownership-tint colors applied to tool/toy sprites.
 * Used as <COLOR>_{TOOL,TOY}.
 * ------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------ *
 * Direction - 8-way movement. The CGrunt debug dump prints [dir=%d].
 * Ordering below is a GUESS (clockwise from North); TODO verify.
 * ------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------ *
 * Statez - game state-machine state ids (STATEZ_*). Drives CState and its
 * subclasses (CSplashState, CMenuState, ...).
 * ------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------ *
 * LaunchMode - WinMain command-line dispatch tokens (verbatim).
 * LOAD: takes an argument (LOAD:<name>). These are parsed as strings; the enum
 * is our convenience mapping.
 * ------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------ *
 * LaunchModeCode - the INTEGER mode code the binary actually stores after
 * parsing the command line (the internal "unknownMode"), distinct from the string
 * LaunchMode tokens above. Recovered from the dispatch (version-independent).
 * ------------------------------------------------------------------ */
enum LaunchModeCode {
    LAUNCHCODE_DEFAULT = 2, // no recognized launch token
    LAUNCHCODE_PLAY = 3,    // "PLAY"
    LAUNCHCODE_DEMO = 7,    // "DEMO"
    LAUNCHCODE_SELECT = 16, // "SELECT"
    LAUNCHCODE_MULTI = 17   // "MULTI"
    // (unverified) codes for the remaining tokens (ATTRACT/HOST/JOIN/...) not recovered
};

/* ------------------------------------------------------------------ *
 * Resolution - the "Resolution" registry value decoded by
 * CGruntzMgr::DecodeResolution() into width x height (16bpp). Default = 1
 * (640x480), which is also the DirectDraw init mode.
 * ------------------------------------------------------------------ */
enum Resolution {
    RES_640x480 = 1, // 640 x 480  (default; the only mode the engine inits)
    RES_800x600 = 2, // 800 x 600
    RES_1024x768 = 3 // 1024 x 768
};

/* ------------------------------------------------------------------ *
 * Commands - WM_COMMAND notification codes posted by the app.
 * ------------------------------------------------------------------ */
enum Commands {
    // Posted when the game is launched from a DirectPlay lobby (LOBBYLAUNCH).
    LOBBYLAUNCH = 0x80B7
};

/* ------------------------------------------------------------------ *
 * GruntzVolumeAttenuation - the 101-entry volume->attenuation lookup table the
 * audio code uses to convert a 0..100 volume into a DirectSound attenuation
 * (hundredths of a dB). Generated by:
 *     table[i] = (int)( -1000.0 * log2(100.0 / i) )    for i = 1..100
 *     table[0]   = -10000   (silence floor)
 *     table[100] = 0        (full volume)
 * Modeled here as the generating spec, not the 101 literals.
 * ------------------------------------------------------------------ */
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
