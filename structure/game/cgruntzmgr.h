#ifndef GAME_CGRUNTZMGR_H
#define GAME_CGRUNTZMGR_H

/*
 * CGruntzMgr — the central Gruntz game manager (CGameMgr subclass).
 * .?AVCGruntzMgr@@
 *
 * Leaked source TU: C:\Proj\Gruntz\GruntzMgr.cpp
 *
 * This is the MOST-RECONSTRUCTED game class. LAYOUT PORTED FROM tomalla
 * (refs/tomalla-gruntz/gruntz/cgruntzmgr.h), attributed and kept intact.
 * @address values are for the PATCHED build 1.0.1.77 — approximate for v1.0.0.76;
 * re-verify before matching. Field offsets = high confidence; most field NAMES
 * past the config block are tomalla "fieldUnknownNNN" placeholders.
 *
 * The named option fields (m_isVoiceEnabled, m_soundVolume, m_scrollSpeed, …) and
 * the static is_*_disabled / is_*_enabled globals map 1:1 to the registry
 * value-names (see ../registry.h).
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>
#include <dplobby.h>

#include "../wap32/cgamemgr.h"
#include "../utils/registry_helper.h"   // Utils::RegistryHelper (m_pRegistryHelper)
#include "../utils/memory_pool.h"       // Utils::MemoryPool<T>  (memory_pool)
#include "../utils/font.h"              // Font (font_large/medium/small/tiny)

/* --- helper types ported from tomalla --- */

// Element type of CGruntzMgr::memory_pool (Utils::MemoryPool<Pair>).
struct Pair
{
    int a;
    int b;
};

/*
 * Forward decl for the surface/page-manager family root held at @0x30.
 * HYPOTHESIZED CDirectDrawMgr — see ../managers/ddrawmgr_surface_family.h
 * (tomalla's "harry_potter" hierarchy; class identity is a HYPOTHESIS).
 */
class UnknownClassCGruntzMgrHarryPotter;

/*
 * UnknownClassArrays — nested array bundle inside UnknownClassInCGruntzMgr.
 * Ported verbatim from tomalla; almost entirely @todo (padding + int blocks +
 * four MFC arrays). Largest referenced offset ~0x144.
 */
class UnknownClassArrays
{
public:
    //@size: ? (@todo: perhaps 0x144 — largest offset referenced in the ctor)

    UnknownClassArrays();
    ~UnknownClassArrays();
    void FreeArrays();

    //@offset: 0
    char _padding1[0x18];
    //@offset: 18
    int fieldUnknown018;
    int fieldUnknown01C, fieldUnknown020, fieldUnknown024, fieldUnknown028;
    int fieldUnknown02C, fieldUnknown030;
    //@offset: 34
    char _padding2[0x14];
    //@offset: 48
    int fieldUnknown048, fieldUnknown04C, fieldUnknown050;
    int fieldUnknown054, fieldUnknown058, fieldUnknown05C;
    //@offset: 60
    char _padding3[0x14];
    //@offset: 74 .. cc  (block of ints, all @todo)
    int fieldUnknown074, fieldUnknown078, fieldUnknown07C, fieldUnknown080;
    int fieldUnknown084, fieldUnknown088, fieldUnknown08C, fieldUnknown090;
    int fieldUnknown094, fieldUnknown098, fieldUnknown09C, fieldUnknown0A0;
    int fieldUnknown0A4, fieldUnknown0A8, fieldUnknown0AC, fieldUnknown0B0;
    int fieldUnknown0B4, fieldUnknown0B8, fieldUnknown0BC, fieldUnknown0C0;
    int fieldUnknown0C4, fieldUnknown0C8, fieldUnknown0CC;
    //@offset: d0
    char _padding4[0x0C];
    //@offset: dc (size: 0x14)
    CPtrArray unknownMemoryPoolPointers1;
    //@offset: f0
    CPtrArray unknownMemoryPoolPointers2;
    //@offset: 104
    CDWordArray unknownDwordArray1;
    //@offset: 118
    CDWordArray unknownDwordArray2;
    //@offset: 12c
    char _padding5[0x10];
    //@offset: 13c
    int fieldUnknown13C;
    //@offset: 140
    int fieldUnknown140;
};

/*
 * UnknownClassInCGruntzMgr — a 0x238-byte sub-object; CGruntzMgr holds an array
 * of 4 (one per player?). Ported from tomalla.
 */
