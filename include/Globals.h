#ifndef GRUNTZ_GLOBALS_H
#define GRUNTZ_GLOBALS_H

#include <rva.h> // int aliases (i8..u64)
#include <Bute/ButeTree.h>
#include <Gruntz/ScrollState.h> // the auto-scroll state block (was declared inline here)

struct AttractActorList;
struct CVariantSlot;
struct CActEntry;
class CDDrawPtrCollections;
typedef CDDrawPtrCollections CDirectDrawMgr; // one class, both spellings
struct CVariantSlot;
struct CDropEntry;
struct CVariantSlot;
struct CHaznEntry;
struct CKSlimeEntry;
struct CVariantSlot;
struct CPartEntry;
class CImagePool; // g_previewMgr (canonical <Image/ImagePool.h>; was CPreviewMgr view)
struct CProjActEntry;
struct CVariantSlot;
struct CVariantSlot;
struct CTBombEntry;
struct CVariantSlot;
struct CToobEntry;
struct CVariantSlot;
struct CVTrigEntry;
struct SFMANL101TAG;
typedef struct SFMANL101TAG SFMANL101API;
class CDDrawWorkerHost; // g_backView's real class (the CLevelPlane scroll plane)
struct ShadeDescr;

#include <Gruntz/SoundState.h> // g_sndEnabled (sound-on gate) + g_sndCueTag (cue-item id)

extern char g_typeDesc3[];
extern char g_typeDesc1[];
extern char g_typeDesc2[];
extern i32 g_screenTag;
extern const i32 g_msgmap_CBattlezDlgColors;
extern void* g_battlezCustomMsgMap;
extern i32 g_idleSpriteIds[4];
extern float g_secretRatioScale;
extern const i32 g_msgmap_CCheckpointDlg;
extern const float g_diffScale;
extern double g_wingzScale;
extern double g_wingzBias;
extern i32 g_menuSparkleLo;
extern i32 g_menuSparkleHi;
extern const double g_slimeSpeedNum;
extern const double g_objDropDiv;
extern double g_dropFallBias;
extern const double g_projPhase0;
extern const double g_projPhase1;
extern const double c_volScale;
extern const double c_volNum;
extern const double c_powExp;
extern const double c_acosNorm;
extern float g_c24;
extern float g_one;
extern float g_255;
extern float g_p01;
extern float g_lumaR;
extern float g_lumaG;
extern float g_lumaB;
extern float g_inv255;
extern void* g_retAddrBreadcrumb;
extern u32 g_zvecErrSentinel;
extern const double g_motionTimeScale;
extern const double g_motionNegHalf;
extern const double g_motionNegTwo;
extern float g_fxBias;
extern float g_fxEps;
extern char
    g_msgCaption[]; // "Gruntz" @0x60aac8 (def: WinMain.cpp; doubles as GameText's descriptor tag)
extern char s_codeD[];
extern char s_codeF[];
extern char s_codeM[];
extern char s_codeH[];
extern char s_codeN[];
extern char s_codeQ[];
extern char s_codeO[];
extern char k_60df94[];
extern "C" char g_id0_613dff;
extern "C" char g_id1_613e00;
extern "C" char g_id2_613e01;
extern "C" char g_id3_613e02;
extern i32 g_dplayAppGuid[4]; // 0x60fab8  DirectPlay app GUID (DEFINED in src/Gruntz/Multi.cpp)
extern u8 g_titleBuf;
extern char s_GameKey[];
extern i32 g_warpX;
extern i32 g_warpY;
extern i32 g_lastLevelNum;
extern void* g_desc_6156f4;
extern char g_bmpHeaderTemplate[];
extern i32 g_wwdObjIdCounter;
extern "C" i16 g_charClass[];
extern "C" i16 g_transTable[97][49][3];
extern i32 g_posSoundReq;
extern char g_cheatTable[];
extern char g_secretMsgA[0x20];
extern char g_secretMsgB[0x80]; // strB extent 0x80 (SecretMsgRow[24].strB), not 0x20 (DD-G)
extern char g_cheatTableEnd[];
extern i32 g_bootyCheatBuilt;
extern i32 g_stepRun;
extern i32 g_stepCol;
extern i32 g_stepRow;
extern i32 g_diffTier;
extern class CDDrawSurfaceMgr*
    g_dat62c268;    // 0x62c268  the world holder seeded for the custom-world popup
