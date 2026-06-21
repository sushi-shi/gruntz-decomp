#ifndef GAME_CGRUNTZMGR_H
#define GAME_CGRUNTZMGR_H

/*
 * CGruntzMgr — the central Gruntz game manager (CGameMgr subclass).
 * .?AVCGruntzMgr@@  (size 0xa30)
 *
 * Leaked source TU: C:\Proj\Gruntz\GruntzMgr.cpp
 *
 * This is the MOST-RECONSTRUCTED game class. Layout ported from tomalla (@approx
 * tomalla 1.0.1.77; field offsets = high confidence, most field NAMES past the
 * config block are placeholders). The named option fields (m_isVoiceEnabled,
 * m_soundVolume, m_scrollSpeed, …) and the static is_*_disabled/is_*_enabled
 * globals map 1:1 to the registry value-names (see registry.h).
 *
 * NOT yet graduated to src/. External dependencies are modeled as compilable
 * placeholders (no <afxwin.h>/<dplobby.h>): CString -> a 4-byte MgrCString;
 * DirectPlay lobby/connection pointers -> void*; CPtrArray/CDWordArray -> 0x14-byte
 * placeholder structs; Utils::RegistryHelper -> a forward decl (held by pointer).
 */

#include <rva.h>                // OVERRIDE macro (override under clang, no-op under MSVC 5.0)
#include "memory_pool.h"        // Utils::MemoryPool<T>  (memory_pool static)
#include "font.h"               // Font (font_large/medium/small/tiny statics)

// WAP32::CGameMgr base (0x2c). The authoritative layout now lives in src/
// (<Wap32/Wap32.h>), which wins on any name overlap, so the engine's separate
// comprehension cgamemgr.h was dropped. cgruntzmgr.h is still an ungraduated
// comprehension header, though, and must parse STANDALONE under the struct-gen
// plain driver (no -I include / no MFC paths) - so the base is modeled here as a
// self-contained placeholder (same doctrine as MgrCString/CPtrArray below): the
// polymorphic 0x2c shape CGruntzMgr derives from, carrying the five engine
// virtuals CGruntzMgr overrides.
namespace WAP32
{
    class CGameApp;
    class CGameWnd;

    class CGameMgr
    {
    public:
        CGameMgr();
        virtual ~CGameMgr();                  // vtbl +0x00 (vector deleting dtor)
        virtual int  UnknownVirtualMethod1(CGameWnd *pGameWnd, char *szCmdLine); // +0x04
        virtual void UnknownClose();           // +0x08
        virtual void UnknownVirtualMethod3();  // +0x0c
        virtual void UnknownVirtualMethod4();  // +0x10
        virtual void UnknownVirtualMethod5();  // +0x14

        CGameWnd *m_pGameWnd;        // +0x04
        CGameApp *m_pGameApp;        // +0x08
        int       fieldUnknown00C;   // +0x0c
        int       m_isSoundEnabled;  // +0x10
        int       m_isMusicEnabled;  // +0x14
        int       fieldUnknown018;   // +0x18
        int       fieldUnknown01C;   // +0x1c
        int       fieldUnknown020;   // +0x20
        int       fieldUnknown024;   // +0x24
        char      _pad28[0x2c - 0x28]; // +0x28  (tail pad to 0x2c)
    };
}

// CString in the binary is an MFC type (a single char*); modeled as a 4-byte
// placeholder so the header parses without <afxwin.h>.
typedef void *MgrCString;

// MFC array placeholders — only their SIZE (0x14) is load-bearing here.
struct CPtrArray   { void *_v; char _raw[0x14 - 4]; };  // 0x14
struct CDWordArray { void *_v; char _raw[0x14 - 4]; };  // 0x14

namespace Utils { class RegistryHelper; }  // held by pointer @0x38

// Element type of CGruntzMgr::memory_pool (Utils::MemoryPool<Pair>).
struct Pair
{
    int a;   // +0x00
    int b;   // +0x04
};           // 0x08

// Forward decl for the surface/page-manager family root held at @0x30
// (HYPOTHESIZED CDirectDrawMgr — see ddrawmgr_surface_family.h).
class UnknownClassCGruntzMgrHarryPotter;

/*
 * UnknownClassArrays — nested array bundle inside UnknownClassInCGruntzMgr.
 * Ported from tomalla; almost entirely unknown (padding + int blocks + four MFC
 * arrays). No vtable. Size 0x144.
 */
class UnknownClassArrays
{
public:
    UnknownClassArrays();
    ~UnknownClassArrays();
    void FreeArrays();

