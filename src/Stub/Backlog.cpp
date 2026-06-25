#include <Win32.h> // ShowCursor (matching-neutral; ApiCallers.cpp in this aggregate already pulls it)
#include <stdio.h> // engine sprintf (reloc-masked) - LoadGruntzPalette

#include <rva.h>
// Backlog.cpp - engine-label stubs without a class attribution.

// Folded engine-label stubs with a known owning class.
// A named asset-namespace registry slot (BootyState's m_28/m_2c/m_30). Lookup()
// finds a child set by name and returns a handle (reloc-masked __thiscall).
struct BootyNamespace {
    i32 Lookup(char* szName); // FUN_0013bae0 __thiscall
};
// The registrar object reached via BootyState::m_c->m_10. Its vtable slot +0x4c
// registers a looked-up image set under a prefix (returns -1 on failure). The
// slot is a __thiscall virtual; modeled as a pointer-to-member loaded from the
// vtable so MSVC emits `mov edx,[ecx]; call [edx+0x4c]`. The class must be
// COMPLETE before the T::* typedef so the PMF stays 4 bytes (no this-adjustor) -
// see docs/patterns/pmf-complete-class-4byte.md.
struct BootyRegistrarVtbl; // opaque; the PMF lives at offset 0x4c inside it
struct BootyRegistrar {
    BootyRegistrarVtbl* m_vtbl; // +0x00
    i32 CallRegister(i32 handle, char* prefix, char* sep);
};
typedef i32 (BootyRegistrar::*BootyRegFn)(i32 handle, char* prefix, char* sep);
struct BootyRegistrarVtbl {
    char m_pad00[0x4c];
    BootyRegFn Register; // +0x4c
};
inline i32 BootyRegistrar::CallRegister(i32 handle, char* prefix, char* sep) {
    return (this->*(m_vtbl->Register))(handle, prefix, sep);
}
struct CGruntDataLoader { // BootyState::m_c->m_4 sub-object
    void Load();          // FUN @ 0x158ee0 __thiscall (reloc-masked)
};
struct BootyAssetRoot { // BootyState::m_c
    char m_pad00[0x4];
    CGruntDataLoader* m_4; // +0x04
    char m_pad08[0x10 - 0x8];
    BootyRegistrar* m_10; // +0x10  vtable-bearing registrar (slot +0x4c)
};
class BootyState {
public:
    void vfunc_9(i32);
    i32 OnActivate_vfunc8();
    i32 BaseOnActivate();                                             // base vfunc8 (reloc-masked)
    i32 RegisterMultiNamespaces(char* mode, i32, i32, i32, i32, i32); // FUN @ 0x1e60
    void OnActivated();                                               // FUN @ 0x3fdf (reloc-masked)
    void StartTimer(i32, i32, i32, i32);                              // FUN @ 0x1843 (reloc-masked)

    char m_pad00[0xc];
    BootyAssetRoot* m_c; // +0x0c
    char m_pad10[0x28 - 0x10];
    BootyNamespace* m_28; // +0x28  "LEVEL" image namespace
    BootyNamespace* m_2c; // +0x2c  "BOOTY" image namespace
    BootyNamespace* m_30; // +0x30  "GRUNTZ" image namespace
};
// (class ButeMgr removed - its only method ParseAttributeFile @0x170750 graduated
// to src/Bute/ButeMgr.cpp; see the migration note further down.)
// The help-screen game state. LoadAssets() first chains the base-class asset
// loader (LoadGameAssetNamespaces, reloc-masked external call) which populates
// m_4/m_8, forces the cursor hidden, registers the "STATEZ_HELP" namespace
// through m_8 (cached at m_2c), then pumps a fixed message burst through
// m_4->m_4. Only offsets / code bytes are load-bearing.
struct CHelpAssetSet {          // m_8 points here
    void* Register(char* name); // FUN_0053c030, __thiscall, returns the registered object
};
struct CHelpMsgPump {          // m_4->m_4 points here
    void Pump(i32 msg, i32 n); // FUN_0053d4e0, __thiscall message burst
};
struct CHelpAssetRoot { // m_4 / arg1 points here
    char m_pad00[0x4];
    CHelpMsgPump* m_4; // +0x4
};
class CHelpState {
public:
    i32 LoadAssets(i32, i32, i32);
    i32 LoadGameAssetNamespaces(i32, i32, i32); // base loader; reloc-masked external call

    char m_pad00[0x4];
    CHelpAssetRoot* m_4; // +0x4
    CHelpAssetSet* m_8;  // +0x8
    char m_pad0c[0x2c - 0xc];
    void* m_2c; // +0x2c  the registered "STATEZ_HELP" object (0 on failure)
};
class CloudHazard {
public:
    void vfunc_20(i32, i32);
};
class GameLevelState {
public:
    void OnActivate_vfunc8();
};
class Projectile {
public:
    void vfunc_9();
};
class SFManager {
public:
    void SelectBestDevice();
};
class StatusBarItem {
public:
    void vfunc_12(i32, i32);
    void vfunc_16(i32, i32, i32);
};
// The icon-key registry object BuildPowerupIconKeys writes into (arg1=esi).
// SetGroup() seeds the "GAME_INGAMEICONZ" group key, Add() appends one entry.
// Both __thiscall, reloc-masked externals.
class PowerupKeyRegistry {
public:
    void SetGroup(char* key); // FUN_001b9e74 __thiscall
    void Add(char* key);      // FUN_001ba0c8 __thiscall
};

// MFC CString modeled by its GetBuffer/ReleaseBuffer (reloc-masked __thiscall
// externals) for the COM-registry helper Stub_1bf702. (engine_label_stubs pulls
// <windows.h> first, so the real <afx.h> can't follow.)
struct EngCString {
    char* m_pszData;
    char* GetBuffer(i32 nMinBufLength); // CString::GetBuffer, FUN_001ba11c
    void ReleaseBuffer(i32 nNewLength); // CString::ReleaseBuffer, FUN_001ba16b
};

// CConfigStore - the engine's registry-or-INI config wrapper. When m_7c (a
// registry subkey path under HKCU\Software\...) is set the value getters go
// through ADVAPI32!Reg*; when it is null they fall back to GetPrivateProfile*
// against the INI file named by m_90. Only offsets are load-bearing.
class CConfigStore {
public:
    HKEY OpenRoot();              // Stub_1d4ee3  __thiscall, opens HKCU\Software\<m_7c>
    HKEY OpenSubKey(char* szSub); // Stub_1d4f77  __thiscall
    i32 GetInt(char* szSection, char* szKey, i32 nDefault); // Stub_1d4fbd

    char m_pad00[0x7c];
    char* m_7c; // +0x7c  registry subkey path (null -> use INI)
    char m_pad80[0x90 - 0x80];
    char* m_90; // +0x90  INI file path (used in INI fallback)
};

namespace EngineLabelBacklog {

