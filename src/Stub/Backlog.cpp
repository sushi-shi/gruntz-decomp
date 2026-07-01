#include <Win32.h> // ShowCursor (matching-neutral; ApiCallers.cpp in this aggregate already pulls it)
#include <stdio.h>  // engine sprintf (reloc-masked) - LoadGruntzPalette / FormatGameInfoString
#include <string.h> // inline strlen/strcat/memset intrinsics (/O2) - FormatGameInfoString

#include <rva.h>
// Backlog.cpp - engine-label stubs without a class attribution.

// Folded engine-label stubs with a known owning class.
// A named asset-namespace registry slot (BootyState's m_28/m_2c/m_30). Lookup()
// finds a child set by name and returns a handle (reloc-masked __thiscall).
SIZE_UNKNOWN(BootyNamespace);
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
SIZE_UNKNOWN(BootyRegistrar);
struct BootyRegistrar {
    BootyRegistrarVtbl* m_vtbl; // +0x00
    i32 CallRegister(i32 handle, char* prefix, char* sep);
};
typedef i32 (BootyRegistrar::*BootyRegFn)(i32 handle, char* prefix, char* sep);
SIZE_UNKNOWN(BootyRegistrarVtbl);
struct BootyRegistrarVtbl {
    char m_pad00[0x4c];
    BootyRegFn Register; // +0x4c
};
inline i32 BootyRegistrar::CallRegister(i32 handle, char* prefix, char* sep) {
    return (this->*(m_vtbl->Register))(handle, prefix, sep);
}
SIZE_UNKNOWN(CGruntDataLoader);
struct CGruntDataLoader { // BootyState::m_c->m_4 sub-object
    void Load();          // FUN @ 0x158ee0 __thiscall (reloc-masked)
};
SIZE_UNKNOWN(BootyAssetRoot);
struct BootyAssetRoot { // BootyState::m_c
    char m_pad00[0x4];
    CGruntDataLoader* m_4; // +0x04
    char m_pad08[0x10 - 0x8];
    BootyRegistrar* m_10; // +0x10  vtable-bearing registrar (slot +0x4c)
};
SIZE_UNKNOWN(BootyState);
class BootyState {
public:
    i32 vfunc_9(i32);
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
// vfunc_9 (0x18d30) - the BootyState "bg" activation tick. Like OnActivate but
// installs the "bg" multi-namespace, kicks the grunt-data load + state timer,
// and (when the BOOTY_LOOP ambient sound entry exists and is enabled by
// g_61ab20) re-triggers it on a rate-limited timer keyed off the g_6bf3c0 frame
// counter vs the entry's last-played stamp + interval. The shared game registry
// (g_gameReg) sound chain is modeled minimally for this method's needs.
SIZE_UNKNOWN(BootySndPlayer);
struct BootySndPlayer {
    void Play(i32 token, i32, i32, i32); // FUN_001360d0 __thiscall
};
SIZE_UNKNOWN(BootySndEntry);
struct BootySndEntry {
    char m_pad00[0x10];
    BootySndPlayer* m_10; // +0x10
    u32 m_14;             // +0x14  last-played stamp
    u32 m_18;             // +0x18  interval
};
SIZE_UNKNOWN(BootySndTable);
struct BootySndTable {
    void Find(char* szName, BootySndEntry** out); // FUN_001b8438 __thiscall, out-param
};
SIZE_UNKNOWN(BootySndSet);
struct BootySndSet {
    char m_pad00[0x10];
    BootySndTable m_10; // +0x10  (&m_10 == set+0x10)
    char m_pad11[0x30 - 0x11];
    i32 m_30; // +0x30  active guard
};
SIZE_UNKNOWN(BootySndMgr);
struct BootySndMgr {
    char m_pad00[0x28];
    BootySndSet* m_28; // +0x28
};
// A distinct typed view of the *0x24556c game-registry singleton for this method's
// sound chain (the All.cpp aggregate's shared g_gameReg/CGameReg is defined later
// in ApiCallers.cpp, so this method uses its own correctly-RVA'd alias).
struct BzGameReg {
    char m_pad00[0x30];
    BootySndMgr* m_30; // +0x30
    char m_pad34[0x11c - 0x34];
    i32 m_11c; // +0x11c  sound token
};
DATA(0x0024556c)
extern BzGameReg* g_bzReg;
DATA(0x0021ab20)
extern i32 g_61ab20; // BOOTY_LOOP enable gate
DATA(0x002bf3c0)
extern "C" u32 g_6bf3c0; // wrap-safe draw clock
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
// DrawSaveGameMenu's save-game record arg (modeled just above its definition).
struct CSaveGameMenu;
// StatusBarItem (the in-game status-bar tab item) is modeled in full just above
// its vfunc_16 reconstruction further down (it needs ~15 sub-object types).
class StatusBarItem;
// The icon-key registry object BuildPowerupIconKeys writes into (arg1=esi).
// SetGroup() seeds the "GAME_INGAMEICONZ" group key, Add() appends one entry.
// Both __thiscall, reloc-masked externals.
SIZE_UNKNOWN(PowerupKeyRegistry);
class PowerupKeyRegistry {
public:
    void SetGroup(char* key); // FUN_001b9e74 __thiscall
    void Add(char* key);      // FUN_001ba0c8 __thiscall
};

namespace EngineLabelBacklog {