    char _pad000[0x18];          // +0x000
    int  fieldUnknown018, fieldUnknown01C, fieldUnknown020, fieldUnknown024; // +0x18..
    int  fieldUnknown028, fieldUnknown02C, fieldUnknown030;                  // ..+0x33
    char _pad034[0x48 - 0x34];   // +0x034
    int  fieldUnknown048, fieldUnknown04C, fieldUnknown050; // +0x48..
    int  fieldUnknown054, fieldUnknown058, fieldUnknown05C; // ..+0x5f
    char _pad060[0x74 - 0x60];   // +0x060
    int  fieldUnknown074, fieldUnknown078, fieldUnknown07C, fieldUnknown080; // +0x74..0xcf
    int  fieldUnknown084, fieldUnknown088, fieldUnknown08C, fieldUnknown090;
    int  fieldUnknown094, fieldUnknown098, fieldUnknown09C, fieldUnknown0A0;
    int  fieldUnknown0A4, fieldUnknown0A8, fieldUnknown0AC, fieldUnknown0B0;
    int  fieldUnknown0B4, fieldUnknown0B8, fieldUnknown0BC, fieldUnknown0C0;
    int  fieldUnknown0C4, fieldUnknown0C8, fieldUnknown0CC;
    char _pad0D0[0x0C];          // +0x0d0
    CPtrArray   unknownMemoryPoolPointers1; // +0x0dc
    CPtrArray   unknownMemoryPoolPointers2; // +0x0f0
    CDWordArray unknownDwordArray1;         // +0x104
    CDWordArray unknownDwordArray2;         // +0x118
    char _pad12C[0x10];          // +0x12c
    int  fieldUnknown13C;        // +0x13c
    int  fieldUnknown140;        // +0x140
};                               // 0x144

/*
 * UnknownClassInCGruntzMgr — a 0x238-byte sub-object; CGruntzMgr holds an array
 * of 4 (one per player?). Ported from tomalla. No vtable.
 */
class UnknownClassInCGruntzMgr
{
public:
    UnknownClassInCGruntzMgr();
    ~UnknownClassInCGruntzMgr();
    void ResetFields();

    int        fieldUnknown000;         // +0x000
    MgrCString strUnknownString004;     // +0x004
    int        fieldUnknown008;         // +0x008
    char       _pad00C[4];              // +0x00c
    int        fieldUnknown010, fieldUnknown014, fieldUnknown018; // +0x010..0x1b
    char       _pad01C[4];              // +0x01c
    int        fieldUnknown020;         // +0x020
    char       _pad024[4];              // +0x024
    int        fieldUnknown028, fieldUnknown02C, fieldUnknown030; // +0x028..0x33
    char       _pad034[4];              // +0x034
    UnknownClassArrays unknownObjectArrays; // +0x038 (0x144)
    char       _pad17C[0xA4];           // +0x17c
    int        fieldUnknown220, fieldUnknown224, fieldUnknown228; // +0x220..
    int        fieldUnknown22C, fieldUnknown230;                  // ..0x233
    char       _pad234[4];              // +0x234
};                                      // 0x238

class CGruntzMgr : public WAP32::CGameMgr
{
public:
    CGruntzMgr();
    virtual ~CGruntzMgr() OVERRIDE;               // vtbl +0x00 (vector deleting dtor)
    virtual int  UnknownVirtualMethod1(WAP32::CGameWnd *pGameWnd, char *szCmdLine) OVERRIDE; // +0x04
    virtual void UnknownClose() OVERRIDE;          // +0x08
    virtual void UnknownVirtualMethod3() OVERRIDE;  // +0x0c
    virtual void UnknownVirtualMethod4() OVERRIDE;  // +0x10
    virtual void UnknownVirtualMethod5() OVERRIDE;  // +0x14

    void ReportError(int errorMessageId, int errorCode);