    void CreateGameObjectByName();
    void __stdcall LoadBootyCheatState(i32, i32, i32);
    void ShowSecretBonusMessage();
    void BuildGruntSprintAnimation();
    void UpdateBootyWalkingGruntz();
    void BuildBootyPerfectAnimation();
    void ShowLevelCompleteMessage();
    void BuildBootyGruntIdleAnimation();
    void __stdcall BuildPowerupIconKeys(PowerupKeyRegistry* reg, i32 key);
    void DrawBattleStats();
    void StartUpPrompt();
    i32 Stub_01fd70(char* szPath);
    void LoadCheatConfig();
    void LoadCustomWorldInfo();
    void HandleFortConquered();
    void __stdcall LoadVehicleGruntSprites(i32);
    void __stdcall WireTileSwitchLogic(i32, i32, i32);
    void __stdcall LoadTerrainTileSprites(i32, i32, i32, i32, i32, i32);
    void LoadCameraSprite();
    void __stdcall LoadToyBoxIcon(i32, i32, i32, i32, i32);
    void LoadExplosionSprites();
    void __stdcall BuildRockBreakParticles(i32, i32, i32, i32);
    void __stdcall LoadGruntResurrectTuning(i32, i32, i32);
    void LoadPowerupIconSprites();
    void __stdcall LaunchPortalExe(i32);
    void __stdcall BuildLevelRezPath(i32, i32, i32, i32, i32);
    void LoadHelpBookSprite();
    void __stdcall LoadObjectImageResources(i32, i32);
    void __stdcall LoadObjectSoundResources(i32, i32);
    void __stdcall LoadObjectAnimResources(i32, i32);
    void __stdcall LoadMenuStateAssets(i32, i32, i32);
    void LoadAreaLevelTable();
    void LoadRollingBallHazardSprites();
    void BuildGruntzCrcInfo();
    void BuildNamedGruntTable();
    void __stdcall LoadLevelByMode(i32, i32);
    void DrawDebugStats();
    void ValidateLevelTiles();
    void __stdcall BuildAssetNamespacePrefixes(i32, i32, i32, i32);
    void DrawSaveGameMenu();
    void BuildLevelTitleString();
    void BuildSoundFontPath();
    i32 Stub_0f90f0(char* szPath);
    void LoadStatzTabToggleSprite();
    // BuildStatzTabSmall_vfunc1 reconstructed as CStatzTabSmall::BuildSmall below.
    void UpdateGruntOvenStatusBar();
    void UpdateChipGrinderStatusBar();
    void UpdateWarpStoneStatusBar();
    void UpdateDestructButtonStatusBar();
    void LoadSwitchDownSprite();
    void LoadSwitchUpSprite();
    void __stdcall LoadBridgeMoveSprites(i32);
    void LoadPyramidBridgeSprites();
    void __stdcall BuildStatzTabSmall_vfunc1(i32, i32, i32, i32, i32, i32, i32, i32, i32);
    void SaveScreenshot();
    void FreeAllFonts();
    void FormatGameInfoString();
    void __stdcall BuildVoiceSoundList(i32);
    void __stdcall Stub_148940(i32, i32, i32, i32);
    void _tr_init();
    void _ct_init();
    void Stub_18c780();
    i32 __stdcall Stub_1bf702(char* szClsid, EngCString* out);
    void __stdcall Stub_1bf8f8(i32, i32);
    void __stdcall Stub_1c1609(i32, i32);
    void __stdcall Stub_1c176a(i32, i32);
    void Stub_1c7cb3();
    void __stdcall Stub_1ccb5c(i32, i32, i32);
    void __stdcall Stub_1d5029(i32, i32, i32, i32);
    void __stdcall Stub_1d513b(i32, i32, i32, i32);
} // namespace EngineLabelBacklog

// @confidence: high
// @source: string-xref
// @proximity: CPlaneRender@-0x3b0 | Utils@+0xd60 (boundary - pick one)
// @stub
RVA(0x0000a3b0, 0x6e2)
void EngineLabelBacklog::CreateGameObjectByName() {}

// @confidence: low
// @source: string-xref
// @proximity: CBattlezDlgCustom@-0x800 | CBootyState@+0x460 (boundary - pick one)
// @stub
RVA(0x00018830, 0x380)
void __stdcall EngineLabelBacklog::LoadBootyCheatState(i32, i32, i32) {}

// @confidence: med
// @source: decomp-xref
// @proximity: CBootyState@-0xa0 | CState@+0x810 (boundary - pick one)
// @stub
RVA(0x00018d30, 0xcd)
void BootyState::vfunc_9(i32) {}

// @confidence: med
// @source: string-xref
// @proximity: CBootyState@-0x270 | CState@+0x640 (boundary - pick one)
// @stub
RVA(0x00018f00, 0x4fb)
void EngineLabelBacklog::ShowSecretBonusMessage() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00019920, 0x1c2)
void EngineLabelBacklog::BuildGruntSprintAnimation() {}

// UpdateBootyWalkingGruntz (0x1b690, 1983 B) - the per-frame update of the booty
// (treasure / "WARP" spell) walking-grunt animation state machine, a __thiscall
// method on the CGruntzMgr-like game-state object (`this` carries the per-player
// grunt arrays at +0x2c8 / +0x2d8, the active-player step index m_2e8, and the
// sub-state flags m_2ec/m_2f0; m_1b4 gates the init vs step path). IDENTITY +
// STRUCTURE recovered (dump_target + string_xref):
//   - Entry guards: returns true early if g_gameReg->m_7c->m_8 != 0, or
//     g_gameReg->m_7c->m_4 > 0x24, or this->m_2e8 >= 4.
//   - Init path (m_1b4 == 0): a 4-player loop (i=0..3) that sets each grunt's
//     idle animation ("GRUNTZ_NORMALGRUNT_SOUTH_IDLE" / "..._IDLE4") and builds
//     the per-player pickup-spell name "GRUNTZ_PICKUPS_<L>" where the letter L is
//     a 4-entry jump-table on the player index spelling W/A/R/P ("WARP"); fires
//     the "GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D" wand cue + a "GAME_FLAGRISE" flag,
//     plays sounds via g_gameReg->m_60/m_74, and seeds a PRNG step (the
//     `shl/sub/idiv 0x11` LCG at 0x6c1288).
//   - Step path: advances m_2e8 per frame through the 4 players, toggling the
//     +0x40 visibility bit, the +0x5c sprite (g_5e9068 table) + the +0x60 timer
//     (0x1f4 / 0xdc), with a second 4-entry "WARP" jump-table at 0x41be60.
// DEFERRED to the final sweep: a >512 B deeply-branchy state machine with ~15
// unnamed engine callees (the CGruntzGrunt animation setters 0x150540/0x1505b0,
// the CString builders 0x1b8438/0x1b9ff5, the g_gameReg dispatch helpers) + the
// CGruntzMgr +0x1a0../+0x2c8.. layout; per the matcher >512B-branchy rule it is
// left stubbed (a partial would under-count AND diverge its regalloc) for a
// leaf-first redo once the callee set + the owning class are modeled.
// @confidence: med
// @source: decomp-xref
// @proximity: CState@-0x240 | EngineLabelBacklog@+0x9e0 (boundary - pick one)
// @stub
RVA(0x0001b690, 0x7bf)
void EngineLabelBacklog::UpdateBootyWalkingGruntz() {}

// BuildBootyPerfectAnimation @0x01c070 graduated to src/Gruntz/IconLoaders.cpp.

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0001c9d0, 0x351)
void EngineLabelBacklog::ShowLevelCompleteMessage() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x0001ce60, 0x450)
void EngineLabelBacklog::BuildBootyGruntIdleAnimation() {}

