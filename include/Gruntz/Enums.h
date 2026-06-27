// Enums.h - Gruntz game-taxonomy enums, graduated from the comprehension layer
// (src/Stub/types/enums.h, which stays as the un-matched reference). Lifting the
// enum definitions into a real include/ header is matching-NEUTRAL: enumerator
// names do not change /O2 codegen. Where an enumerator's INTEGER value is recovered
// (Resolution, LaunchModeCode, Commands), the reconstructed src/ may use the name in
// place of the magic constant; for the rosters whose binary numbering is still
// unverified (GruntType/Tool/Toy/...), only the NAME set is authoritative - do NOT
// rely on the implicit 0..N values for matching until verified against the binary.
#ifndef GRUNTZ_GRUNTZ_ENUMS_H
#define GRUNTZ_GRUNTZ_ENUMS_H

#include <Ints.h>

/* ------------------------------------------------------------------ *
 * GruntType - 36 grunt types. Each maps to a sprite namespace
 * GRUNTZ_<TYPE> and (probably) a CGrunt subtype/behavior selector.
 * Source: STRINGS_ANALYSIS.md roster.
 * ------------------------------------------------------------------ */
enum GruntType {
    GRUNT_NORMAL,       // NORMALGRUNT
    GRUNT_WAND,         // WANDGRUNT
    GRUNT_CLUB,         // CLUBGRUNT
    GRUNT_SWORD,        // SWORDGRUNT
    GRUNT_GLOVEZ,       // GLOVEZGRUNT
    GRUNT_GAUNTLETZ,    // GAUNTLETZGRUNT
    GRUNT_SHOVEL,       // SHOVELGRUNT
    GRUNT_SPRING,       // SPRINGGRUNT
    GRUNT_GOOBER,       // GOOBERGRUNT
    GRUNT_BOMB,         // BOMBGRUNT
    GRUNT_TIMEBOMB,     // TIMEBOMBGRUNT
    GRUNT_ROCK,         // ROCKGRUNT
    GRUNT_BRICK,        // BRICKGRUNT
    GRUNT_BOOMERANG,    // BOOMERANGGRUNT
    GRUNT_NERFGUN,      // NERFGUNGRUNT
    GRUNT_GUNHAT,       // GUNHATGRUNT
    GRUNT_SHIELD,       // SHIELDGRUNT
    GRUNT_SPY,          // SPYGRUNT
    GRUNT_TOOB,         // TOOBGRUNT
    GRUNT_TOOBWATER,    // TOOBWATERGRUNT
    GRUNT_WELDER,       // WELDERGRUNT
    GRUNT_WINGZ,        // WINGZGRUNT
    GRUNT_GRAVITYBOOTZ, // GRAVITYBOOTZGRUNT
    GRUNT_WARPSTONE,    // WARPSTONEGRUNT
    GRUNT_SCROLL,       // SCROLLGRUNT
    GRUNT_REAPER,       // REAPERGRUNT
    GRUNT_POGOSTICK,    // POGOSTICKGRUNT
    GRUNT_YOYO,         // YOYOGRUNT
    GRUNT_JUMPROPE,     // JUMPROPEGRUNT
    GRUNT_BEACHBALL,    // BEACHBALLGRUNT
    GRUNT_BIGWHEEL,     // BIGWHEELGRUNT
    GRUNT_BABYWALKER,   // BABYWALKERGRUNT
    GRUNT_GOKART,       // GOKARTGRUNT
    GRUNT_JACKINTHEBOX, // JACKINTHEBOXGRUNT
    GRUNT_SQUEAKTOY,    // SQUEAKTOYGRUNT
    GRUNT_HAREKRISHNA,  // HAREKRISHNAGRUNT
    GRUNT_TYPE_COUNT    // = 36
    // (unverified) exact integer values / ordering unverified against the binary
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
    WARLORD_KING,     // WARLORDZ_KING
    WARLORD_NAPOLEAN, // WARLORDZ_NAPOLEAN  (sic - spelled this way in the binary)
    WARLORD_PATTON,   // WARLORDZ_PATTON
    WARLORD_VIKING,   // WARLORDZ_VIKING
    WARLORD_COUNT     // = 4
    // (unverified) exact integer values / ordering unverified against the binary
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