class UnknownClassInCGruntzMgr
{
public:
    //@size: 0x238

    UnknownClassInCGruntzMgr();
    ~UnknownClassInCGruntzMgr();
    void ResetFields();

    //@offset: 0
    int fieldUnknown000;
    //@offset: 4
    CString strUnknownString004;
    //@offset: 8
    int fieldUnknown008;
    //@offset: c
    char _padding1[4];
    //@offset: 10
    int fieldUnknown010, fieldUnknown014, fieldUnknown018;
    //@offset: 1c
    char _padding2[4];
    //@offset: 20
    int fieldUnknown020;
    //@offset: 24
    char _padding3[4];
    //@offset: 28
    int fieldUnknown028, fieldUnknown02C, fieldUnknown030;
    //@offset: 34
    char _padding4[4];
    //@offset: 38
    UnknownClassArrays unknownObjectArrays;
    //@offset: 17c
    char _padding9[0xA4];
    //@offset: 220
    int fieldUnknown220, fieldUnknown224, fieldUnknown228, fieldUnknown22C, fieldUnknown230;
    //@offset: 234
    char _padding5[4];
};

class CGruntzMgr : public WAP32::CGameMgr
{
public:
    //@size: a30
    CGruntzMgr();
    virtual ~CGruntzMgr();

    //@vftable: 0  vector deleting destructor (-> @address: 00483250)
    //@vftable: 4
    virtual bool UnknownVirtualMethod1(WAP32::CGameWnd* pGameWnd, char* szCmdLine);
    //@vftable: 8
    virtual void UnknownClose();
    //@vftable: C
    virtual void UnknownVirtualMethod3();
    //@vftable: 10
    virtual void UnknownVirtualMethod4();
    //@vftable: 14
    virtual void UnknownVirtualMethod5();

    void ReportError(int errorMessageId, int errorCode);

    //@offset: 0   CGameMgr base (0x2c)

    //@offset: 2c
    int fieldUnknown02C; // @todo: pointer to current game state (a CState subclass)
    //@offset: 30  HYPOTHESIZED CDirectDrawMgr family root (constructed during init;
    //             its UnknownVirtualMethod18 does the 640x480x16 display-mode init).
    //             See ../managers/ddrawmgr_surface_family.h.
    UnknownClassCGruntzMgrHarryPotter* fieldUnknown030_maybeSurfaceRestoreHandler;
    //@offset: 34
    int fieldUnknown034;
    //@offset: 38
    Utils::RegistryHelper* m_pRegistryHelper;
    //@offset: 3c
    int fieldUnknown03C, fieldUnknown040, fieldUnknown044, fieldUnknown048;
    int fieldUnknown04C, fieldUnknown050, fieldUnknown054, fieldUnknown058;
    int fieldUnknown05C, fieldUnknown060, fieldUnknown064, fieldUnknown068;
    int fieldUnknown06C, fieldUnknown070, fieldUnknown074, fieldUnknown078;
    int fieldUnknown07C;
    //@offset: 80
    int m_numRuns;
    //@offset: 84
    int m_numMovies;
    //@offset: 88   (ctor default fieldUnknown088 = 16  @approx tomalla 1.0.1.77)
    int fieldUnknown088, fieldUnknown08C, fieldUnknown090;
    //@offset: 94
    int m_resolutionWidth;
    //@offset: 98
    int m_resolutionHeight;
    //@offset: 9c
    bool m_unknownIsLobbyConnectionSettingsInitialized;
    //@offset: a0
    bool m_unknownIsLobbyConnectionSettingsAttempted;
    //@offset: a4
    int fieldUnknown0A4, fieldUnknown0A8, fieldUnknown0AC, fieldUnknown0B0, fieldUnknown0B4;
    //@offset: b8   "Checkpoint Prompts" (ctor default 1  @approx tomalla 1.0.1.77)
    int m_isCheckpointPrompts;
    //@offset: bc
    int fieldUnknown0BC;
    //@offset: c0
    LPDIRECTPLAYLOBBYA m_pDirectPlayLobby;
    //@offset: c4
    DPLCONNECTION* m_pDirectPlayConnection;
    //@offset: c8
    CString strUnknownString0C8;
    //@offset: cc   (ctor default fieldUnknown0CC = 30  @approx tomalla 1.0.1.77)
    int fieldUnknown0CC;
    //@offset: d0
    char m_driveLetter;
    //@offset: d4
    bool m_isDriveLetterLoaded;
    //@offset: d8
    CPtrArray aUnknownPtrArray0D8;
    //@offset: ec
    CString strUnknownString0EC;
    //@offset: f0
    CString strUnknownString0F0;
    //@offset: f4   (ctor default fieldUnknown0F4 = 1  @approx tomalla 1.0.1.77)
    int fieldUnknown0F4, fieldUnknown0F8, fieldUnknown0FC;
    //@offset: 100  (ctor default 1 — all enable flags default enabled)
    int m_isVoiceEnabled;
    //@offset: 104  "Ambient"  (ctor default 1)
    int m_isAmbientEnabled;
    //@offset: 108  "Interlaced"  (ctor default 0)
    int m_isInterlaced;
    //@offset: 10c  "High Detail"  (ctor default 1)
    int m_isHighDetail;
    //@offset: 110  (ctor default 1 — second high-detail flag)
    int m_unknownSecondIsHighDetail;
    //@offset: 114
    int fieldUnknown114;
    //@offset: 118  "Easy Mode"  (ctor default 0)
    int m_isEasyMode;
    //@offset: 11c  "Sound Volume"  (registry default 60)
    int m_soundVolume;
    //@offset: 120  "Voice Volume"  (registry default 80)
    int m_voiceVolume;
    //@offset: 124  "Scroll Speed"  (registry default 20)
    int m_scrollSpeed;
    //@offset: 128   (ctor default fieldUnknown138 = 3  @approx tomalla 1.0.1.77)
    int fieldUnknown128, fieldUnknown12C, fieldUnknown130, fieldUnknown134, fieldUnknown138;
    //@offset: 13c
    char _padding4[0x14];
    //@offset: 150
    UnknownClassInCGruntzMgr aUnknownObjs[4];