    void BuildBootyPerfectAnimation();
    void __stdcall BuildPowerupIconKeys(PowerupKeyRegistry* reg, i32 key);
    i32 Stub_01fd70(char* szPath);
    i32 LoadCustomWorldInfo(HWND hDlg);
    // LoadHelpBookSprite reconstructed as CHelpBookSprite::Update below.
    void HandleFortConquered();
    void __stdcall WireTileSwitchLogic(i32, i32, i32);
    void __stdcall LoadTerrainTileSprites(i32, i32, i32, i32, i32, i32);
    void LoadCameraSprite();
    void LoadExplosionSprites();
    void LoadPowerupIconSprites();
    void __stdcall LoadMenuStateAssets(i32, i32, i32);
    void LoadAreaLevelTable();
    void LoadRollingBallHazardSprites();
    void __stdcall LoadLevelByMode(i32, i32);
    void ValidateLevelTiles();
    i32 DrawSaveGameMenu(HWND hDlg, i32 cmd, CSaveGameMenu* obj);
    void BuildLevelTitleString();
    i32 Stub_0f90f0(char* szPath);
    void LoadStatzTabToggleSprite();
    // BuildStatzTabSmall_vfunc1 reconstructed as CStatzTabSmall::BuildSmall below.
    void UpdateGruntOvenStatusBar();
    void UpdateChipGrinderStatusBar();
    void UpdateWarpStoneStatusBar();
    void UpdateDestructButtonStatusBar();
    void LoadSwitchDownSprite();
    void LoadSwitchUpSprite();
    void LoadPyramidBridgeSprites();
    void __stdcall BuildStatzTabSmall_vfunc1(i32, i32, i32, i32, i32, i32, i32, i32, i32);
    void FreeAllFonts();
    void __stdcall BuildVoiceSoundList(i32);
} // namespace EngineLabelBacklog

// @early-stop
// regalloc wall (~95%): retail holds `set` (reg->m_30->m_28) in eax and the play
// entry `res` live in eax with no reload; the /O2 recompile pins `set` in ecx and
// spills/reloads `res` at the Play call. Logic + all externs/strings named.
RVA(0x00018d30, 0xcd)
i32 BootyState::vfunc_9(i32) {
    while (ShowCursor(FALSE) >= 0)
        ;
    if (!RegisterMultiNamespaces("bg", 0, 0, 0, 0, 1)) {
        return 0;
    }
    m_c->m_4->Load();
    StartTimer(0x50, 0x3e8, 0, 1);

    BzGameReg* reg = g_bzReg;
    BootySndSet* set = reg->m_30->m_28;
    i32 token = reg->m_11c;
    if (set->m_30 == 0) {
        BootySndEntry* res = 0;
        set->m_10.Find("BOOTY_LOOP", &res);
        if (res != 0 && g_61ab20 != 0) {
            u32 now = g_6bf3c0;
            if (now - res->m_14 >= res->m_18) {
                res->m_14 = now;
                res->m_10->Play(token, 0, 0, 1);
            }
        }
    }
    return 1;
}

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

// LoadCustomWorldInfo (0x3b7c0) - the custom-world picker dialog helper. Reads the
// level name from the dialog's listbox (id 0x3fc, LB_GETCURSEL/LB_GETTEXT), then
// builds "<gameDir>\Custom\<level>.WWD" through the two global string-key builders
// (g_pathStr/g_levelStr; Set=FUN_001b9e74, Append=FUN_001ba0c8, Reset=FUN_001b9c69,
// the same builder used by BuildPowerupIconKeys). If the file exists it pops the
// CUSTOM_WORLDINFO dialog. Frameless; helpers reloc-masked externals.
struct GameKeyStr {
    char* m_str;          // +0x00  the built C-string
    void Set(char* s);    // FUN_001b9e74 __thiscall
    void Append(char* s); // FUN_001ba0c8 __thiscall
    void Reset();         // FUN_001b9c69 __thiscall
};
DATA(0x0022c25c)
extern GameKeyStr g_pathStr; // 0x62c25c  full path builder
DATA(0x0022c260)
extern GameKeyStr g_levelStr; // 0x62c260  level name
DATA(0x0022c26c)
extern HWND g_customWorldParent; // 0x62c26c
DATA(0x0022c270)
extern HINSTANCE g_customWorldInst; // 0x62c270
// FUN_0011fc10 __cdecl: writes the gruntz game dir into buf (ret nonzero on ok).
extern i32 GetGameDir(char* buf, i32 cb);
// FUN_00004282 __cdecl: tests a path exists (OpenFile probe). ret nonzero on ok.
extern i32 PathFileExists(char* path);
// The CUSTOM_WORLDINFO dialog proc (RVA 0x305d), reloc-masked code pointer.
extern "C" INT_PTR __stdcall CustomWorldInfoDlgProc(HWND, UINT, WPARAM, LPARAM);

RVA(0x0003b7c0, 0x12c)
i32 EngineLabelBacklog::LoadCustomWorldInfo(HWND hDlg) {
    char szLevel[0x100];
    char szDir[0x100];

    HWND hList = GetDlgItem(hDlg, 0x3fc);
    if (!hList) {
        return 0;
    }
    i32 sel = (i32)SendMessageA(hList, 0x188 /*LB_GETCURSEL*/, 0, 0);
    if (sel == -1) {
        return 0;
    }
    if ((i32)SendMessageA(hList, 0x189 /*LB_GETTEXT*/, sel, (LPARAM)szLevel) == -1) {
        return 0;
    }
    g_levelStr.Set(szLevel);
    if (!GetGameDir(szDir, 0xfe)) {
        return 0;
    }
    g_pathStr.Set(szDir);
    g_pathStr.Append("\\Custom\\");
    g_pathStr.Append(szLevel);
    g_pathStr.Append(".WWD");
    if (!PathFileExists(g_pathStr.m_str)) {
        g_pathStr.Reset();
        return 0;
    }
    DialogBoxParamA(
        g_customWorldInst,
        "CUSTOM_WORLDINFO",
        g_customWorldParent,
        (DLGPROC)CustomWorldInfoDlgProc,
        0
    );
    return 1;
}

// HandleFortConquered (0x3f5f0, 1318 B) - the per-frame fort-conquest check on the
// trigger object (`this`), a /GX routine. Structure FULLY decoded (dump_target +
// string_xref): +0x1a0 sub-clock tick (this->m_38->m_1a0.Tick(g_6bf3bc), FUN
// 0x15c360); mode gate on g_mgrSettings->m_134==1 (the simple ProbeA arm FUN
// 0x3120 else the full path); HitTestCell (FUN 0x32ce) + dedup vs owner->m_124; the
// 5-CString "<A> was conquered by <B>!" HUD message (GetName FUN 0x3e54 by value +
// the operator+ chain FUN 0x1b9f81/0x1b9f1b + ShowMessage FUN 0x1483); tag the old
// config entry (m_150[oldArea].m_24=1, stride 0x238); TWO handler-type re-home list
// walks (obj->m_7c->m_10 == LAB_004017e4 / LAB_00403148) moving objects from the
// old area to the found one (re-home helper FUN 0x4165) + a g_freeList (0x645544)
// pop seeding the flag position (FUN 0x1b5144); a per-object Explosion/
// GAME_EXPLOSION3 eye-candy spawn (CreateSprite FUN 0x1597b0 + Configure FUN
// 0x1505b0) for objects inside the area rect; all exits jmp the shared epilogue.
// DEFERRED to the final sweep as a leaf-first regalloc redo: a full-body /GX
// reconstruction (CFortConquerTrigger::HandleConquered, ~200 lines, all callees +
// strings named) was written and BUILDS, but retail pins `this` to ebp (reused as
// the walk cursor) with a 0x24 frame + the canonical /GX prologue, while the /O2
// recompile pins `this` to ebx with a 0x20 frame + hoists the `mov eax,fs:0` SEH
// read - a this-register + frame-slot + prologue-schedule divergence that cascades
// a byte-mismatch through all 1318 B (objdiff 0.0%, BELOW the empty-stub 27.1%
// baseline). Per the breadth-first >512B rule (a partial under-counts AND diverges
// its regalloc), left stubbed until the callee set + owning class + a matching
// regalloc are modeled leaf-first.
// @confidence: med
// @source: decomp-xref
// @proximity: CGruntCreationPoint@-0x930 | CWormhole@+0x680 (boundary - pick one)
// @stub
RVA(0x0003f5f0, 0x526)
void EngineLabelBacklog::HandleFortConquered() {}

// Projectile::vfunc_9 (0x61cb0) graduated to src/Gruntz/ProjectileUpdate.cpp as
// CProjectile::Update - the per-frame projectile impact tick (the 5-slot
// "Projectile"/"Boomerang"/"TimeBomb" eye-candy spawn switch + the AttackDowntime
// bute-tuning tail); @early-stop on the jump-table-data-overlap wall.

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

// BuildRockBreakParticles (0x7b440, 1008 B) graduated to
// src/Gruntz/RockBreakParticles.cpp (eh unit) as
// CRockBreakMgr::BuildRockBreakParticles - the per-tile "rock break" radius scan
// (LEVEL_ROCKBREAK Particlez sprite + rate-limited sound + the giant-rock
// resurrect/"No giant rock logic found" CString diagnostic). Its CString temp
// gives it the /GX exception frame. Reconstructed complete; @early-stop on the
// loop-strength-reduction/regalloc wall (sibling of LoadGruntResurrectTuning).

// ---------------------------------------------------------------------------
// LoadGruntResurrectTuning (0x7be60) - the "resurrect gruntz within a tile
// radius" pass. __thiscall(this, cx, cy, r): walks the owner's grunt list (m_4),
// builds a tile-space RECT from the center (cx>>5, cy>>5) +- r, and for each
// idle grunt (m_5c==0) inside the rect resurrects it. The per-type config entry
// lives in g_mgrSettings (+0x150 + type*0x238); mode m_134==1 reads the
// "Grunt"/"RessurectAIType"+"RessurectAIRadius" tuning from g_buteMgr (unless the
// entry caches m_14), otherwise it uses the entry's m_20/m_24/m_2c/m_14 gates or
// the entry's +0x38 probe. On success it tags the grunt logic (m_38->m_8 |=
// 0x10000), notifies the owner, and spawns a "LightFx" GAME_LIGHTING_FLASH /
// GAME_FLASH eye-candy sprite at the grunt's pixel origin. Only offsets / code
// bytes are load-bearing; helpers are reloc-masked externals.
SIZE_UNKNOWN(ResGruntLogic);
struct ResGruntLogic { // grunt->m_38
    char m_pad00[0x8];
    u32 m_8; // +0x08  flags (|= 0x10000 on resurrect)
};
struct ResHost; // grunt->m_6c (opaque; passed through to Resurrect)
SIZE_UNKNOWN(ResGrunt);
struct ResGrunt {
    char m_pad00[0x38];
    ResGruntLogic* m_38; // +0x38
    char m_pad3c[0x54 - 0x3c];
    i32 m_54; // +0x54  x tile
    i32 m_58; // +0x58  y tile
    i32 m_5c; // +0x5c  busy/skip gate
    char m_pad60[0x68 - 0x60];
    i32 m_68;      // +0x68  grunt type index
    ResHost* m_6c; // +0x6c
};
SIZE_UNKNOWN(ResNode);
struct ResNode {
    ResNode* m_next; // +0x00
    char m_pad04[0x4];
    ResGrunt* m_grunt; // +0x08
};
SIZE_UNKNOWN(ResCfgSub38);
struct ResCfgSub38 {         // config entry + 0x38
    i32 Probe(i32 x, i32 y); // FUN @ 0x3fee __thiscall
};
SIZE_UNKNOWN(ResMgrCfgEntry);
struct ResMgrCfgEntry { // g_mgrSettings + 0x150 + type*0x238
    char m_pad00[0x14];
    i32 m_14; // +0x14
    char m_pad18[0x20 - 0x18];
    i32 m_20; // +0x20
    i32 m_24; // +0x24
    char m_pad28[0x2c - 0x28];
    i32 m_2c; // +0x2c
    char m_pad30[0x38 - 0x30];
    ResCfgSub38 m_38; // +0x38
    char m_pad3c[0x238 - 0x3c];
};
struct ResSprite;
SIZE_UNKNOWN(ResSpriteFactory);
struct ResSpriteFactory { // g_mgrSettings->m_30->m_8
    // FUN_001597b0 __thiscall, ret 0x18: build the named eye-candy sprite.
    ResSprite* CreateSprite(i32 kind, i32 px, i32 py, i32 hint, char* name, i32 flags);
};
SIZE_UNKNOWN(ResFactoryHost);
struct ResFactoryHost {
    char m_pad00[0x8];
    ResSpriteFactory* m_8; // +0x08
};
SIZE_UNKNOWN(ResSettings);
struct ResSettings {
    char m_pad00[0x30];
    ResFactoryHost* m_30; // +0x30
    char m_pad34[0x134 - 0x34];
    i32 m_134; // +0x134  resurrect mode
    char m_pad138[0x150 - 0x138];
    ResMgrCfgEntry m_150[1]; // +0x150  per-type config (stride 0x238)
};
SIZE_UNKNOWN(ResLightCfg);
struct ResLightCfg {                                // spr->m_7c->m_18
    void Configure(char* a, char* b, i32 c, i32 d); // FUN @ 0x2117 __thiscall
};
SIZE_UNKNOWN(ResSpriteCtl);
struct ResSpriteCtl { // spr->m_7c
    char m_pad00[0x10];
    void(__cdecl* Init)(ResSprite*); // +0x10
    char m_pad14[0x18 - 0x14];
    ResLightCfg* m_18; // +0x18
};
SIZE_UNKNOWN(ResSprite);
struct ResSprite {
    char m_pad00[0x7c];
    ResSpriteCtl* m_7c; // +0x7c
};
SIZE_UNKNOWN(ResButeMgr);
struct ResButeMgr {
    i32 GetInt(char* sec, char* key); // CButeMgr::GetInt FUN_00171af0
};
DATA(0x002453d8)
extern ResButeMgr g_resButeMgr;
DATA(0x0024556c)
extern ResSettings* g_resSettings;
SIZE_UNKNOWN(CGruntResurrector);
struct CGruntResurrector {
    char m_pad00[0x4];
    ResNode* m_4; // +0x04  grunt list head
    // FUN_000040bb __thiscall: spawn/resurrect one grunt (ret != -1 on success).
    i32 Resurrect(
        i32 type,
        i32 px,
        i32 py,
        i32 a3,
        i32 a4,
        ResHost* host,
        i32 a6,
        i32 a7,
        i32 aiType,
        i32 radius,
        i32 a10,
        i32 a11,
        i32 a12
    );
    void Notify(ResNode* node); // FUN_001b4ac7 __thiscall
    i32 LoadGruntResurrectTuning(i32 cx, i32 cy, i32 r);
};

// @early-stop
// regalloc/frame-layout wall (~65%): instruction selection, calls, constants,
// strings + the rect/loop/spawn structure are byte-faithful, but retail
// frame-allocates the `node` loop variable (a dedicated 4-byte slot at [esp+0x14]
// inside a 0x18 frame) while this /O2 recompile reuses an incoming-arg slot,
// yielding a 0x14 frame and a +4 cascade across every [esp+N] operand. Not
// source-steerable (the slot-vs-frame choice is the allocator's). Logic complete.
// See docs/patterns/zero-register-pinning.md + const-materialize-into-reg-vs-immediate.md.
RVA(0x0007be60, 0x21e)
i32 CGruntResurrector::LoadGruntResurrectTuning(i32 cx, i32 cy, i32 r) {
    RECT rect;
    i32 hx = cx >> 5;
    i32 hy = cy >> 5;
    rect.left = hx - r;
    rect.right = hx + r;
    rect.top = hy - r;
    rect.bottom = hy + r;

    for (ResNode* node = m_4; node != 0; node = node->m_next) {
        ResGrunt* g = node->m_grunt;
        if (g->m_5c != 0) {
            continue;
        }
        POINT pt;
        pt.x = g->m_54;
        pt.y = g->m_58;
        if (!PtInRect(&rect, pt)) {
            continue;
        }

        i32 type = g->m_68;
        ResSettings* s = g_resSettings;
        ResMgrCfgEntry* cfg = &s->m_150[type];
        i32 aiType = 0;
        i32 ok = 0;
        i32 px = (g->m_54 << 5) + 0x10;
        i32 py = (g->m_58 << 5) + 0x10;

        if (s->m_134 == 1) {
            i32 radius = 0;
            if (cfg->m_14 == 0) {
                aiType = g_resButeMgr.GetInt("Grunt", "RessurectAIType");
                radius = g_resButeMgr.GetInt("Grunt", "RessurectAIRadius");
            }
            if (Resurrect(type, px, py, 0x186a0, 3, g->m_6c, 0, 0, aiType, radius, 0, 0, 0) != -1) {
                ok = 1;
            }
        } else if (cfg->m_20 != 0 && cfg->m_2c == 0 && cfg->m_24 == 0) {
            if (cfg->m_14 != 0) {
                if (Resurrect(type, px, py, 0x186a0, 3, g->m_6c, 0, 0, 0, 0, 0, 0, 0) != -1) {
                    ok = 1;
                }
            } else if (cfg->m_38.Probe(g->m_54, g->m_58) != 0) {
                ok = 1;
            }
        }

        if (ok) {
            g->m_38->m_8 |= 0x10000;
            Notify(node);
            ResSprite* spr =
                g_resSettings->m_30->m_8->CreateSprite(0, px, py, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->Init(spr);
            spr->m_7c->m_18->Configure("GAME_LIGHTING_FLASH", "GAME_FLASH", 8, 1);
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// LoadHelpBookSprite (0x997c0) - the help-book interactive sprite's per-frame
// tick (frameless __thiscall, returns 0). Pings a sub-logic clock
// (m_38->m_1a0.Tick(g_6bf3bc)), hit-tests the owner's tile cell against the area
// map (g_hbMgr->m_68->HitTestCell -> the found object + its (areaId, subId)).
// Bails when nothing is under the cell (and clears the m_38->m_40 active bit), or
// when the cell is outside the current area (areaId != g_644c54), or when the
// (areaId, subId) pair is unchanged from the cached (m_54, m_58) (dedup). It then
// excludes the "K" placeholder type (g_typeColl.IndexToPtr(found->m_14->m_1c),
// strcmp against DAT_0060d7f8 == "K", after touching the g_typeNodes CString
// cache), loads the object's pickup sprites, and - when the owner stands inside
// the area's active rect - plays the rate-limited "GAME_HELPBOOK" sound cue
// (the BootyState::vfunc_9 sound-chain idiom, reusing the Booty* sound types).
// Finally it caches (areaId, subId) and sets the m_38->m_40 active bit. Only
// offsets / code bytes are load-bearing; helpers are reloc-masked externals.
SIZE_UNKNOWN(HbF14);
struct HbF14 {
    char m_pad00[0x1c];
    i32 m_1c; // +0x1c  type index fed to g_typeColl.IndexToPtr
};
SIZE_UNKNOWN(HbFoundObj);
struct HbFoundObj { // HitTestCell result
    char m_pad00[0x14];
    HbF14* m_14;                                    // +0x14
    i32 LoadPickupSprites(i32, i32, i32, i32, i32); // FUN_00403c6a (thunk) __thiscall
};
SIZE_UNKNOWN(HbCellMgr);
struct HbCellMgr { // g_hbMgr->m_68
    // FUN_004035f3 (thunk) __thiscall: hit-test a cell, returning the object +
    // its (areaId, subId) out-params.
    HbFoundObj* HitTestCell(i32 x, i32 y, i32* areaId, i32* subId, i32 flag);
};
SIZE_UNKNOWN(HbSub1a0);
struct HbSub1a0 {         // m_38 + 0x1a0
    void Tick(i32 clock); // FUN_0095c360 __thiscall
};
SIZE_UNKNOWN(HbLogic);
struct HbLogic { // this->m_38
    char m_pad00[0x40];
    i32 m_40; // +0x40  active bit (|=1 on cue, &=~1 on miss)
    char m_pad44[0x1a0 - 0x44];
    HbSub1a0 m_1a0; // +0x1a0
};
SIZE_UNKNOWN(HbOwner);
struct HbOwner { // this->m_10
    char m_pad00[0x5c];
    i32 m_5c; // +0x5c  tile x
    i32 m_60; // +0x60  tile y
    char m_pad64[0x124 - 0x64];
    i32 m_124; // +0x124  pickup-sprite arg
};
SIZE_UNKNOWN(HbMgr);
struct HbMgr { // the *0x64556c singleton, this method's view
    char m_pad00[0x30];
    BootySndMgr* m_30; // +0x30  sound mgr (-> m_28 set -> m_10 table / m_30 gate)
    char m_pad34[0x68 - 0x34];
    HbCellMgr* m_68; // +0x68  cell hit-tester
    char m_pad6c[0x13c - 0x6c];
    i32 m_13c; // +0x13c  area rect (x lo)
    i32 m_140; // +0x140  (y lo)
    i32 m_144; // +0x144  (x hi)
    i32 m_148; // +0x148  (y hi)
};
DATA(0x0024556c)
extern "C" HbMgr* g_mgrSettings; // _g_mgrSettings (the *0x64556c singleton)
// The 4-byte default-constructed CString cache nodes (FUN_001b9b93 == CString
// default ctor; matched array-touch loop). g_typeNodes is the base pointer.
SIZE_UNKNOWN(EngStr4);
struct EngStr4 {
    char* m_pszData; // +0x00 (4 bytes so the loop's `p++` advances by 4)
    void Ctor();     // FUN_001b9b93 __thiscall (CString default ctor)
};
struct CTypeKeyColl {
    char** IndexToPtr(i32 idx); // FUN_00403864 (thunk) __thiscall -> node (*node == name)
};
DATA(0x002bf650)
extern CTypeKeyColl g_typeColl;
DATA(0x002bf66c)
extern void* g_typeNodes;
DATA(0x002bf670)
extern i32 g_typeCount;
DATA(0x002bf3bc)
extern "C" i32 g_6bf3bc; // sub-logic clock fed to HbSub1a0::Tick
DATA(0x00244c54)
extern "C" i32 g_644c54; // current area index
DATA(0x0021ab24)
extern i32 g_sndCueTag; // ?g_sndCueTag@@3HA (HELPBOOK sound token)
DATA(0x0020d7f8)
extern "C" char g_str_K[]; // DAT_0060d7f8 == "K" (placeholder/null type marker)
SIZE_UNKNOWN(CHelpBookSprite);
class CHelpBookSprite {
public:
    i32 Update(); // 0x997c0
    char m_pad00[0x10];
    HbOwner* m_10; // +0x10
    char m_pad14[0x38 - 0x14];
    HbLogic* m_38; // +0x38
    char m_pad3c[0x54 - 0x3c];
    i32 m_54; // +0x54  cached areaId
    i32 m_58; // +0x58  cached subId
};

// @early-stop
// regalloc/scheduling wall (~76%): complete + correct, verified instruction-by-
// instruction vs retail (the whole front half - prologue, HitTestCell, the area/
// dedup guards, the g_typeColl IndexToPtr + CString-cache `while(n--)` loop, the
// inline-strcmp `sete` reject - is byte-exact modulo reloc names). Residual: retail
// loads the __thiscall receivers (found/res) into ecx eagerly and schedules the
// member loads earlier, while this /O2 recompile defers the receiver to ecx after
// the arg pushes - forcing a `res` spill/reload at the Play call and an m_10-reg
// swap at LoadPickupSprites - cascading regalloc from the bounds check through both
// epilogues. Not source-steerable (the receiver-into-ecx timing is the allocator's).
// The 34 ARG_MISMATCH rows are the reloc-name scoring artifact (Ghidra-named helper
// rel32 calls; code bytes match). See docs/patterns/pin-local-for-callee-saved-reg.md.
RVA(0x000997c0, 0x1e7)
i32 CHelpBookSprite::Update() {
    m_38->m_1a0.Tick(g_6bf3bc);

    i32 areaId;
    i32 subId;
    HbFoundObj* found =
        g_mgrSettings->m_68->HitTestCell(m_10->m_5c, m_10->m_60, &areaId, &subId, 1);
    if (found == 0) {
        m_58 = -1;
        m_38->m_40 &= ~1;
        return 0;
    }
    if (areaId != g_644c54) {
        return 0;
    }
    if (m_58 != -1 && areaId == m_54 && subId == m_58) {
        return 0;
    }

    char** node = g_typeColl.IndexToPtr(found->m_14->m_1c);
    EngStr4* p = (EngStr4*)g_typeNodes;
    i32 n = g_typeCount;
    while (n-- != 0) {
        if (p != 0) {
            p->Ctor();
        }
        p++;
    }
    bool eq = (strcmp(*node, g_str_K) == 0);
    if (eq) {
        return 0;
    }

    if (!found->LoadPickupSprites(0x5e, 0, m_10->m_124, 0, 1)) {
        return 0;
    }

    HbOwner* o = m_10;
    i32 x = o->m_5c;
    i32 y = o->m_60;
    if (x < g_mgrSettings->m_144 && x >= g_mgrSettings->m_13c && y < g_mgrSettings->m_148
        && y >= g_mgrSettings->m_140) {
        BootySndSet* set = g_mgrSettings->m_30->m_28;
        if (set->m_30 == 0) {
            BootySndEntry* res = 0;
            set->m_10.Find("GAME_HELPBOOK", &res);
            if (res != 0) {
                i32 enable = g_61ab20;
                i32 token = g_sndCueTag;
                if (enable != 0) {
                    u32 now = g_6bf3c0;
                    if (now - res->m_14 >= res->m_18) {
                        res->m_14 = now;
                        res->m_10->Play(token, 0, 0, 0);
                    }
                }
            }
        }
    }

    m_54 = areaId;
    m_58 = subId;
    m_38->m_40 |= 1;
    return 0;
}

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

// LoadLevelByMode (0x000ca200, 3636 B) is the PLAY game-state per-mode level
// loader (BATTLEZ/MULTI/CUSTOMLEVEL/TRAINING/Level%i); reconstructed (complete
// body + the linear init chain, @early-stop on the /GX megafunction wall) as
// CPlayLevelLoad::LoadByMode in src/Gruntz/LoadLevelByMode.cpp (eh unit). Its
// area-name / WarpStone CString temps give it the exception frame.

// vfunc_12 (0x000cbcc0, 5850 B) is the PLAY-state keyboard/cheat input
// dispatcher, not a status-bar method; reconstructed (complete top structure,
// @early-stop on the megafunction regalloc wall) as CGamePlayInput::DispatchKey
// in src/Gruntz/GameKeyHandler.cpp (frameless base unit).

// ---------------------------------------------------------------------------
// StatusBarItem::vfunc_16 (0xce660) - the in-game status-bar tab item's
// pointer/click dispatcher: vfunc_16(msg, x, y), __thiscall returning a handled
// flag. Gated paths first (m_484 / null child / m_4fc / the global m_68->m_400
// gate / the m_368|m_36c "delegate to base vtable slot +0x38" path). The active
// path: if the child is in mode 2 and the hot point is inside it, fire the
// "GAME_TABHIGHLIGHT1" cue (the Booty sound-chain idiom) + a toggle; else hit-
// test the child's sub-items, then the item's own rect (delegate on a miss),
// then probe the area map; finally, on an in-rect hit it tile-aligns the pixel
// origin and spawns through m_4->m_6c. The *0x64556c game-registry singleton is
// modeled with this method's own correctly-RVA'd typed alias (g_sbiMgr). Only
// offsets / code bytes are load-bearing; helpers are reloc-masked externals.
SIZE_UNKNOWN(SbiSndEntry);
struct SbiSndEntry { // a found sound entry; PlayCue is __thiscall on the entry
    void PlayCue(i32 token, i32, i32, i32); // FUN @ 0x25fe (thunk) __thiscall
};
SIZE_UNKNOWN(SbiSndTable);
struct SbiSndTable {
    void Find(char* szName, SbiSndEntry** out); // FUN_001b8438 __thiscall, out-param
};
SIZE_UNKNOWN(SbiSndSet);
struct SbiSndSet { // m_4->m_30->m_28
    char m_pad00[0x10];
    SbiSndTable m_10; // +0x10
    char m_pad11[0x30 - 0x11];
    i32 m_30; // +0x30  active guard
};
SIZE_UNKNOWN(SbiPoint);
struct SbiPoint {
    i32 x; // +0x00
    i32 y; // +0x04
};
SIZE_UNKNOWN(SbiCoordSrc);
struct SbiCoordSrc { // m_4->m_30->m_24->m_5c
    char m_pad00[0x40];
    SbiPoint m_40; // +0x40
};
SIZE_UNKNOWN(SbiHost24);
struct SbiHost24 { // m_4->m_30->m_24
    char m_pad00[0x10];
    i32 m_10; // +0x10  origin x offset
    i32 m_14; // +0x14  origin y offset
    char m_pad18[0x5c - 0x18];
    SbiCoordSrc* m_5c; // +0x5c
};
SIZE_UNKNOWN(SbiHostInner);
struct SbiHostInner { // m_4->m_30
    char m_pad00[0x24];
    SbiHost24* m_24; // +0x24
    SbiSndSet* m_28; // +0x28
};
SIZE_UNKNOWN(SbiProbe);
struct SbiProbe { // m_4->m_68
    // FUN @ 0x3cb0 __thiscall: probe the area map under (x,y).
    i32 Probe(i32 x, i32 y, i32* outArea, i32* outVal, i32 flag);
};
SIZE_UNKNOWN(SbiSpawner);
struct SbiSpawner { // m_4->m_6c
    // FUN @ 0x2095 __thiscall: spawn the tab eye-candy at a tile origin.
    void Spawn(i32 a1, char area, i32 a3, i32 a4, i32 px, i32 py, i32 a7, i32 a8);
};
SIZE_UNKNOWN(SbiHost);
struct SbiHost { // this->m_4
    char m_pad00[0x30];
    SbiHostInner* m_30; // +0x30
    char m_pad34[0x68 - 0x34];
    SbiProbe* m_68;   // +0x68
    SbiSpawner* m_6c; // +0x6c
};
SIZE_UNKNOWN(SbiRectSrc);
struct SbiRectSrc { // this->m_c->m_24
    char m_pad00[0x10];
    RECT m_10; // +0x10
};
SIZE_UNKNOWN(SbiRectHost);
struct SbiRectHost { // this->m_c
    char m_pad00[0x24];
    SbiRectSrc* m_24; // +0x24
};
SIZE_UNKNOWN(SbiChild);
struct SbiChild {                        // this->m_2dc
    i32 m_0;                             // +0x00  mode tag
    i32 HitTest(i32 x, i32 y);           // FUN @ 0x3ad5 __thiscall, ret index/-1
    void Select(i32 idx, i32 flag);      // FUN @ 0x20b8 __thiscall
    void Refresh();                      // FUN @ 0x123f __thiscall
    void Notify(i32 v);                  // FUN @ 0x4179 __thiscall
    i32 Check();                         // FUN @ 0x3549 __thiscall, ret bool
    i32 Dispatch(i32 msg, i32 x, i32 y); // FUN @ 0x1b81 __thiscall, ret result
};
SIZE_UNKNOWN(SbiToggle);
struct SbiToggle {   // this->m_2e0
    void Set(i32 n); // FUN @ 0x171c __thiscall
};
SIZE_UNKNOWN(SbiEntry);
struct SbiEntry { // this->m_374[i]
    i32 m_0;      // +0x00  center x
    i32 m_4;      // +0x04  center y
};
SIZE_UNKNOWN(SbiMgr68);
struct SbiMgr68 { // g_sbiMgr->m_68
    char m_pad00[0x10c];
    i32 m_10c[1]; // +0x10c  per-area counter array
    char m_pad110[0x400 - 0x110];
    i32 m_400; // +0x400  active gate
};
SIZE_UNKNOWN(SbiCfgEntry);
struct SbiCfgEntry { // g_sbiMgr->m_150[area]  (stride 0x238)
    char m_pad00[0x228];
    i32 m_228; // +0x228  max count
    char m_pad22c[0x238 - 0x22c];
};
SIZE_UNKNOWN(SbiMgr);
struct SbiMgr { // g_sbiMgr (*0x64556c)
    char m_pad00[0x68];
    SbiMgr68* m_68; // +0x68
    char m_pad6c[0x150 - 0x6c];
    SbiCfgEntry m_150[1]; // +0x150
};
DATA(0x0024556c)
extern SbiMgr* g_sbiMgr; // *0x64556c, this method's typed alias
// sub_1c44 (0x1c44 thunk) __stdcall: is the hot point inside the mode-2 child?
i32 __stdcall SbiPointInChild(i32 x, i32 y);
struct SbiVtbl; // completed below (carries the base-handler PMF at slot +0x38)
SIZE_UNKNOWN(StatusBarItem);
class StatusBarItem {
public:
    i32 vfunc_16(i32 msg, i32 x, i32 y); // 0xce660

    SbiVtbl* m_vptr; // +0x000
    SbiHost* m_4;    // +0x004
    char m_pad08[0xc - 0x8];
    SbiRectHost* m_c; // +0x00c
    char m_pad10[0x2dc - 0x10];
    SbiChild* m_2dc;  // +0x2dc
    SbiToggle* m_2e0; // +0x2e0
    char m_pad2e4[0x368 - 0x2e4];
    i32 m_368; // +0x368
    i32 m_36c; // +0x36c
    char m_pad370[0x374 - 0x370];
    SbiEntry** m_374; // +0x374
    i32 m_378;        // +0x378
    char m_pad37c[0x484 - 0x37c];
    i32 m_484; // +0x484
    char m_pad488[0x4fc - 0x488];
    i32 m_4fc; // +0x4fc
};
// The base-class handler at vtable slot +0x38, modeled as a 4-byte PMF off the
// vtable so MSVC emits `mov edx,[this]; mov ecx,this; call [edx+0x38]`. The class
// must be COMPLETE before this typedef - see docs/patterns/pmf-complete-class-4byte.md.
typedef i32 (StatusBarItem::*SbiSlot38Fn)(i32, i32, i32);
SIZE_UNKNOWN(SbiVtbl);
struct SbiVtbl {
    char m_pad00[0x38];
    SbiSlot38Fn slot38; // +0x38
};

// @early-stop
// regalloc/frame-slot wall (~52%): logic complete + verified instruction-by-
// instruction vs retail (every guard, the Dispatch/vtable delegates, the mode-2
// cue arm, HitTest/Select, the rect test, Probe, and the tile-align spawn loop all
// match), and dropping the `m_2dc` local cache already pinned `this` to edi exactly
// like retail (the prologue now matches byte-for-byte modulo the frame size). The
// sole residual: retail's 0x10 frame reuses the dead x-arg home for the Probe
// out-param, while this /O2 build gives it a fresh 5th slot ([esp+0x10]) -> a 0x14
// frame that +4-shifts every [esp+N] operand. The slot-coloring (proving the
// out-param's lifetime can't overlap the early Dispatch paths' x-home reads) is the
// allocator's; not source-steerable. See docs/patterns/zero-register-pinning.md +
// identical-return-epilogue-tailmerge.md.
RVA(0x000ce660, 0x362)
i32 StatusBarItem::vfunc_16(i32 msg, i32 x, i32 y) {
    if (m_484 != 0) {
        return 1;
    }
    if (m_2dc == 0) {
        return 1;
    }
    if (m_4fc != 0) {
        return m_2dc->Dispatch(msg, x, y);
    }
    if (g_sbiMgr->m_68->m_400 == 0) {
        return m_2dc->Dispatch(msg, x, y);
    }
    if (m_368 != 0 || m_36c != 0) {
        return (this->*(m_vptr->slot38))(msg, x, y);
    }

    if (m_2dc->m_0 == 2 && SbiPointInChild(x, y)) {
        SbiSndSet* set = m_4->m_30->m_28;
        if (set->m_30 == 0) {
            SbiSndEntry* e = 0;
            set->m_10.Find("GAME_TABHIGHLIGHT1", &e);
            if (e != 0) {
                e->PlayCue(g_sndCueTag, 0, 0, 0);
            }
        }
        m_2dc->Refresh();
        if (m_2dc->m_0 == 1) {
            m_2e0->Set(2);
        } else {
            m_2e0->Set(1);
        }
        return 1;
    }

    i32 idx = m_2dc->HitTest(x, y);
    if (idx != -1) {
        m_2dc->Select(idx, 1);
        return 1;
    }

    RECT* rc = &m_c->m_24->m_10;
    i32 rl = rc->left;
    i32 rt = rc->top;
    i32 rr = rc->right;
    i32 rb = rc->bottom;
    if (x < rl || x > rr || y < rt || y > rb) {
        return m_2dc->Dispatch(msg, x, y);
    }

    i32 outArea;
    i32 outVal;
    if (m_4->m_68->Probe(x, y, &outArea, &outVal, 5) && g_644c54 == outArea) {
        m_2dc->Notify(outVal);
        return 1;
    }

    if (m_368 != 0) {
        return 1;
    }
    i32 area = g_644c54;
    SbiCfgEntry* cfg = &g_sbiMgr->m_150[area];
    if (cfg == 0) {
        return 0;
    }
    if (g_sbiMgr->m_68->m_10c[area] >= cfg->m_228) {
        return 0;
    }

    SbiHost24* h = m_4->m_30->m_24;
    i32 px = h->m_5c->m_40.x - h->m_10 + x;
    i32 py = h->m_5c->m_40.y - h->m_14 + y;
    for (i32 i = 0; i < m_378; i++) {
        SbiEntry* e = m_374[i];
        if (e == 0) {
            continue;
        }
        RECT er;
        SetRect(&er, e->m_0 - 0x10, e->m_4 - 0x10, e->m_0 + 0x10, e->m_4 + 0x10);
        if (px < er.right && px >= er.left && py < er.bottom && py >= er.top) {
            if (!m_2dc->Check()) {
                return 1;
            }
            char ab = (char)g_644c54;
            px = (px & 0xffe0) + 0x10;
            py = (py & 0xffe0) + 0x10;
            m_4->m_6c->Spawn(1, ab, 0, 0, px, py, 0, 0);
            return 1;
        }
    }
    return 1;
}

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
SIZE_UNKNOWN(CActionResRegistry);
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
SIZE_UNKNOWN(CActionResMgr);
struct CActionResMgr { // this->m_c points here; +0x10 is the registry
    char m_pad00[0x10];
    CActionResRegistry* m_10; // +0x10
};
SIZE_UNKNOWN(CTileSetSource);
struct CTileSetSource {                // this->m_28 points here
    void* LookupTileSet(char* szName); // FUN_0053bae0 __thiscall, ret set ptr
};
DATA(0x002bf37c)
extern i32 g_severusCounterA;
extern "C" char g_emptyString[]; // 0x6293f4

// Typed view of `this` for this loader: m_c is the resource manager, m_28 the
// level's tile-set source.
SIZE_UNKNOWN(CActionTileOwner);
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
SIZE_UNKNOWN(CSoundResRegistry);
struct CSoundResRegistry {                              // this->m_c->+0x28 points here
    i32 Has(char* szName);                              // FUN_004583c0 __thiscall, ret found
    void Register(char* szName, char* szKey);           // FUN_00557c70 __thiscall
    void Install(void* set, char* szName, char* szKey); // FUN_00557ee0 __thiscall
};
SIZE_UNKNOWN(CSoundResMgr);
struct CSoundResMgr { // this->m_c points here; +0x28 is the sound registry
    char m_pad00[0x28];
    CSoundResRegistry* m_28; // +0x28
};
SIZE_UNKNOWN(CSoundSetSource);
struct CSoundSetSource {                // this->m_28 points here
    void* LookupSoundSet(char* szName); // FUN_0053bae0 __thiscall, ret set ptr
};

// Typed view of `this` for this loader: m_c is the sound resource manager, m_28
// the level's sound-set source.
SIZE_UNKNOWN(CSoundOwner);
struct CSoundOwner {
    char m_pad00[0xc];
    CSoundResMgr* m_c; // +0x0c
    char m_pad10[0x28 - 0x10];
    CSoundSetSource* m_28; // +0x28
};

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

// DrawSaveGameMenu (0xe3f40, 982 B) - the save-game dialog's command dispatcher,
// __cdecl(hDlg, cmd, obj). The dialog defers its real command through the
// g_savedMenuCmd global (cmd==1 pulls the stored one), then a 4-way dense-switch
// jump-table cascade decodes the control/button id: a control notification
// (HIWORD==0x100, id in [0x435..0x43e]) latches the pending command; ids
// [0x49a..0x4a3] pop the "GAME_INFO" dialog per slot; [0x4a4..0x4ad] pop
// "GAME_DELETE" then delete; [0x490..0x499] read the slot name box, prompt
// "GAME_OVERWRITE" when the slot is used, then commit + EndDialog (logging
// "ERROR - Cannot Save Game." on failure). Frameless; every callee is a reloc-
// masked external, the ids/strings are the load-bearing bytes.
SIZE_UNKNOWN(CSaveGameMenu);
struct CSaveGameMenu {                             // arg3: the save-game record
    i32 GetSlotState(i32 slot);                    // FUN @ 0x3bcf __thiscall
    void WriteName(i32 slot, char* nm, void* mgr); // FUN @ 0x219e __thiscall
    i32 CommitSlot(i32 a, i32 b);                  // FUN @ 0x2d97 __thiscall
};
SIZE_UNKNOWN(SaveMenuMgr);
struct SaveMenuMgr { // the *0x64556c singleton, this method's alias
    i32 Prompt(const char* name, void* proc, i32 flag); // FUN @ 0x2bb7 __thiscall
    void Notify(i32 state, char* nm);                   // FUN @ 0x24c3 __thiscall
    void Log(const char* s);                            // FUN @ 0x417e __thiscall
};
DATA(0x0024556c)
extern SaveMenuMgr* g_saveMenuMgr; // *0x64556c, this method's alias
DATA(0x00213a9c)
extern i32 g_savedMenuCmd; // DAT_00613a9c  pending deferred command
DATA(0x0024c864)
extern i32 g_slotState; // DAT_0064c864  last-queried slot state
// FUN_00002694 __cdecl / FUN_00002e05 __cdecl (thunks): slot-used test / delete.
i32 SlotHasSave(i32 state);
void DeleteSaveSlot(HWND hDlg, CSaveGameMenu* obj);
// The three dialog sub-proc thunks passed as callbacks (reloc-masked code ptrs).
extern "C" void SaveInfoProc();
extern "C" void SaveDeleteProc();
extern "C" void SaveOverwriteProc();

// @early-stop
// jump-table scoring wall: the code bytes match the retail dispatch (the four
// dense-case jump tables, the deferred-command latch, the GAME_INFO/DELETE/
// OVERWRITE/save arms, EnableWindow/GetDlgItemText/EndDialog gates and the two
// return tails), but MSVC emits each switch's jump table as its own `$Lnnn`
// COMDAT while the delinker inlines the four `switchdataD_*` tables into the
// function body, so the four `jmp [id*4+table]` base relocs + the table data
// don't pair (see docs/patterns/switch-jumptable-separate-comdat.md +
// jumptable-data-overlap.md). Logic complete; not source-steerable.
RVA(0x000e3f40, 0x3d6)
i32 EngineLabelBacklog::DrawSaveGameMenu(HWND hDlg, i32 cmd, CSaveGameMenu* obj) {
    i32 c;
    if (cmd == 1) {
        c = g_savedMenuCmd;
        if (c == -1) {
            return 0;
        }
    } else {
        c = cmd;
    }

    // Latch a pending command from a control notification.
    if (((u32)c >> 16) == 0x100) {
        switch (c & 0xffff) {
            case 0x435:
                g_savedMenuCmd = 0x490;
                break;
            case 0x436:
                g_savedMenuCmd = 0x491;
                break;
            case 0x437:
                g_savedMenuCmd = 0x492;
                break;
            case 0x438:
                g_savedMenuCmd = 0x493;
                break;
            case 0x439:
                g_savedMenuCmd = 0x494;
                break;
            case 0x43a:
                g_savedMenuCmd = 0x495;
                break;
            case 0x43b:
                g_savedMenuCmd = 0x496;
                break;
            case 0x43c:
                g_savedMenuCmd = 0x497;
                break;
            case 0x43d:
                g_savedMenuCmd = 0x498;
                break;
            case 0x43e:
                g_savedMenuCmd = 0x499;
                break;
        }
    }

    // GAME_INFO buttons.
    i32 info;
    switch (c) {
        case 0x49a:
            info = 0;
            break;
        case 0x49b:
            info = 1;
            break;
        case 0x49c:
            info = 2;
            break;
        case 0x49d:
            info = 3;
            break;
        case 0x49e:
            info = 4;
            break;
        case 0x49f:
            info = 5;
            break;
        case 0x4a0:
            info = 6;
            break;
        case 0x4a1:
            info = 7;
            break;
        case 0x4a2:
            info = 8;
            break;
        case 0x4a3:
            info = 9;
            break;
        default:
            info = -1;
            break;
    }
    if (info != -1) {
        g_slotState = obj->GetSlotState(info);
        if (g_slotState == 0) {
            return 0;
        }
        EnableWindow(hDlg, FALSE);
        g_saveMenuMgr->Prompt("GAME_INFO", (void*)SaveInfoProc, 0);
        EnableWindow(hDlg, TRUE);
        return 0;
    }

    // GAME_DELETE buttons.
    i32 del;
    switch (c) {
        case 0x4a4:
            del = 0;
            break;
        case 0x4a5:
            del = 1;
            break;
        case 0x4a6:
            del = 2;
            break;
        case 0x4a7:
            del = 3;
            break;
        case 0x4a8:
            del = 4;
            break;
        case 0x4a9:
            del = 5;
            break;
        case 0x4aa:
            del = 6;
            break;
        case 0x4ab:
            del = 7;
            break;
        case 0x4ac:
            del = 8;
            break;
        case 0x4ad:
            del = 9;
            break;
        default:
            del = -1;
            break;
    }
    if (del != -1) {
        g_slotState = obj->GetSlotState(del);
        if (g_slotState == 0) {
            return 0;
        }
        EnableWindow(hDlg, FALSE);
        i32 ok = g_saveMenuMgr->Prompt("GAME_DELETE", (void*)SaveDeleteProc, 0);
        EnableWindow(hDlg, TRUE);
        if (ok == 0) {
            return 0;
        }
        DeleteSaveSlot(hDlg, obj);
        return 0;
    }

    // Save / overwrite buttons.
    i32 slot = -1;
    switch (c) {
        case 0x490:
            slot = 0;
            break;
        case 0x491:
            slot = 1;
            break;
        case 0x492:
            slot = 2;
            break;
        case 0x493:
            slot = 3;
            break;
        case 0x494:
            slot = 4;
            break;
        case 0x495:
            slot = 5;
            break;
        case 0x496:
            slot = 6;
            break;
        case 0x497:
            slot = 7;
            break;
        case 0x498:
            slot = 8;
            break;
        case 0x499:
            slot = 9;
            break;
    }
    if (slot == -1) {
        return 0;
    }

    char name[0x20];
    GetDlgItemTextA(hDlg, 0x435 + slot, name, 0x20);
    if (_strcmpi(name, "(Empty)") == 0) {
        sprintf(name, "Saved Game #%i", slot + 1);
    }
    if (SlotHasSave(obj->GetSlotState(slot))) {
        g_slotState = obj->GetSlotState(slot);
        if (g_slotState != 0) {
            EnableWindow(hDlg, FALSE);
            i32 ok = g_saveMenuMgr->Prompt("GAME_OVERWRITE", (void*)SaveOverwriteProc, 0);
            EnableWindow(hDlg, TRUE);
            if (ok == 0) {
                return 1;
            }
        }
    }
    obj->WriteName(slot, name, g_saveMenuMgr);
    g_saveMenuMgr->Notify(obj->GetSlotState(slot), name);
    EndDialog(hDlg, 1);
    if (!obj->CommitSlot(obj->GetSlotState(slot) + 0x35, 0x81a6)) {
        g_saveMenuMgr->Log("ERROR - Cannot Save Game.");
    }
    return 1;
}

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
SIZE_UNKNOWN(CStatzRect60);
struct CStatzRect60 {
    i32 d[0x18]; // 0x60 bytes
};
struct CStatzSprite;
SIZE_UNKNOWN(CStatzSpriteFactory);
struct CStatzSpriteFactory {
    // FUN_001597b0 __thiscall, ret 0x18: build the named tab sprite.
    CStatzSprite* CreateSprite(i32 kind, i32 px, i32 py, i32 hint, void* name, i32 flags);
};
SIZE_UNKNOWN(CStatzFactoryHolder);
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
SIZE_UNKNOWN(CStatzSpriteInitVtbl);
struct CStatzSpriteInitVtbl {
    void* m_slot0[4];                   // slots 0..3
    void(__cdecl* Init)(CStatzSprite*); // slot 4 (+0x10)
};
SIZE_UNKNOWN(CStatzSprite);
struct CStatzSprite {
    char m_pad0[0x7c];
    CStatzSpriteInitVtbl* m_7c; // +0x7c
    char m_pad80[0x198 - 0x80];
    i32 m_198;                        // +0x198
    void Configure(void* tag, i32 a); // FUN_001504d0 __thiscall
};
DATA(0x0020aa34)
extern char g_statzTabSpriteName[]; // CreateSprite name buffer
DATA(0x0020f928)
extern char g_statzTabCfgTag[]; // Configure tag global
SIZE_UNKNOWN(CStatzTabSmall);
struct CStatzTabSmall {
    virtual i32 BaseBuild(i32, i32, i32, i32, i32, i32, i32, i32); // slot 0 (reloc-masked)
    i32
    BuildSmall(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, CStatzRect60* a6, i32 a7, i32 a8, i32 a9);
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
i32 CStatzTabSmall::
    BuildSmall(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, CStatzRect60* a6, i32 a7, i32 a8, i32 a9) {
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

// FormatGameInfoString (0x1183b0) - builds a URL/POST query string describing a
// saved game into the global accumulator g_infoMaster, by sprintf'ing each piece
// into the scratch buffer g_infoScratch and strcat-appending it. Pieces:
//   "Name=%s&Type=%i&Location=%s&Version=%lu"  (name buf, type, location buf, ver)
//   "&S=%lu&H=%i&M=%02i&SE=%02i"               (timestamp decompose: S/H/M/SE)
//   "&Month=%i&Day=%i&Year=%i"
//   "&Checksum=%lu"                            (a fold of the time fields)
// then it URL-encodes spaces to '+'. Frameless __thiscall on the save-info record;
// the time sub-object lives at this+0xb8 (0x1c bytes). Only offsets / code bytes
// are load-bearing; the sprintf/strcat/strlen/memset are reloc-masked CRT
// (inlined intrinsics), and Check1/ValidateGameTime/DecodeGameTime are reloc-
// masked engine helpers.
SIZE_UNKNOWN(CGameInfoTime);
struct CGameInfoTime { // this+0xb8 (0x1c bytes; zeroed on a failed validate)
    i32 m_0;           // +0x00 (this+0xb8)
    u32 m_4;           // +0x04 (this+0xbc)  S (seconds, %lu)
    i32 m_8;           // +0x08 (this+0xc0)  timestamp fed to DecodeGameTime
    i32 m_c;           // +0x0c (this+0xc4)  Month
    i32 m_10;          // +0x10 (this+0xc8)  Day
    i32 m_14;          // +0x14 (this+0xcc)  Year
    i32 m_18;          // +0x18 (this+0xd0)
};
// FUN_00118310 __cdecl(&time) -> validity flag; FUN_00119210 __cdecl(ts, &a, &b,
// &c) -> decompose the timestamp into three out-values.
i32 ValidateGameTime(CGameInfoTime* t);                       // 0x118310
void DecodeGameTime(i32 ts, i32* outA, i32* outB, i32* outC); // 0x119210
SIZE_UNKNOWN(CGameInfo);
class CGameInfo {
public:
    i32 Check1();               // FUN_001182f0 __thiscall (ready/dirty gate)
    i32 FormatGameInfoString(); // 0x1183b0

    char m_pad0[0x8];
    u32 m_8; // +0x08  Version (%lu)
    char m_pad0c[0x14 - 0xc];
    char m_14[0x36 - 0x14]; // +0x14  Name buffer
    char m_36[0xb8 - 0x36]; // +0x36  Location buffer
    CGameInfoTime m_b8;     // +0xb8
    u32 m_d4;               // +0xd4  Type (%i)
};
DATA(0x0024ecf8)
extern char g_infoMaster[0x800]; // 0x64ecf8  query accumulator
DATA(0x0024ebf8)
extern char g_infoScratch[0x100]; // 0x64ebf8  per-piece scratch

RVA(0x001183b0, 0x211)
i32 CGameInfo::FormatGameInfoString() {
    char* name = m_14;
    if (name == 0) {
        return 0;
    }
    if (strlen(name) == 0) {
        return 0;
    }
    if (!Check1()) {
        return 0;
    }

    g_infoMaster[0] = 0;
    sprintf(g_infoScratch, "Name=%s&Type=%i&Location=%s&Version=%lu", name, m_d4, m_36, m_8);
    strcat(g_infoMaster, g_infoScratch);

    CGameInfoTime* t = &m_b8;
    if (t == 0) {
        return 0;
    }
    if (!ValidateGameTime(t)) {
        memset(t, 0, 28);
    }

    i32 a = 0, b = 0, c = 0;
    DecodeGameTime(t->m_8, &a, &b, &c);
    sprintf(g_infoScratch, "&S=%lu&H=%i&M=%02i&SE=%02i", t->m_4, a, b, c);
    strcat(g_infoMaster, g_infoScratch);

    sprintf(g_infoScratch, "&Month=%i&Day=%i&Year=%i", t->m_c, t->m_10, t->m_14);
    strcat(g_infoMaster, g_infoScratch);

    i32 chk = (69 * (b * a) + 1) * c + b + a + t->m_c + t->m_14 + t->m_10 + t->m_4;
    sprintf(g_infoScratch, "&Checksum=%lu", chk);
    strcat(g_infoMaster, g_infoScratch);

    if (g_infoMaster[0] != 0) {
        for (char* p = g_infoMaster; *p != 0; p++) {
            if (*p == ' ') {
                *p = '+';
            }
        }
    }
    return 0;
}

// CRT lowio/startup internals (FID-anchored in config/library_labels.csv as
// __findfirst / __write_lk / __sopen / __lseek_lk / __read_lk / __NMSG_WRITE);
// the library label is canonical, so the redundant backlog stubs were removed:
//   0x0011f900 __findfirst   0x0012abf0 __NMSG_WRITE  0x0012d230 __write_lk
//   0x0012d460 __sopen       0x0012d880 __lseek_lk    0x001315d0 __read_lk

// 0x18c780 _stat/_wstat core (statically-linked CRT); reclassified as library
// (config/library_labels.csv __stat) - was an empty RVA stub, now carved out of the
// match universe (not a reconstruction target).
//
// _tr_init (0x188440) / _ct_init (0x1884c0) are zlib trees.c, already matched by the
// vendor `trees` unit; the dead EngineLabelBacklog decls above bound no RVA (they
// were vestigial), so they are removed here.

// Stub_1bf8f8 / Stub_1c1609 / Stub_1c176a (the Win32 file-path/time-info utility
// family, all in the 0x1bf*-0x1d5* `framed` ebp-frame module) graduated to
// src/Gruntz/FilePathInfo.cpp (framed + /GX) as CanonicalizePath / GetFileTimeInfo
// / RestoreFileTimeInfo - GetFullPathName canonicalization plus a FindFirstFile/
// SetFileTime timestamp+attribute snapshot/restore pair. The volume-root CString
// temp gives CanonicalizePath its /GX exception frame (@early-stop: the toolchain
// never emits the retail `call __EH_prolog`, a wall shared with ConfigStoreRead).

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