// @source: string-xref
// BuildPowerupIconKeys - seeds the "GAME_INGAMEICONZ" icon-key group then
// appends one tool/toy/powerup key string selected by `key` (a sparse switch;
// out-of-range / gap keys fall through to POWERUPZ_COIN).
// @early-stop
// jump-table-data-overlap scoring wall (~53%): code bytes byte-exact vs retail
// (verified by raw byte-compare; only the index-table/jump-table base reloc
// operands + $L labels differ). See docs/patterns/jumptable-data-overlap.md.
RVA(0x0001e720, 0x2fe)
void __stdcall EngineLabelBacklog::BuildPowerupIconKeys(PowerupKeyRegistry* reg, i32 key) {
    reg->SetGroup("GAME_INGAMEICONZ");
    switch (key) {
        case 1:
            reg->Add("TOOLZ_BOMBZ");
            return;
        case 2:
            reg->Add("TOOLZ_BOOMERANGZ");
            return;
        case 3:
            reg->Add("TOOLZ_BRICKZ");
            return;
        case 4:
            reg->Add("TOOLZ_CLUBZ");
            return;
        case 5:
            reg->Add("TOOLZ_GAUNTLETZ");
            return;
        case 6:
            reg->Add("TOOLZ_GLOVEZ");
            return;
        case 7:
            reg->Add("TOOLZ_GOOBERZ");
            return;
        case 8:
            reg->Add("TOOLZ_GRAVITYBOOTZ");
            return;
        case 9:
            reg->Add("TOOLZ_GUNHATZ");
            return;
        case 10:
            reg->Add("TOOLZ_NERFGUNZ");
            return;
        case 11:
            reg->Add("TOOLZ_ROCKZ");
            return;
        case 12:
            reg->Add("TOOLZ_SHIELDZ");
            return;
        case 13:
            reg->Add("TOOLZ_SHOVELZ");
            return;
        case 14:
            reg->Add("TOOLZ_SPRINGZ");
            return;
        case 15:
            reg->Add("TOOLZ_SPYZ");
            return;
        case 16:
            reg->Add("TOOLZ_SWORDZ");
            return;
        case 17:
            reg->Add("TOOLZ_TIMEBOMBZ");
            return;
        case 18:
            reg->Add("TOOLZ_TOOBZ");
            return;
        case 19:
            reg->Add("TOOLZ_WANDZ");
            return;
        case 20:
            reg->Add("TOOLZ_WARPSTONEZ1");
            return;
        case 21:
            reg->Add("TOOLZ_WELDERZ");
            return;
        case 22:
            reg->Add("TOOLZ_WINGZ");
            return;
        case 23:
            reg->Add("TOYZ_BABYWALKERZ");
            return;
        case 24:
            reg->Add("TOYZ_BEACHBALLZ");
            return;
        case 25:
            reg->Add("TOYZ_BIGWHEELZ");
            return;
        case 26:
            reg->Add("TOYZ_GOKARTZ");
            return;
        case 27:
            reg->Add("TOYZ_JACKINTHEBOXZ");
            return;
        case 28:
            reg->Add("TOYZ_JUMPROPEZ");
            return;
        case 29:
            reg->Add("TOYZ_POGOSTICKZ");
            return;
        case 30:
            reg->Add("TOYZ_SCROLLZ");
            return;
        case 31:
            reg->Add("TOYZ_SQUEAKTOYZ");
            return;
        case 32:
            reg->Add("TOYZ_YOYOZ");
            return;
        case 50:
            reg->Add("POWERUPZ_MEGAPHONEZ");
            return;
        case 54:
            reg->Add("POWERUPZ_GHOST");
            return;
        case 55:
            reg->Add("POWERUPZ_SUPERSPEED");
            return;
        case 56:
            reg->Add("POWERUPZ_INVULNERABILITY");
            return;
        case 57:
            reg->Add("POWERUPZ_CONVERSION");
            return;
        case 58:
            reg->Add("POWERUPZ_DEATHTOUCH");
            return;
        case 59:
            reg->Add("POWERUPZ_ROIDZ");
            return;
        case 60:
            reg->Add("POWERUPZ_REACTIVEARMOR");
            return;
        case 61:
            reg->Add("POWERUPZ_RANDOMCOLORZ");
            return;
        case 62:
            reg->Add("POWERUPZ_SCREENSHAKE");
            return;
        case 63:
            reg->Add("POWERUPZ_BLACKSCREEN");
            return;
        case 64:
            reg->Add("POWERUPZ_MINICAM");
            return;
        default:
            reg->Add("POWERUPZ_COIN");
            return;
    }
}

// @confidence: med
// @source: string-xref
// @proximity: CMultiBootyState@-0x40 | GruntzPlayer@+0x720 (boundary - pick one)
// @stub
RVA(0x0001ed30, 0x549)
void EngineLabelBacklog::DrawBattleStats() {}

// @source: decomp-xref
// BootyState::OnActivate_vfunc8 - on state activation: hide the cursor, register
// the BOOTY/GRUNTZ/LEVEL image namespaces under the asset host, install the
// "multi" namespaces, then kick the grunt-data load + the state timer.
RVA(0x0001f6f0, 0x10b)
i32 BootyState::OnActivate_vfunc8() {
    if (!BaseOnActivate()) {
        return 0;
    }

    while (ShowCursor(FALSE) >= 0)
        ;

    i32 h = m_2c->Lookup("IMAGEZ");
    if (!h) {
        return 0;
    }
    BootyRegistrar* reg = m_c->m_10;
    if (reg->CallRegister(h, "BOOTY", "_") == -1) {
        return 0;
    }

    h = m_30->Lookup("IMAGEZ");
    if (!h) {
        return 0;
    }
    reg = m_c->m_10;
    if (reg->CallRegister(h, "GRUNTZ", "_") == -1) {
        return 0;
    }

    h = m_28->Lookup("IMAGEZ");
    if (!h) {
        return 0;
    }
    reg = m_c->m_10;
    if (reg->CallRegister(h, "LEVEL", "_") == -1) {
        return 0;
    }

    if (!RegisterMultiNamespaces("multi", 0, 0, 0, 0, 1)) {
        return 0;
    }

    OnActivated();
    m_c->m_4->Load();
    StartTimer(0x50, 0x3e8, 0, 1);
    return 1;
}

// @confidence: high
// @source: tomalla
// @proximity: LeafCue@-0x70 | CChatBoxOwner@+0xb30 (boundary - pick one)
// @stub
RVA(0x0001f9b0, 0x2d2)
void EngineLabelBacklog::StartUpPrompt() {}

// ---------------------------------------------------------------------------
// LoadChatBoxSprite - looks up the "GAME_CHATBOX" sprite set in
// the game's image registry (this->m_18->m_10->+0x10 hash sub-table, Lookup =
// FUN_005b8008) and, if present, renders the level's chatbox frame into it and
// stamps the level/world title text. arg1 is the level/world context: arg1->m_2c
// is a DC-host whose +0x8 is a polymorphic DC source (GetDC = vtable slot +0x44,
// Done = slot +0x68, both __stdcall taking the object explicitly), and
// arg1->m_2c is non-null guarded. this->m_8 is a mode flag (==3 selects the
// alternate frame/offset/string-id set). Only offsets / code bytes are
// load-bearing; helpers are reloc-masked externals.
//
// int (BOOL) return like its loader siblings: the m_10==0 and hdc==0 guards jump
// to the shared `mov eax,1; ret` tail (return 1), the m_2c==0 / spr==0 / frame==0
// guards `return 0` (reused zeroed eax). A void return would tail-merge the bare
// epilogues and drop the eax=1 tail.
struct CChatBoxFrame { // the looked-up "GAME_CHATBOX" sprite set
    char m_pad00[0x14];
    void** m_14; // +0x14  frame-entry array
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  frame index (mode != 3)
    i32 m_68; // +0x68  frame index (mode == 3)
};
struct CChatBoxHash { // embedded at CChatBoxRegistry+0x10
    // FUN_005b8008 __thiscall; writes the found set to *out.
    void Lookup(char* szName, void** out);
};
struct CChatBoxRegistry { // this->m_18->m_10 points here
    char m_pad00[0x10];
    CChatBoxHash m_10; // +0x10
};
struct CChatBoxRegRoot { // this->m_18 points here
    char m_pad00[0x10];
    CChatBoxRegistry* m_10; // +0x10
};
// arg1->m_2c->m_8: a polymorphic DC source. GetDC (slot +0x44 == #17) and Done
// (slot +0x68 == #26) are __stdcall slots taking the object explicitly (the
// object is pushed as the first arg, not passed in ecx).
struct CChatBoxDcSrc {
    struct CChatBoxDcVtbl* m_vptr;
};
struct CChatBoxDcVtbl {
    void* s0[0x11];                                   // slots 0..16
    void(__stdcall* GetDC)(CChatBoxDcSrc*, HDC* out); // slot 17 == +0x44
    void* s18[0x68 / 4 - 0x12];                       // slots 18..25
    void(__stdcall* Done)(CChatBoxDcSrc*, HDC);       // slot 26 == +0x68
};
struct CChatBoxDcHost { // arg1->m_2c points here
    char m_pad00[0x8];
    CChatBoxDcSrc* m_8; // +0x08
};
struct CChatBoxCtx { // arg1 points here
    char m_pad00[0x2c];
    CChatBoxDcHost* m_2c; // +0x2c
};
// FUN_00553790 __stdcall: renders the chatbox frame into the looked-up set.
void __stdcall RenderChatBoxFrame(i32 ctx, void* a, void* b, i32 z); // RVA 0x153790
// The text-stamp host reached through this->m_14 (FUN @ RVA 0x1cd0, __thiscall).
struct CChatBoxTextHost {
    void StampText(HDC dc, i32 id, void* rect); // FUN @ 0x1cd0
};
// Typed view of `this`: m_0/m_4 are the two source roots whose sub-fields feed
// the render + text-stamp, m_8 the mode flag, m_10/m_14 source pointers, m_18
// the registry root.
struct CChatBoxOwner {
    char* m_0; // +0x00
    char* m_4; // +0x04
    i32 m_8;   // +0x08  mode (==3 selects alternate set)
    char m_pad0c[0x10 - 0xc];
    void* m_10;             // +0x10
    CChatBoxTextHost* m_14; // +0x14
    CChatBoxRegRoot* m_18;  // +0x18
};

