#ifndef GRUNTZ_REGISTRY_H
#define GRUNTZ_REGISTRY_H

/*
 * Registry-backed configuration.
 *
 * Gruntz reads/writes its options at HKLM\Software\Monolith Productions\Gruntz\1.0
 * (RegQueryValueExA reads; tomalla's Utils::RegistryHelper shows the key is opened
 * with RegCreateKeyExA — i.e. CREATE-or-open, not read-only — and values are also
 * written back via SetValueDword. See STRINGS_ANALYSIS.md §2 and the graduated
 * src/Utils/RegistryHelper.h).
 * Each value-name below maps 1:1 to a CGruntzMgr / options field. The raw
 * registry value-name string is kept verbatim in a comment; the C++ field name
 * is a normalized version of it.
 *
 * Where the per-CGruntzMgr field type is actually known (tomalla's reconstruction
 * stores most of these as `int` flags), it is used; otherwise the type is a guess.
 * We do NOT know the on-disk value types for certain — most read via
 * GetValueDword (DWORD) per Utils::RegistryHelper.
 *
 * This struct is a SCAFFOLD grouping of the value-names; it is NOT asserted to
 * match any single object's layout in the binary (no offsets are claimed). The
 * string fields are MFC CString in the binary, modeled here as a 4-byte pointer
 * placeholder so the header parses without <afxwin.h>.
 */

typedef void* CfgCString;

struct GruntzRegistryConfig {
    /* --- Audio (master enable flags + volumes) --- */
    int disableAudio;        // "Disable Audio"
    int disableSound;        // "Disable Sound"
    int disableMusic;        // "Disable Music"
    int disableSoundFonts;   // "Disable SoundFonts"
    int disableSoundEffectz; // "Disable Sound Effectz"
    int ambient;             // "Ambient"   (ambient-sound enable flag)
    int soundVolume;         // "Sound Volume"   (default 60)
    int musicVolume;         // "Music Volume"   (default 100)
    int voiceVolume;         // "Voice Volume"   (default 80)
    int sound;               // "Sound"   (enable flag)
    int music;               // "Music"   (enable flag)
    int voice;               // "Voice"   (enable flag)

    /* --- Video --- */
    int resolution;               // "Resolution"  (default 1; decoded -> width/height)
    int disableDirectVideoAccess; // "Disable Direct Video Access"
    int disableFades;             // "Disable Fades"
    int disableHighQualityMovie;  // "Disable High Quality Movie"
    int enableEmulation;          // "Enable Emulation"
    int enableHiColor;            // "Enable HiColor"
    int enableTrueColor;          // "Enable TrueColor"
    int enableTriple;             // "Enable Triple"   (triple buffering)
    int highDetail;               // "High Detail"   (default 1)
    int interlaced;               // "Interlaced"    (default 0)

    /* --- Input / scroll --- */
    int disableJoystick; // "Disable Joystick"
    int scrollSpeed;     // "Scroll Speed"   (default 20)

    /* --- Gameplay --- */
    int easyMode;          // "Easy Mode"     (default 0)
    int checkpointPrompts; // "Checkpoint Prompts"   (default 1)
    int enableCheatzfile;  // "Enable Cheatzfile"   (gates CHEATZ.TXT)
    CfgCString playerName; // "Player Name"
    CfgCString gameName;   // "Game Name"
    int defaultMaxGruntz;  // "DefaultMaxGruntz"

    /* --- Install / session bookkeeping --- */
    CfgCString cdRomDrive; // "CdRom Drive"   (cached CD drive letter, string)
    int numRuns;           // "Num Runs"      (times the game has been launched)
    int numMovies;         // "Num Movies"    (intro/movie playback counter)

    /*
     * --- Per-save / last-used (INDEXED slots) ---
     * These value-names contain a %d/%i — they are read/written per slot index,
     * so they are not single scalar fields. Represented here only as a record of
     * the value-name patterns; the real storage is per-slot arrays (array sizing /
     * slot count unknown).
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