    // --- own fields begin after the CGameMgr base (which ends at 0x2c) ---
    int fieldUnknown02C;     // +0x2c  pointer to current game state (a CState subclass)
    // +0x30  HYPOTHESIZED CDirectDrawMgr family root (constructed during init;
    //        its UnknownVirtualMethod18 does the 640x480x16 display-mode init).
    UnknownClassCGruntzMgrHarryPotter *fieldUnknown030_maybeSurfaceRestoreHandler; // +0x30
    int fieldUnknown034;     // +0x34
    Utils::RegistryHelper *m_pRegistryHelper; // +0x38
    int fieldUnknown03C, fieldUnknown040, fieldUnknown044, fieldUnknown048; // +0x3c..
    int fieldUnknown04C, fieldUnknown050, fieldUnknown054, fieldUnknown058;
    int fieldUnknown05C, fieldUnknown060, fieldUnknown064, fieldUnknown068;
    int fieldUnknown06C, fieldUnknown070, fieldUnknown074, fieldUnknown078;
    int fieldUnknown07C;     // ..+0x80
    int m_numRuns;           // +0x80
    int m_numMovies;         // +0x84
    int fieldUnknown088, fieldUnknown08C, fieldUnknown090; // +0x88..0x93
    int m_resolutionWidth;   // +0x94
    int m_resolutionHeight;  // +0x98
    int m_unknownIsLobbyConnectionSettingsInitialized;  // +0x9c (bool)
    int m_unknownIsLobbyConnectionSettingsAttempted;    // +0xa0 (bool)
    int fieldUnknown0A4, fieldUnknown0A8, fieldUnknown0AC, fieldUnknown0B0, fieldUnknown0B4; // +0xa4..0xb7
    int m_isCheckpointPrompts; // +0xb8  "Checkpoint Prompts"
    int fieldUnknown0BC;     // +0xbc
    void *m_pDirectPlayLobby;       // +0xc0  LPDIRECTPLAYLOBBYA
    void *m_pDirectPlayConnection;  // +0xc4  DPLCONNECTION*
    MgrCString strUnknownString0C8; // +0xc8
    int fieldUnknown0CC;     // +0xcc
    char m_driveLetter;      // +0xd0
    char _padD1[0xd4 - 0xd1];// +0xd1
    int  m_isDriveLetterLoaded; // +0xd4 (bool)
    CPtrArray  aUnknownPtrArray0D8; // +0xd8 (0x14)
    MgrCString strUnknownString0EC; // +0xec
    MgrCString strUnknownString0F0; // +0xf0
    int fieldUnknown0F4, fieldUnknown0F8, fieldUnknown0FC; // +0xf4..0xff
    int m_isVoiceEnabled;    // +0x100
    int m_isAmbientEnabled;  // +0x104  "Ambient"
    int m_isInterlaced;      // +0x108  "Interlaced"
    int m_isHighDetail;      // +0x10c  "High Detail"
    int m_unknownSecondIsHighDetail; // +0x110
    int fieldUnknown114;     // +0x114
    int m_isEasyMode;        // +0x118  "Easy Mode"
    int m_soundVolume;       // +0x11c  "Sound Volume"
    int m_voiceVolume;       // +0x120  "Voice Volume"
    int m_scrollSpeed;       // +0x124  "Scroll Speed"
    int fieldUnknown128, fieldUnknown12C, fieldUnknown130, fieldUnknown134, fieldUnknown138; // +0x128..0x13b
    char _pad13C[0x14];      // +0x13c
    UnknownClassInCGruntzMgr aUnknownObjs[4]; // +0x150 (4 * 0x238 = 0x8e0)

    //@address: 00646498  (init 0x482ec0 / dtor 0x482f10)
    static Utils::MemoryPool<Pair> memory_pool;

    //@address: 0064fa18 / 0064fa40 / 0064fa58 / 0064f9b0
    static Font font_large, font_medium, font_small, font_tiny;

    // --- option globals (1:1 with registry value-names; see registry.h) ---
    static bool is_high_quality_movie_enabled;  // "Disable High Quality Movie" (inverted)
    static int  is_audio_disabled;               // "Disable Audio"
    static int  is_sound_disabled;               // "Disable Sound"
    static int  is_music_disabled;               // "Disable Music"
    static int  is_fades_disabled;               // "Disable Fades"
    static int  is_direct_video_access_disabled; // "Disable Direct Video Access"
    static int  is_joystick_disabled;            // "Disable Joystick"
    static int  is_soundfonts_disabled;          // "Disable SoundFonts"
    static int  is_triple_enabled;               // "Enable Triple"
    static int  is_hicolor_enabled;              // "Enable HiColor"
    static int  is_truecolor_enabled;            // "Enable TrueColor"
    static int  is_emulation_enabled;            // "Enable Emulation"

private:
    static bool is_font_initialized;

    bool InitializeFonts();
    void DecodeResolution(int resolution, int &width, int &height);
    char GetGruntzDriveLetter();
    bool InitializeLobbyConnectionSettings();
};                           // 0xa30

#endif /* GAME_CGRUNTZMGR_H */