// ---------------------------------------------------------------------------
// LoadCreditzStateAssets - the credits-screen game-state asset
// loader. Sibling of CHelpState::LoadAssets (same base-loader chain): first
// chains the base loader LoadGameAssetNamespaces(a1,a2,a3) (reloc-masked near
// call; bail returning its result on failure), forces the hardware cursor
// hidden, zeroes a 4-DWORD scratch block at +0x1b8.., registers the
// "STATEZ_CREDITZ" namespace through m_8 (cached at m_2c, == the CHelpState
// idiom: FUN_0053c030 __thiscall, returns the registered object). Then off that
// object it looks up the "SOUNDZ" set (FUN_0053a230 __thiscall) and installs it
// as "CREDITZ"/"_" into the sound registry at m_c->+0x28 (FUN_00557ee0, the
// LoadLevelSounds Install). It also looks up the "MIDIZ" music set off m_2c
// (FUN_0053bae0), resolves its "PLAY"/"MONOLITH" sub-entries (FUN_0053a000,
// 2-arg: name + the packed 'IMX' tag 0x584d49), and wires each ready sub-entry
// into the image registry at m_4->+0x48 (FUN_00538670 Install3). Finally it
// pumps the state's ready/init pair (m_c->m_4 FUN_00558d20/FUN_00558cb0),
// invokes two owner methods (the cursor/title setup at RVA 0x39a60 and the
// state-finish at 0x439c40), stamps mode 2 at +0x20c and clears +0x1b4. Only
// offsets / code bytes are load-bearing; helpers are reloc-masked externals.
//
// The "STATEZ_CREDITZ" registered object (m_2c): same Register source as
// CHelpState (FUN_0053c030). FindSet/FindSubset/Resolve/IsLoaded below are the
// reloc-masked __thiscall helpers off it / its sub-entries.
struct CCreditzSubEntry { // a music sub-entry ("PLAY"/"MONOLITH")
    i32 IsLoaded();       // FUN_00539960 __thiscall, ret BOOL/value
    char m_pad00[0xc];
    void* m_c; // +0x0c
};
struct CCreditzMusicSet { // the looked-up "MIDIZ" set (m_2c->FindSet)
    // FUN_0053a000 __thiscall: resolve a named sub-entry under a packed tag.
    CCreditzSubEntry* Resolve(char* szName, i32 tag);
};
struct CCreditzRegObj {               // the registered STATEZ_CREDITZ object (m_2c)
    void* FindSoundSet(char* szName); // FUN_0053a230 __thiscall, ret set ptr
    void* FindMusicSet(char* szName); // FUN_0053bae0 __thiscall, ret set ptr
};
struct CCreditzSoundRegistry { // this->m_c->+0x28 (the LoadLevelSounds registry)
    void Install(void* set, char* szName, char* szKey); // FUN_00557ee0 __thiscall
};
struct CCreditzImageRegistry { // this->m_4->+0x48
    // FUN_00538670 __thiscall: install a resolved sub-entry under a name.
    void Install3(void* res, void* host, char* szName);
};
struct CCreditzStateCore {      // this->m_c->m_4 (the ready/init pump)
    i32 IsReady();              // FUN_00558d20 __thiscall, ret BOOL
    i32 Init(i32 a, i32 flags); // FUN_00558cb0 __thiscall, ret BOOL
};
struct CCreditzImageRoot { // this->m_4 points here; +0x48 is the registry
    char m_pad00[0x48];
    CCreditzImageRegistry* m_48; // +0x48
};
struct CCreditzSoundMgr { // this->m_c points here
    char m_pad00[0x4];
    CCreditzStateCore* m_4; // +0x04
    char m_pad08[0x28 - 0x8];
    CCreditzSoundRegistry* m_28; // +0x28
};
struct CCreditzRegSet {                   // this->m_8 points here
    CCreditzRegObj* Register(char* name); // FUN_0053c030 __thiscall (CHelpState idiom)
};
// Two owner methods reached at the tail, both __thiscall(this) no args:
// the title/cursor setup (RVA 0x39a60) and the state-finish (0x439c40).

// Typed view of `this`: m_4 the image-registry root, m_8 the namespace registry,
// m_c the sound/state manager, m_2c the registered STATEZ_CREDITZ object.
//
struct CCreditzOwner {
    char m_pad00[0x4];
    CCreditzImageRoot* m_4; // +0x04
    CCreditzRegSet* m_8;    // +0x08
    CCreditzSoundMgr* m_c;  // +0x0c
    char m_pad10[0x2c - 0x10];
    CCreditzRegObj* m_2c; // +0x2c
    char m_pad30[0x1b4 - 0x30];
    i32 m_1b4; // +0x1b4
    i32 m_1b8; // +0x1b8
    i32 m_1bc; // +0x1bc
    i32 m_1c0; // +0x1c0
    i32 m_1c4; // +0x1c4
    char m_pad1c8[0x20c - 0x1c8];
    i32 m_20c;                                  // +0x20c
    void SetupTitle();                          // RVA 0x39a60 __thiscall
    i32 FinishState();                          // RVA 0x439c40 __thiscall
    i32 LoadGameAssetNamespaces(i32, i32, i32); // base loader; reloc-masked near call
};

// @confidence: med
// @source: decomp-xref
// @proximity: WwdFile@-0x350 | CGruntCreationPoint@+0x3500 (boundary - pick one)
// @stub
RVA(0x0003b7c0, 0x12c)
void EngineLabelBacklog::LoadCustomWorldInfo() {}

// @confidence: med
// @source: decomp-xref
// @proximity: CGruntCreationPoint@-0x930 | CWormhole@+0x680 (boundary - pick one)
// @stub
RVA(0x0003f5f0, 0x526)
void EngineLabelBacklog::HandleFortConquered() {}

// @confidence: med
// @source: string-xref
// @proximity: CUserLogic@-0x2f90 | CGrunt@+0x4d0 (boundary - pick one)
// @stub
RVA(0x00050ce0, 0x399)
void __stdcall EngineLabelBacklog::LoadVehicleGruntSprites(i32) {}

// @confidence: med
// @source: decomp-xref
// @proximity: CGrunt (HIGH bracket, tu_layout) - CONFLICTS with this Projectile::vfunc_9 guess; verify the real vtable owner before graduating.
// @stub
RVA(0x00061cb0, 0x34a)
void Projectile::vfunc_9() {}