    //@address: 00646498 / 00482ec0 (init) / 00482f10 (dtor)
    static Utils::MemoryPool<Pair> memory_pool;

    //@address: 0064fa18 / 005156e0 (init) / 00515720 (dtor)
    static Font font_large;
    //@address: 0064fa40
    static Font font_medium;
    //@address: 0064fa58
    static Font font_small;
    //@address: 0064f9b0
    static Font font_tiny;

    /* --- option globals (1:1 with registry value-names; see ../registry.h) --- */
    //@address: 0064652c
    static bool is_high_quality_movie_enabled;  // "Disable High Quality Movie" (inverted)
    //@address: 0064650c
    static int is_audio_disabled;               // "Disable Audio"
    //@address: 00646514
    static int is_sound_disabled;               // "Disable Sound"
    //@address: 00646518
    static int is_music_disabled;               // "Disable Music"
    //@address: 0064651c
    static int is_fades_disabled;               // "Disable Fades"
    //@address: 00646528
    static int is_direct_video_access_disabled; // "Disable Direct Video Access"
    //@address: 00646520
    static int is_joystick_disabled;            // "Disable Joystick"
    //@address: 00646524
    static int is_soundfonts_disabled;          // "Disable SoundFonts"
    //@address: 00646530
    static int is_triple_enabled;               // "Enable Triple"
    //@address: 00646534
    static int is_hicolor_enabled;              // "Enable HiColor"
    //@address: 00646538
    static int is_truecolor_enabled;            // "Enable TrueColor"
    //@address: 0064653c
    static int is_emulation_enabled;            // "Enable Emulation"

    //@address: 006461c4 .. 006464bc  (unidentified globals, @todo)
    static int unknown_global_006461c4;
    static int unknown_global_00646228;
    static int unknown_global_006461c0;
    static int unknown_global_006464c0;
    static int unknown_global_00646490;
    static int unknown_global_006460fc;
    static int unknown_global_0064622c;
    static int unknown_global_00646200;
    static int unknown_global_006464b0;
    static int unknown_global_006464b8;
    static int unknown_global_006464b4;
    static int unknown_global_006464bc;

private:
    //@address: 0064fa6c
    static bool is_font_initialized;

    bool InitializeFonts();
    void DecodeResolution(int resolution, int& width, int& height);
    char GetGruntzDriveLetter();
    bool InitializeLobbyConnectionSettings();
};

#endif /* GAME_CGRUNTZMGR_H */