class CVariantSlot; // Bute/ButeTree.h (Set @0x16d850)
extern i32 g_jitterX;
extern i32 g_jitterY;
extern "C" char g_msgScratch[];
extern i32 g_panMinX;
extern i32 g_panMaxX;
extern "C" i32 g_areaHazardParam;
extern "C" i32 g_cdPromptResult;
extern "C" i32 g_levelBias100;
extern i32 g_debugDisplayFlags; // 0x6455f4 debug-overlay flags word (def: GruntzMgr.cpp)
extern i32 g_dlgResultSink;
extern "C" i32 g_activePlayerCount;
extern u32 g_ackThrottleDeadline; // 0x648d14  DEFINED in src/Gruntz/Multi.cpp (owner TU)
extern char g_lobbyRecvBuf[0x800];
extern unsigned char gB_flag;
extern i32 gB_val;
extern i32 gB_m14;
extern i32 gB_e04;
extern unsigned char gB_e08;
extern unsigned char gB_data;
extern unsigned char gA_flag;
extern unsigned char gA_slot;
extern i32 gA_seq;
extern i32 gA_e04;
extern unsigned char gA_e08;
extern unsigned char gA_data;
extern i32 g_poolCount;
extern i32 g_savedMultiWndProc;
extern CDDrawWorkerHost* g_backView; // 0x64c27c the "BACK" scroll plane (CLevelPlane)
extern "C" i32 g_soundChannelInUse[17];
extern u16 g_idx_64da80;
extern u32 g_ratingRaw_64da84;
extern i32 g_factoryRc_64da88;
extern char g_traceBuf_64da90[];
extern u32 g_sfCfgB0;
extern char* g_sfCurPath;
extern u16 g_sfCfgB12;
extern char g_sfMusic4[];
extern char g_ratingBuf_64dbe0[];
extern char g_sfLocal4[];
extern char g_sfMusic[];
extern char g_sfLocal[];
extern u16 g_caps_64df30;
extern u32 g_capsFlags_64df36;
extern char g_capsName_64df46[];
extern u16 g_remaining_64df98;
extern u32 g_id_64df9c;
extern char g_sfDir[];
extern u32 g_sfVer;
extern u16 g_sfDeviceCount;
extern void* g_sfDll;
extern SFMANL101API* g_sfDevice;
extern void* g_sfReady;
extern u8 g_ratings_64e0c0[];
extern i32 g_suppress_64e360;
extern i32 g_loadedFlag;
extern CVariantSlot* g_vtrigColl2;
extern i32 g_vtrigLo;
extern i32 g_vtrigHi;
extern char* g_vtrigBase;
extern CVTrigEntry* g_vtrigCur;
extern i32 g_vtrigStride;
extern i32 g_vtrigScratch;
extern i32 g_panTable[];
class CGameWnd;
extern CGameWnd* g_activeGameWnd;  // was placeholder g_singleton653c68
extern i32 g_gameAppInstanceCount; // was placeholder g_instCount653c6c
extern i32 g_wap32Now;
extern i32 g_wap32FrameDelta;
extern i32 g_wap32ClockReset;
extern i32 g_wap32Run7c;
extern i32 g_wap32Run80;
extern i32 g_imageCacheIndex;
extern u8 g_clut[]; // 3-plane 16bpp LUT; interior slices +0x2/+0x10002/+0x20002 (subsumed banks)
extern u16 g_lut16[256];
extern i32 (*g_restoreHandler)();
extern u8 g_paletteRampBuf[];
extern i32 g_warpU;
extern i32 g_warpV;
extern i32 g_warpTexBase;
extern i32 g_warpUStep;
extern i32 g_warpVStep;
extern i32 g_warpUMask;
extern i16 g_warpColorkey;
extern "C" CDirectDrawMgr* g_DirectDrawMgr;
extern u8 g_scratch[];
extern ShadeDescr* g_shadeDescr208;
extern ShadeDescr* g_shadeDescr20c;
extern ShadeDescr* g_shadeDescr210;
extern ShadeDescr* g_shadeDescr214;
extern ShadeDescr* g_shadeDescr21c;
extern ShadeDescr* g_shadeDescr220;
struct _DDBLTFX;
extern _DDBLTFX
    g_bltFx; // 0x2bf318  the shared BltEx DDBLTFX (dwSize=100; the mirror-flip fx block)