// WireTileSwitchLogic (0x0006c130, 3426 B) graduated to
// src/Gruntz/TileSwitchLogic.cpp (eh unit) as CGruntzMgr::WireTileSwitchLogic:
// the tile-switch/plate wire dispatcher (the "No switch/trigger/plate logic
// found" + "GAME_SECRETSWITCH" / "CrumbleTileDelay" $SG set). Its CString
// diagnostic temp gives it the /GX exception frame. Reconstructed (validated top
// - prologue, grid clamp, cell-tag resolve, primary switch dispatch + the first
// diagnostic arm; @early-stop on the branchy nested-jump-table /GX wall).

// LoadTerrainTileSprites (0x00075e90, 4905 B) is the per-tile terrain-action
// sprite loader; reconstructed (complete top structure + DIRT/GIANTROCK arms,
// @early-stop on the /GX nested-jump-table megafunction wall) as
// CTerrainTileLoader::Load in src/Gruntz/TerrainTileLoader.cpp (eh unit). Its
// CString diagnostic temp gives it the exception frame.

// LoadCameraSprite @0x078960 graduated to src/Gruntz/IconLoaders.cpp.

// @confidence: med
// @source: decomp-xref
// @proximity: CTriggerMgr (HIGH bracket, tu_layout) is a FALSE POSITIVE - this is an icon loader; siblings LoadCameraSprite/LoadExplosionSprites graduated to src/Gruntz/IconLoaders.cpp. Graduate there, not to CTriggerMgr.
// @stub
RVA(0x0007a3f0, 0xd7)
void __stdcall EngineLabelBacklog::LoadToyBoxIcon(i32, i32, i32, i32, i32) {}

// LoadExplosionSprites @0x07b330 graduated to src/Gruntz/IconLoaders.cpp.

// @confidence: med
// @source: decomp-xref
// @proximity: EngineLabelBacklog@-0x110 | CTriggerMgr@+0x9d0 (boundary - pick one)
// @stub
RVA(0x0007b440, 0x3f0)
void __stdcall EngineLabelBacklog::BuildRockBreakParticles(i32, i32, i32, i32) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0007be60, 0x21e)
void __stdcall EngineLabelBacklog::LoadGruntResurrectTuning(i32, i32, i32) {}

// LoadPowerupIconSprites @0x07c620 graduated to src/Gruntz/IconLoaders.cpp.

// @confidence: low
// @source: decomp-xref
// @proximity: CGruntzMgr (HIGH bracket, tu_layout) - plausible owner (existing label is low-confidence); verify before graduating.
// @stub
RVA(0x00090550, 0x1e6)
void __stdcall EngineLabelBacklog::LaunchPortalExe(i32) {}

// @confidence: med
// @source: string-xref
// @proximity: CGruntzMgr@-0x720 | CGruntzWnd@+0x900 (boundary - pick one)
// @stub
RVA(0x00093d40, 0x473)
void __stdcall EngineLabelBacklog::BuildLevelRezPath(i32, i32, i32, i32, i32) {}

// @source: decomp-xref
RVA(0x00095090, 0x6e)
i32 CHelpState::LoadAssets(i32 a1, i32 a2, i32 a3) {
    if (!LoadGameAssetNamespaces(a1, a2, a3)) {
        return 0;
    }
    while (ShowCursor(0) >= 0)
        ;
    m_2c = m_8->Register("STATEZ_HELP");
    if (!m_2c) {
        return 0;
    }
    m_4->m_4->Pump(0x100, 0x40);
    return 1;
}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000997c0, 0x1e7)
void EngineLabelBacklog::LoadHelpBookSprite() {}

// @confidence: med
// @source: decomp-xref
// @proximity: CSpawnEntry@-0xc0 | CAreaMgr@+0xf20 (boundary - pick one)
// @stub
RVA(0x0009a510, 0x275)
void __stdcall EngineLabelBacklog::LoadObjectImageResources(i32, i32) {}

// @confidence: med
// @source: decomp-xref
// @proximity: CSpawnEntry@-0x4c0 | CAreaMgr@+0xb20 (boundary - pick one)
// @stub
RVA(0x0009a910, 0x261)
void __stdcall EngineLabelBacklog::LoadObjectSoundResources(i32, i32) {}

// @confidence: med
// @source: decomp-xref
// @proximity: CSpawnEntry@-0x7d0 | CAreaMgr@+0x810 (boundary - pick one)
// @stub
RVA(0x0009ac20, 0x261)
void __stdcall EngineLabelBacklog::LoadObjectAnimResources(i32, i32) {}

// @confidence: med
// @source: string-xref
// @proximity: CMapVisitTarget@-0x660 | CChatBox@+0x430 (boundary - pick one)
// @stub
RVA(0x0009fe50, 0x343)
void __stdcall EngineLabelBacklog::LoadMenuStateAssets(i32, i32, i32) {}

// 0x000a11d0 (6157 B) is NOT a level-table loader - the $SG string set
// ("MENU.MAINMENU.TITLE", "SINGLEPLAYER", ...) identifies it as the main-menu
// tree builder. Reconstructed (complete body, @early-stop on the /GX EH-state
// wall) in the eh unit src/Gruntz/MainMenuBuilder.cpp; its CString temps give it
// an exception frame, so it left the frameless backlog aggregate.

// LoadRollingBallHazardSprites (0x000b0140, 2682 B) is NOT a sprite loader: it
// graduated to src/Gruntz/RollingBall.cpp (eh unit) as CRollingBall::Update, the
// per-tick rolling-ball movement/state update (the LEVEL_ROLLINGBALL_*
// directional roll + sink/fall/explosion sound $SG set + the sub-tile position
// interpolation). Its two CString sound-name temps give it the /GX exception
// frame. Reconstructed (complete body - prologue, action/direction/sink switches,
// float-interp tail; @early-stop on the branchy nested-jump-table /GX wall).

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000b4640, 0x104)
void CloudHazard::vfunc_20(i32, i32) {}

// @confidence: low
// @source: decomp-xref
// @proximity: CFileIO@-0x1df0 | CMultiStartDlg@+0x2580 (boundary - pick one)
// @stub
RVA(0x000bf1d0, 0x249)
void EngineLabelBacklog::BuildGruntzCrcInfo() {}

// @confidence: med
// @source: decomp-xref
// BuildNamedGruntTable - seeds the 4 named-grunt CString globals (contiguous
// array at 0x64bdb0) with their default names via CString::operator=(LPCSTR)
// (FUN_001b9d4c, reloc-masked __thiscall).
struct EngStrAssign {
    char* m_pszData;
    void operator=(const char* s); // CString::operator=, FUN_001b9d4c
};
// 4 contiguous CString globals at 0x64bdb0 (defined in the engine's data).
DATA(0x0064bdb0)
extern EngStrAssign g_gruntNames[4];
RVA(0x000c16b0, 0x3d)
void EngineLabelBacklog::BuildNamedGruntTable() {
    g_gruntNames[0] = "Beefy";
    g_gruntNames[1] = "Zed";
    g_gruntNames[2] = "Serra";
    g_gruntNames[3] = "Jebediah";
}

// LoadLevelByMode (0x000ca200, 3636 B) is the PLAY game-state per-mode level
// loader (BATTLEZ/MULTI/CUSTOMLEVEL/TRAINING/Level%i); reconstructed (complete
// body + the linear init chain, @early-stop on the /GX megafunction wall) as
// CPlayLevelLoad::LoadByMode in src/Gruntz/LoadLevelByMode.cpp (eh unit). Its
// area-name / WarpStone CString temps give it the exception frame.

