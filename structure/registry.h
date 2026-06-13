#ifndef GRUNTZ_REGISTRY_H
#define GRUNTZ_REGISTRY_H

/*
 * Registry-backed configuration.
 *
 * Gruntz reads its options from HKLM\Software\Monolith Productions\Gruntz\1.0
 * (read-only path: RegOpenKeyA / RegQueryValueExA — see STRINGS_ANALYSIS.md §2).
 * Each value-name below maps 1:1 to a CGruntzMgr / options field. The raw
 * registry value-name string is kept verbatim in a comment; the C++ field name
 * is a normalized version of it.
 *
 * Where the per-CGruntzMgr field type is actually known (tomalla's reconstruction
 * stores most of these as `int` flags), it is used; otherwise the type is a guess
 * marked @todo. We do NOT know the on-disk value types for certain — most read
 * via GetValueDword (DWORD) per Utils::RegistryHelper.
 *
 * This struct is a SCAFFOLD grouping of the value-names; it is not asserted to
 * match any single object's layout in the binary.
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>  /* CString, DWORD */

struct GruntzRegistryConfig
{
    /* --- Audio (master enable flags + volumes) --- */
    int  disableAudio;          // "Disable Audio"
    int  disableSound;          // "Disable Sound"
    int  disableMusic;          // "Disable Music"
    int  disableSoundFonts;     // "Disable SoundFonts"
    int  disableSoundEffectz;   // "Disable Sound Effectz"
    int  soundVolume;           // "Sound Volume"
    int  musicVolume;           // "Music Volume"
    int  voiceVolume;           // "Voice Volume"
    int  sound;                 // "Sound"   (enable flag)
    int  music;                 // "Music"   (enable flag)
    int  voice;                 // "Voice"   (enable flag)

    /* --- Video --- */
    int  resolution;            // "Resolution"  (CGruntzMgr decodes -> width/height)
    int  disableDirectVideoAccess; // "Disable Direct Video Access"
    int  disableFades;          // "Disable Fades"
    int  disableHighQualityMovie;  // "Disable High Quality Movie"
    int  enableEmulation;       // "Enable Emulation"
    int  enableHiColor;         // "Enable HiColor"
    int  enableTrueColor;       // "Enable TrueColor"
    int  enableTriple;          // "Enable Triple"   (triple buffering)

    /* --- Input / scroll --- */
    int  disableJoystick;       // "Disable Joystick"
    int  scrollSpeed;           // "Scroll Speed"

    /* --- Gameplay --- */
    int  easyMode;              // "Easy Mode"
    int  enableCheatzfile;      // "Enable Cheatzfile"   (gates CHEATZ.TXT)
    CString playerName;         // "Player Name"
    CString gameName;           // "Game Name"
    int  defaultMaxGruntz;      // "DefaultMaxGruntz"

    /*
     * --- Per-save / last-used (INDEXED slots) ---
     * These value-names contain a %d/%i — they are read/written per slot index,
     * so they are not single scalar fields. Represented here only as a record of
     * the value-name patterns; the real storage is per-slot arrays. @todo array
     * sizing / slot count unknown.
     */
    // "LastMap"
    // "LastMultiMap"
    // "LastDiff%d"        (per-slot difficulty)
    // "LastColour%d"      (per-slot color)
    // "LastMaxGruntz%d"   (per-slot max gruntz)
    // "Last Warp Level"
    // "Level %i Warp X"
    // "Level %i Warp Y"
    // "SG%i"              (saved-game index)
    // "Saved Game #%i"
    // "File%d"

    /*
     * --- Cheat engine (registry/file backed, see STRINGS_ANALYSIS.md §7) ---
     * Stored alongside options; included for completeness.
     */
    // "Cheatz"  "NonCheat"  "NumCheatz"  "Cheat%i"  "BootyCheatz"
};

#endif /* GRUNTZ_REGISTRY_H */