extern i32 g_surfaceColorKey;
extern i32 g_aniParsedNameLen;
extern void* g_projActName;
extern u8 g_zArrayTag;
extern "C" i32 g_hr;
extern "C" i32 g_code;
extern "C" char g_szCode[];
extern "C" char g_szMsg[];
#include <Gruntz/Random.h> // g_randSeeded + g_randSeed (the primary LCG state)
extern float g_zeroF;
extern char* g_colorNames[];
extern char* g_difficultyNames[];
extern char g_mapNamePre[4];
extern char g_mapNameBuf[0x200];
extern i32
    g_groupSentinel; // serialized with the trigger group/selection state (def: TriggerMgr.cpp)
extern char* g_areaNames[];
extern CImagePool* g_previewMgr;
extern void* g_previewImage;
extern i32 g_buildNumber;
extern u8 g_grayRamp[];
extern "C" i32 g_helperRefCount;
extern i32 g_debugPrintMode;

extern "C" {
    extern i32 g_opt_22bd64;
    extern i32 g_opt_22bd68;
    extern i32 g_opt_22bd6c;
    extern i32 g_opt_22bd70;
    extern i32 g_opt_22bd84;
    extern i32 g_opt_22bdc4;
    extern i32 g_opt_22bdc8;
    extern i32 g_opt_22bdcc;
    extern i32 g_opt_22bdd0;
    extern i32 g_opt_22bdd4;
    // The .bute [Config] gate band, 0x6455b4..0x6455e4. DEFINED (storage) in the owner TU
    // src/Rez/RezSync.cpp - RezSync::Init loads all twelve from the [Config] keys quoted
    // here, which is also where the names come from. They used to be `extern`-only under
    // three different namings and NO definition anywhere, so every reference to them was
    // an unresolved external.
    extern i32 g_disableAudio;       // "Disable Audio"
    extern i32 g_disableSound;       // "Disable Sound"
    extern i32 g_disableMusic;       // "Disable Music"
    extern i32 g_disableFades;       // "Disable Fades"
    extern i32 g_disableJoystick;    // "Disable Joystick"
    extern i32 g_disableSoundFonts;  // "Disable SoundFonts"
    extern i32 g_disableDirectVideo; // "Disable Direct Video Access"
    extern i32 g_disableHqMovie;     // "Disable High Quality Movie"
    extern i32 g_enableTriple;       // "Enable Triple"
    extern i32 g_enableHiColor;      // "Enable HiColor"
    extern i32 g_enableTrueColor;    // "Enable TrueColor"
    extern i32 g_enableEmulation;    // "Enable Emulation"
    extern i32 g_profAccA;
    extern i32 g_profAccB;
}


// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern char g_typeDesc1[];
extern i32 g_screenTag;
extern void* g_battlezCustomMsgMap;
extern const double c_volScale;
extern const double c_volNum;
extern const double c_powExp;
extern const double c_acosNorm;
extern const double g_motionNegHalf;
extern "C" i32 g_areaHazardParam;
extern "C" i32 g_levelBias100;
extern "C" i32 g_activePlayerCount;
    // g_opt_22bd64..g_opt_22bdd4 (the options-dialog staging cells, 0x22bd64..0x22bdd4)
    // are DEFINED in their owning TU src/Gruntz/VideoConfig.cpp (videoconfig.obj's .bss);
    // the reference externs stay in <Globals.h>.
    // The [Config] gate band (0x6455b4..0x6455e4) is DEFINED in src/Rez/RezSync.cpp
    // (its owner: Init loads all twelve from the .bute keys that name them).
    // g_profAccA/g_profAccB DEFINED in src/Gruntz/Play.cpp (owner TU).

#endif // GRUNTZ_GLOBALS_H