// @confidence: med
// @source: decomp-xref
// @proximity: CPlayLevelLoad@-0x1600 | CGamePlayInput@+0x4c0 (boundary - pick one)
// @stub
RVA(0x000cb800, 0x191)
void GameLevelState::OnActivate_vfunc8() {}

// vfunc_12 (0x000cbcc0, 5850 B) is the PLAY-state keyboard/cheat input
// dispatcher, not a status-bar method; reconstructed (complete top structure,
// @early-stop on the megafunction regalloc wall) as CGamePlayInput::DispatchKey
// in src/Gruntz/GameKeyHandler.cpp (frameless base unit).

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000ce660, 0x362)
void StatusBarItem::vfunc_16(i32, i32, i32) {}

// @confidence: low
// @source: decomp-xref
// @proximity: CGameLevel@-0x960 | CPlay@+0x9b0 (boundary - pick one)
// @stub
RVA(0x000cf770, 0x35e)
void EngineLabelBacklog::DrawDebugStats() {}

// ValidateLevelTiles (0x000d2dd0, 7652 B) is reconstructed (complete body,
// @early-stop on the megafunction regalloc wall) in the eh unit
// src/Gruntz/LevelTileValidation.cpp - moved out of the frameless backlog
// aggregate because its CString diagnostics give it a /GX exception frame.

// ---------------------------------------------------------------------------
// LoadActionTileSprites - per-level ACTION/TILEZ asset loader.
// Reaches the game resource registry through this->m_c (->+0x10): if not forced
// (arg1==0) and the "ACTION" sprite set is already present, bail; otherwise
// register the "ACTION" and "BACK" namespaces (empty-string key), reset the
// severus tile counter, look up the level's "TILEZ" sprite set off this->m_28,
// and wire it into the registry through a virtual slot. Only offsets / code
// bytes are load-bearing; helpers are reloc-masked externals.
//
// The registry object embedded at this->m_c->m_10: Has() probes a named set
// (FUN_00555550, returns found-flag), Register() adds a namespace
// (FUN_00555360), and a virtual slot (index 18) installs the loaded TILEZ set.
struct CActionResRegistry {
    i32 Has(char* szName);                    // FUN_00555550 __thiscall, ret found
    void Register(char* szName, char* szKey); // FUN_00555360 __thiscall
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void InstallTileSet(void* set, char* empty, char* key); // slot 18 (+0x48)
};
struct CActionResMgr { // this->m_c points here; +0x10 is the registry
    char m_pad00[0x10];
    CActionResRegistry* m_10; // +0x10
};
struct CTileSetSource {                // this->m_28 points here
    void* LookupTileSet(char* szName); // FUN_0053bae0 __thiscall, ret set ptr
};
DATA(0x002bf37c)
extern i32 g_severusCounterA;
extern "C" char g_emptyString[]; // 0x6293f4

// Typed view of `this` for this loader: m_c is the resource manager, m_28 the
// level's tile-set source.
struct CActionTileOwner {
    char m_pad00[0xc];
    CActionResMgr* m_c; // +0x0c
    char m_pad10[0x28 - 0x10];
    CTileSetSource* m_28; // +0x28
};

// ---------------------------------------------------------------------------
// LoadLevelSounds - per-level SOUNDZ asset loader. Sibling of the
// two tile/image loaders above; same Lookup/Register registry idiom but reaches
// a DIFFERENT registry object embedded at this->m_c->+0x28 (not ->m_10), whose
// Has/Register/Install are plain non-virtual __thiscall helpers (not the +0x48
// vtable slot). If not forced (arg1==0) and the "LEVEL" set is already present,
// bail; otherwise register the "LEVEL" namespace (key "_"), look up the level's
// "SOUNDZ" set off this->m_28, and install it (key "_"). No severus reset here.
// Only offsets / code bytes are load-bearing; helpers are reloc-masked externals.
struct CSoundResRegistry {                              // this->m_c->+0x28 points here
    i32 Has(char* szName);                              // FUN_004583c0 __thiscall, ret found
    void Register(char* szName, char* szKey);           // FUN_00557c70 __thiscall
    void Install(void* set, char* szName, char* szKey); // FUN_00557ee0 __thiscall
};
struct CSoundResMgr { // this->m_c points here; +0x28 is the sound registry
    char m_pad00[0x28];
    CSoundResRegistry* m_28; // +0x28
};
struct CSoundSetSource {                // this->m_28 points here
    void* LookupSoundSet(char* szName); // FUN_0053bae0 __thiscall, ret set ptr
};

// Typed view of `this` for this loader: m_c is the sound resource manager, m_28
// the level's sound-set source.
struct CSoundOwner {
    char m_pad00[0xc];
    CSoundResMgr* m_c; // +0x0c
    char m_pad10[0x28 - 0x10];
    CSoundSetSource* m_28; // +0x28
};

// @confidence: med
// @source: string-xref
// @proximity: CPlay@-0x3a0 | CProjectile@+0x24f0 (boundary - pick one)
// @stub
RVA(0x000dca70, 0x4a4)
void __stdcall EngineLabelBacklog::BuildAssetNamespacePrefixes(i32, i32, i32, i32) {}

// ---------------------------------------------------------------------------
// LoadGruntzPalette - registers a level's "GRUNTZ_PALETTEZ_<name>"
// palette into the game's image/sprite registry. arg1 is the source registry
// object (its FUN_0053bff0 resolves a "PAL" resource by namespaced name); arg2
// is the level/name string spliced into the format. Reaches the destination
// registry through this->m_4->m_18: Lookup() (FUN_005b8008, +0x10 hash sub-table)
// probes whether the palette is already present, and virtual slot 9 (+0x24)
// installs the resolved palette. Only offsets / code bytes are load-bearing;
// helpers are reloc-masked externals.
//
// The destination registry at this->m_4->m_18 is polymorphic: a hash sub-table
// embedded at +0x10 backs Lookup() (out-param set non-null => already present),
// and Install (vtable slot 9) takes the resolved palette + two null args.
struct CPaletteHashTable { // embedded at CPaletteDestRegistry+0x10
    // FUN_005b8008 __thiscall; writes found value to *out.
    void Lookup(char* szName, void** out);
};
struct CPaletteDestRegistry {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual i32 Install(void* res, i32 a, i32 b); // slot 9 (+0x24)
    char m_pad04[0x10 - 0x4];
    CPaletteHashTable m_10; // +0x10  hash sub-table Lookup runs on
};
struct CPaletteDestRoot { // this->m_4 points here; +0x18 is the dest registry
    char m_pad00[0x18];
    CPaletteDestRegistry* m_18; // +0x18
};
// arg1's source registry: FUN_0053bff0 __thiscall resolves a packed-tag resource
// ('PAL' = 0x50414c) by namespaced name, returning the resource (0 if absent).
struct CPaletteSource {
    void* Resolve(char* szName, i32 tag); // FUN_0053bff0
};

// Typed view of `this`: m_4 is the destination registry root.
struct CPaletteOwner {
    char m_pad00[0x4];
    CPaletteDestRoot* m_4; // +0x04
};

// @confidence: med
// @source: string-xref
// @proximity: CSpriteRef@-0xc60 | CSaveGame@+0xde0 (boundary - pick one)
// @stub
RVA(0x000e3f40, 0x3d6)
void EngineLabelBacklog::DrawSaveGameMenu() {}

// @confidence: med
// @source: string-xref
// @proximity: CSpriteRef@-0x1200 | CSaveGame@+0x840 (boundary - pick one)
// @stub
RVA(0x000e44e0, 0x2b2)
void EngineLabelBacklog::BuildLevelTitleString() {}

// @confidence: med
// @source: decomp-xref
// @proximity: CGrunt@-0x6280 | CAttract@+0x1990 (boundary - pick one)
// @stub
RVA(0x000f8970, 0x3b4)
void SFManager::SelectBestDevice() {}

// @confidence: high
// @source: string-xref
// @proximity: CGrunt@-0x6840 | CAttract@+0x13d0 (boundary - pick one)
// @stub
RVA(0x000f8f30, 0x160)
void EngineLabelBacklog::BuildSoundFontPath() {}

// @source: import:OpenFile
// FileExists - tests a path via OpenFile(OF_EXIST). Re-emitted copy of
// Utils::WinAPI::FileExists (same bytes).
RVA(0x000f90f0, 0x45)
i32 EngineLabelBacklog::Stub_0f90f0(char* szPath) {
    OFSTRUCT of;

    if (!szPath) {
        return 0;
    }
    if (!*szPath) {
        return 0;
    }
    return OpenFile(szPath, &of, 0x4000 /*OF_EXIST*/) != -1;
}

// @confidence: med
// @source: string-xref
// @proximity: CTileTriggerSwitchLogic@-0x40 | CPlayLevelLoad@+0x3b0 (boundary - pick one)
// @stub
RVA(0x00110860, 0x25f)
void __stdcall EngineLabelBacklog::LoadBridgeMoveSprites(i32) {}

// LoadPyramidBridgeSprites (0x00110c10, 3647 B) is the pyramid/bridge tile-
// transition dispatcher (a 0x66-case jump table over the sprite-type id);
// reconstructed (complete top structure + the pyramid jump arms, @early-stop on
// the /GX nested-jump-table megafunction wall) as
// CPlayLevelLoad::LoadPyramidBridge in src/Gruntz/PyramidBridgeSprites.cpp (eh
// unit). Its GAME_<COLOR>PYRAMIDZ / GAME_PYRAMIDUP CString temps give it the
// exception frame.

// ---------------------------------------------------------------------------
// BuildStatzTabSmall (0x112a50) - a status-bar StatzTab builder vfunc (slot 1).
// Bails if already built (m_20 != 0) or, in mode 4, if the caller's rect is empty;
// otherwise copies the caller's 0x60-byte rect block into this+0x2c, chains the
// base builder (vtable slot 0) with the forwarded args, and (when a9 != 0) asks
// the HUD sprite factory (g_gameReg->m_30->m_8->CreateSprite) for the tab sprite
// at the tile's pixel origin (tile<<5 + 0x10), runs its init slot, configures it,
// and reports readiness via the sprite's +0x198. Only offsets / code bytes are
// load-bearing; helpers are reloc-masked externals.
struct CStatzRect60 {
    i32 d[0x18]; // 0x60 bytes
};
struct CStatzSprite;
struct CStatzSpriteFactory {
    // FUN_001597b0 __thiscall, ret 0x18: build the named tab sprite.
    CStatzSprite* CreateSprite(i32 kind, i32 px, i32 py, i32 hint, void* name, i32 flags);
};
struct CStatzFactoryHolder {
    char m_pad0[0x8];
    CStatzSpriteFactory* m_8; // +0x08
};
struct CStatzGameReg {
    char m_pad0[0x30];
    CStatzFactoryHolder* m_30; // +0x30
};
DATA(0x0024556c)
extern CStatzGameReg* g_statzGameReg; // *0x64556c
struct CStatzSpriteInitVtbl {
    void* m_slot0[4];                   // slots 0..3
    void(__cdecl* Init)(CStatzSprite*); // slot 4 (+0x10)
};
struct CStatzSprite {
    char m_pad0[0x7c];
    CStatzSpriteInitVtbl* m_7c; // +0x7c
    char m_pad80[0x198 - 0x80];
    i32 m_198;                       // +0x198
    void Configure(void* tag, i32 a); // FUN_001504d0 __thiscall
};
DATA(0x0020aa34)
extern char g_statzTabSpriteName[]; // CreateSprite name buffer
DATA(0x0020f928)
extern char g_statzTabCfgTag[]; // Configure tag global
struct CStatzTabSmall {
    virtual i32 BaseBuild(i32, i32, i32, i32, i32, i32, i32, i32); // slot 0 (reloc-masked)
    i32 BuildSmall(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, CStatzRect60* a6, i32 a7, i32 a8,
                   i32 a9);
    char m_pad04[0x20 - 0x4];
    i32 m_20; // +0x20  already-built gate
    char m_pad24[0x2c - 0x24];
    CStatzRect60 m_2c; // +0x2c  rect block
};
// @early-stop
// regalloc/tail-merge wall (~62%): instruction selection, calls and constants are
// byte-correct, but retail pins arg3->ebp / arg4->ebx via prologue-interleaved
// arg-loads (push ebx; mov ebx,[esp+0x14]; push ebp; mov ebp,[esp+0x14]) and
// tail-merges the early `return 0` exits into the shared post-BaseBuild epilogue;
// our /O2 build pins different args + emits inline epilogues, cascading the whole
// register allocation. Logic complete; not source-steerable. See
// docs/patterns/identical-return-epilogue-tailmerge.md + pin-local-for-callee-saved-reg.md.
RVA(0x00112a50, 0xdd)
i32 CStatzTabSmall::BuildSmall(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, CStatzRect60* a6, i32 a7,
                               i32 a8, i32 a9) {
    if (m_20 != 0) {
        return 0;
    }
    if (a2 == 4 && a6->d[0] == 0) {
        return 0;
    }
    m_2c = *a6;
    if (!BaseBuild(a1, a2, a3, a4, a5, a7, a8, a9)) {
        return 0;
    }
    i32 px = (a3 << 5) + 0x10;
    i32 py = (a4 << 5) + 0x10;
    if (a9 == 0) {
        return 1;
    }
    CStatzSprite* spr =
        g_statzGameReg->m_30->m_8->CreateSprite(0, px, py, 0, g_statzTabSpriteName, 0x40001);
    if (!spr) {
        return 0;
    }
    spr->m_7c->Init(spr);
    spr->Configure(g_statzTabCfgTag, a9);
    if (spr->m_198 == 0) {
        return 0;
    }
    return 1;
}

// @confidence: high
// @source: decomp-xref
// @proximity: CToobSpikez@-0x790 | CTileTriggerSwitchLogic@+0xf10 (boundary - pick one)
// @stub
RVA(0x00114ff0, 0x1b3)
void EngineLabelBacklog::SaveScreenshot() {}

// @confidence: low
// @source: decomp-xref
// @proximity: CTileTriggerContainer@-0x450 | CGruntVoice@+0x1730 (boundary - pick one)
// @stub
RVA(0x001183b0, 0x211)
void EngineLabelBacklog::FormatGameInfoString() {}

// @confidence: med
// @source: decomp-xref
// @proximity: CGruntSpawnConfig@-0x70 | CSpawnEntry@+0x350 (boundary - pick one)
// @stub
RVA(0x0011c210, 0x29d)
void __stdcall EngineLabelBacklog::BuildVoiceSoundList(i32) {}

// CRT lowio/startup internals (FID-anchored in config/library_labels.csv as
// __findfirst / __write_lk / __sopen / __lseek_lk / __read_lk / __NMSG_WRITE);
// the library label is canonical, so the redundant backlog stubs were removed:
//   0x0011f900 __findfirst   0x0012abf0 __NMSG_WRITE  0x0012d230 __write_lk
//   0x0012d460 __sopen       0x0012d880 __lseek_lk    0x001315d0 __read_lk

// @confidence: low
// @source: second '.PID' xref; image-format dispatch consumer (entry not pinned)
// @proximity: CFileImage@-0xb0 | CImageOwned@+0x3a0 (boundary - pick one)
// @stub
RVA(0x00148940, 0x102)
void __stdcall EngineLabelBacklog::Stub_148940(i32, i32, i32, i32) {}

// ParseAttributeFile @0x170750 graduated to src/Bute/ButeMgr.cpp (the value-line
// driver; reconstructed on the ButeMgr class alongside the matched getters).

// @confidence: high
// @source: call-xref
// @stub
void EngineLabelBacklog::_tr_init() {}

// @confidence: high
// @source: tomalla
// @stub
void EngineLabelBacklog::_ct_init() {}

// @confidence: low
// @source: import:FindFirstFileA
// @stub
RVA(0x0018c780, 0x33f)
void EngineLabelBacklog::Stub_18c780() {}

// The empty global string used as RegQueryValueEx's value name (default value).
extern "C" char g_emptyString[]; // 0x6293f4

// @source: import:RegQueryValueExA
// Stub_1bf702 - reads HKCR\CLSID\<szClsid>\InProcServer32's default value into
// the out CString (the COM in-proc server DLL path). Returns whether it
// succeeded; closes every opened key on the way out.
// @early-stop
// frame-pointer wall (~55%): retail compiles this 0x1bf*-0x1d5* registry/COM
// helper module WITH frame pointers (push ebp/mov ebp,esp); this frameless /O2
// aggregate recompiles it esp-relative. Logic + all externs + string constants
// match. Belongs in a /Oy- TU in the final sweep. docs/patterns/o2-optimizer-bailout-framed.md.
RVA(0x001bf702, 0xac)
i32 __stdcall EngineLabelBacklog::Stub_1bf702(char* szClsid, EngCString* out) {
    HKEY hClsidRoot = 0;
    HKEY hClsid = 0;
    HKEY hServer;
    DWORD dwType;
    i32 result = 0;

    if (RegOpenKeyA((HKEY)0x80000000 /*HKCR*/, "CLSID", &hClsidRoot) == 0) {
        if (RegOpenKeyA(hClsidRoot, szClsid, &hClsid) == 0) {
            if (RegOpenKeyA(hClsid, "InProcServer32", &hServer) == 0) {
                DWORD cb = 0x104;
                char* pszBuf = out->GetBuffer(0x104);
                i32 rc = RegQueryValueExA(hServer, g_emptyString, 0, &dwType, (LPBYTE)pszBuf, &cb);
                out->ReleaseBuffer(-1);
                result = (rc == 0);
                RegCloseKey(hServer);
            }
            RegCloseKey(hClsid);
        }
        RegCloseKey(hClsidRoot);
    }
    return result;
}

// @confidence: low
// @source: import:FindFirstFileA
// @stub
RVA(0x001bf8f8, 0xd9)
void __stdcall EngineLabelBacklog::Stub_1bf8f8(i32, i32) {}

// @confidence: low
// @source: import:FindFirstFileA
// @stub
RVA(0x001c1609, 0xb2)
void __stdcall EngineLabelBacklog::Stub_1c1609(i32, i32) {}

// @confidence: low
// @source: import:CreateFileA
// @stub
RVA(0x001c176a, 0x12d)
void __stdcall EngineLabelBacklog::Stub_1c176a(i32, i32) {}

// @confidence: med
// @source: import:RegCloseKey,RegOpenKeyExA,RegQueryValueExA
// @stub
RVA(0x001c7cb3, 0x177)
void EngineLabelBacklog::Stub_1c7cb3() {}

// @confidence: med
// @source: import:RegCloseKey,RegSetValueExA
// @stub
void __stdcall EngineLabelBacklog::Stub_1ccb5c(i32, i32, i32) {}

// @source: import:RegOpenKeyExA
// CConfigStore::OpenRoot - opens/creates HKCU\Software\<m_7c>\<m_90> and returns
// the deepest key (0 on failure), closing the two intermediate keys.
// @early-stop
// frame-pointer/regalloc wall (~54%): retail keeps an ebp frame + caches `this`
// in ebx; the /O2 recompile of the same logic comes out frameless with this in
// esi. Logic correct; all externs named. See docs/patterns/o2-optimizer-bailout-framed.md.
HKEY CConfigStore::OpenRoot() {
    HKEY hSoftware = 0;
    HKEY hMid = 0;
    HKEY hResult = 0;
    DWORD dwDisp;
    CConfigStore* self = this;

    if (RegOpenKeyExA((HKEY)0x80000001, "Software", 0, 0x2001f, &hSoftware) == 0
        && RegCreateKeyExA(hSoftware, m_7c, 0, 0, 0, 0x2001f, 0, &hMid, &dwDisp) == 0) {
        RegCreateKeyExA(hMid, self->m_90, 0, 0, 0, 0x2001f, 0, &hResult, &dwDisp);
    }

    if (hSoftware) {
        RegCloseKey(hSoftware);
    }
    if (hMid) {
        RegCloseKey(hMid);
    }
    return hResult;
}

// @source: import:OpenFile
// FileExists - tests a path via OpenFile(OF_EXIST). Re-emitted copy of
// Utils::WinAPI::FileExists (same bytes).
RVA(0x0001fd70, 0x45)
i32 EngineLabelBacklog::Stub_01fd70(char* szPath) {
    OFSTRUCT of;

    if (!szPath) {
        return 0;
    }
    if (!*szPath) {
        return 0;
    }
    return OpenFile(szPath, &of, 0x4000 /*OF_EXIST*/) != -1;
}

// @confidence: med
// @source: string-xref
// @proximity: CCheatMgr@-0x1e0 | CCheckpointDlg@+0x640 (boundary - pick one)
// @stub
RVA(0x00022e60, 0x1be)
void EngineLabelBacklog::LoadCheatConfig() {}

// @source: import:RegCloseKey,RegCreateKeyExA
// CConfigStore::OpenSubKey - opens szSub under the OpenRoot() key, then closes
// the root and returns the opened subkey (0 on failure).
// @early-stop
// frame-pointer/regalloc wall (~55%): same ebp-frame divergence as OpenRoot.
// Logic correct; all externs named. See docs/patterns/o2-optimizer-bailout-framed.md.
HKEY CConfigStore::OpenSubKey(char* szSub) {
    HKEY hResult = 0;
    DWORD dwDisp;

    HKEY hRoot = OpenRoot();
    if (hRoot == 0) {
        return 0;
    }

    RegCreateKeyExA(hRoot, szSub, 0, 0, 0, 0x2001f, 0, &hResult, &dwDisp);
    RegCloseKey(hRoot);
    return hResult;
}

// @source: import:RegCloseKey,RegQueryValueExA
// CConfigStore::GetInt - reads an integer value: through the registry when m_7c
// is set (open szSection subkey, RegQueryValueEx szKey), else via the INI file
// m_90 (GetPrivateProfileInt). Returns nDefault on any failure.
// @early-stop
// frame-pointer/regalloc wall (~24%): retail reuses the szSection arg slot as
// cbData and keeps an ebp frame; the recompile allocates a fresh cbData local +
// frameless. Logic correct; all externs named. docs/patterns/o2-optimizer-bailout-framed.md.
i32 CConfigStore::GetInt(char* szSection, char* szKey, i32 nDefault) {
    DWORD dwType;
    DWORD dwData;

    if (m_7c != 0) {
        HKEY hKey = OpenSubKey(szSection);
        if (hKey == 0) {
            return nDefault;
        }
        DWORD cbData = 4;
        i32 rc = RegQueryValueExA(hKey, szKey, 0, &dwType, (LPBYTE)&dwData, &cbData);
        RegCloseKey(hKey);
        if (rc == 0) {
            return dwData;
        }
        return nDefault;
    }

    return GetPrivateProfileIntA(szSection, szKey, nDefault, m_90);
}

// @confidence: med
// @source: import:RegCloseKey
// @stub
RVA(0x001d5029, 0x112)
void __stdcall EngineLabelBacklog::Stub_1d5029(i32, i32, i32, i32) {}

// @confidence: med
// @source: import:RegCloseKey
// @stub
RVA(0x001d513b, 0x11b)
void __stdcall EngineLabelBacklog::Stub_1d513b(i32, i32, i32, i32) {}
