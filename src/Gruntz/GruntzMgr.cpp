// GruntzMgr.cpp - CGruntzMgr, the Gruntz game manager (the real derived
// WAP32::CGameMgr; C:\Proj\Gruntz). This is the 0xa30-byte game manager that
// CGruntzApp::InitializeGameManager allocates (`new CGruntzMgr`); the base
// WAP32::CGameMgr is the genuine 0x2c engine class.
//
// Reconstructed here:
//   CGruntzMgr::ReportError        - forwards to the app's ReportError (m_8
//       holds the CGameApp; call its vtbl slot +0x1c).
//   CGruntzMgr::GetGruntzDriveLetter - memoised CD drive-letter accessor
//       (Utils::WinAPI::GetGruntzDriveLetter on first call).
//   CGruntzMgr::~CGruntzMgr        - virtual dtor: runs UnknownClose() then the
//       compiler-generated member/base teardown under the /GX C++ EH frame.
//   the scalar-deleting destructor (vtable slot 0) is auto-emitted by MSVC.
//
// The ctor is the member-construction-heavy 0x083030 (deferred to the stub).
// <Mfc.h> brings <windows.h> KERNEL32 (GetCurrentDirectoryA; DWORD) and the central
// WINMM timeGetTime decl (the per-frame draw clock).
#include <Mfc.h>
#include <Gruntz/CGameRegistry.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Enums.h>
#include <Io/FileStream.h> // CFileIO (the engine file reader IsBattlezMapFile opens)
#include <ComDefs.h>       // STDMETHOD / HRESULT - the world-dispatch / DirectPlayLobby COM macros
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked) for the toggle-message formatter
#include <string.h> // engine strstr (reloc-masked) for the Battlez header probe
#include <Globals.h>

// ---------------------------------------------------------------------------
// The two COM-interface views CGruntzMgr dispatches on. Defined HERE (the only TU
// that calls their slots) rather than in the widely-included class header, so the
// STDMETHOD virtual-interface footprint stays out of GruntzMgr.h's ~14 includers.
// ---------------------------------------------------------------------------

// A polymorphic world sub-object (CWorldZ::m_1c -> *m_1c): abstract __stdcall COM
// interface, only slot 10 (+0x28) is dispatched. STDMETHOD_(void, ...) ==
// `virtual void __stdcall`, so `d->Slot0a()` lowers to `mov eax,[d]; call [eax+0x28]`.
SIZE_UNKNOWN(CWorldDispatch);
struct CWorldDispatch {
    STDMETHOD_(void, v00)() PURE;
    STDMETHOD_(void, v01)() PURE;
    STDMETHOD_(void, v02)() PURE;
    STDMETHOD_(void, v03)() PURE;
    STDMETHOD_(void, v04)() PURE;
    STDMETHOD_(void, v05)() PURE;
    STDMETHOD_(void, v06)() PURE;
    STDMETHOD_(void, v07)() PURE;
    STDMETHOD_(void, v08)() PURE;
    STDMETHOD_(void, v09)() PURE;
    STDMETHOD_(void, Slot0a)() PURE; // slot 10 (+0x28)
};

// IDirectPlayLobby-shaped COM interface (CGruntzMgr::m_lobby): abstract __stdcall,
// STDMETHOD form; only Release (slot 2) + GetConnectionSettings (slot 8, called
// twice in the size-probe / fill idiom) are pinned.
SIZE_UNKNOWN(IDirectPlayLobbyZ);
struct IDirectPlayLobbyZ {
    STDMETHOD(QueryInterface)(void* riid, void* out) PURE; // slot 0
    STDMETHOD(v01)() PURE;                                 // slot 1
    STDMETHOD(Release)() PURE;                             // slot 2  (+0x08)
    STDMETHOD(v03)() PURE;                                 // slot 3
    STDMETHOD(v04)() PURE;                                 // slot 4
    STDMETHOD(v05)() PURE;                                 // slot 5
    STDMETHOD(v06)() PURE;                                 // slot 6
    STDMETHOD(v07)() PURE;                                 // slot 7
    STDMETHOD(GetConnectionSettings)(u32 appId, void* lpData, u32* lpdwSize) PURE; // slot 8 (+0x20)
};

namespace Utils {
    namespace WinAPI {
        char GetGruntzDriveLetter();
        i32 FileExists(char* szPath);
    } // namespace WinAPI
} // namespace Utils

// External DirectPlay surface used by InitializeLobbyConnectionSettings. The
// DPLAYX export DirectPlayLobbyCreate (ordinal 4) and CNetMgr::ReportError are
// reached as `call rel32`/[IAT] - the displacement reloc-masks, so only the
// arg shapes are load-bearing. `m_connSettings` (the connection-settings buffer) is freed
// through a small NAFXCW-style operator-delete helper (out-of-line, reloc-
// masked); modelled as a plain free function.
extern "C" __declspec(dllimport) i32 __stdcall DirectPlayLobbyCreate(
    void* lpguidDSP,
    IDirectPlayLobbyZ** lplpDPL,
    void* pUnk,
    void* lpData,
    u32 dwDataSize
);

class CNetMgr {
public:
    static void ReportError(char* file, i32 line, i32 hr, void* hWnd);
};

void FreeConnectionSettings(void* p); // FUN_005b9b82 (operator delete wrapper)

void* operator new(u32);
void operator delete(void*); // ??3@YAXPAX@Z (FUN_005b9b82) - scalar/member teardown

// Cached Win32/WINMM API entry points the engine calls through game-owned global
// fn-ptr slots (`call ds:[ptr]` / `mov reg,ptr; call reg`), NOT the IAT. Each is a
// typed fn-ptr global pinned by its DATA() RVA (the indirect-call displacement
// reloc-masks; the DATA pin pairs the named slot). Same model as g_pShowCursor.
extern "C" {
    DATA(0x002c4650)
    extern u32(WINAPI* g_pTimeGetTime)(); // PTR_timeGetTime_006c4650
    DATA(0x002c44a4)
    extern i32(WINAPI* g_pSendMessageA)(
        i32 hwnd,
        u32 msg,
        i32 wp,
        i32 lp
    ); // PTR_SendMessageA_006c44a4
    DATA(0x002c44c8)
    extern i32(WINAPI* g_pPostMessageA)(
        i32 hwnd,
        u32 msg,
        i32 wp,
        i32 lp
    ); // PTR_PostMessageA_006c44c8
    DATA(0x002c4550)
    extern i32(WINAPI* g_pDialogBoxParamA)(
        i32 hInst,
        const char* tmpl,
        i32 hwnd,
        void* dlgProc,
        i32 param
    ); // PTR_DialogBoxParamA_006c4550
}

// Game-clock/registry globals reached by AccrueScoreTime / UnknownClose.
extern "C" {
    DATA(0x00248ce8)
    extern i32 g_648ce8; // DAT_00648ce8  (timeGetTime base stamp)
}

// AccrueScoreTime's engine views. g_gameReg->m_7c is the registry's HUD/score
// accumulator (Refresh at the 0x1884 thunk; a running total at +0x10). The live
// state carries a tally id at +0x1c and a 64-bit level clock pointer at +0x3f4
// (->m_38). m_cmdGrid carries a "scored" flag at +0x288.
struct RegScoreHud {
    char m_pad0[0x10];
    i32 m_10;                // +0x10  running accumulator
    void Refresh(i32 score); // FUN @ 0x1884 thunk (this, score) reloc-masked
};
struct GameRegHudView {
    char m_pad0[0x7c];
    RegScoreHud* m_7c; // +0x7c
};
struct LevelClock {
    char m_pad0[0x38];
    i64 m_38; // +0x38  64-bit elapsed clock
};
struct StateScoreView {
    char m_pad0[0x1c];
    i32 m_1c; // +0x1c  tally id
    char m_pad20[0x3f4 - 0x20];
    LevelClock* m_3f4; // +0x3f4
};
struct CmdGridFlagView {
    char m_pad0[0x288];
    i32 m_288; // +0x288  per-grid scored flag
};

// DelayedQuit's menu lookup. The world's +0x28 sub-object holds a keyed map at
// +0x10; Lookup(key, &out) resolves a named menu node, whose +0x10 sub-object's
// +0x28 holds a base timestamp (offset by 0x1f4). Each engine call is reloc-masked.
// Distinct from MenuPage.cpp's CMenuMap (CMapStringToPtr, Lookup 0x1b8008): this
// world-menu keyed map's Lookup targets FUN_005b8438.
struct CWorldMenuMap {
    void Lookup(const char* key, void** out); // FUN_005b8438 (this, key, &out)
};
struct CMenuNodeSub {
    char m_pad0[0x28];
    i32 m_28; // +0x28
};
struct CMenuNode {
    char m_pad0[0x10];
    CMenuNodeSub* m_10; // +0x10
};
struct CWorldMenuHolder {
    char m_pad0[0x10];
    CWorldMenuMap m_10; // +0x10  embedded keyed map (sub-object)
};

// UnknownClose's teardown vocabulary. Most owned sub-objects share a parameterless
// thiscall teardown then operator delete (modeled as one EngObj type - the per-call
// displacement reloc-masks). m_30/m_3c are torn down through their own vtable slot 1
// (a flagged scalar-delete), m_38 is the settings/registry writer (WriteInt per
// named key), and g_645578 is zeroed field-by-field before delete. Each engine
// entrypoint is out-of-line / reloc-masked.
struct EngObj {
    void Teardown(); // (this) reloc-masked
};
class CWorldDelete {
public:
    virtual void s0();            // slot 0 (+0x00)
    virtual void Slot1(i32 flag); // slot 1 (+0x04) flagged scalar-delete
};
struct CSettingsWriter {
    void WriteInt(const char* key, i32 value); // FUN_00539460 (this, key, value) reloc-masked
};
struct StateMgr578Z {
    i32 m_0, m_4, m_8; // +0x00..+0x08
    char m_padc[0x10 - 0xc];
    i32 m_10, m_14; // +0x10, +0x14
};
// The settings store open/close brackets around the WriteInt block (reloc-masked
// __cdecl free fns; no this).
void OpenSettingsStore();  // FUN_005158f0
void CloseSettingsStore(); // FUN_004f8e20

// The modal dialog OnCheckpointReached pops: a destructible stack local built by
// FUN_004234a0(this, 0) and torn down by FUN_005ba51d, handed to ExitModalUI as a
// CModalDialog. Its ctor/dtor reloc-mask; only the size + destructibility (the /GX
// frame) are load-bearing.
struct CCheckpointDlg {
    CCheckpointDlg(i32 a); // FUN_004234a0 (this, 0)
    ~CCheckpointDlg();     // FUN_005ba51d
    char m_pad[0x5c];
};

// The save-as name dialog SaveGameAs pops (0x14b30 ctor(owner, 0), run through
// ExitModalUI). Its entered name is a CString member behind the CDialog base; the
// use-"custom\" flag sits at +0x68. The class's implicit destructor destructs the
// CString member then the CDialog base (reloc-masked ~CString + CDialog::~CDialog
// 0x1ba51d) - the two /GX destructibles this method's EH frame tracks.
class CSaveDlgBase {
public:
    virtual ~CSaveDlgBase(); // 0x1ba51d  CDialog::~CDialog (virtual, reloc-masked)
};
class CSaveNameDlg : public CSaveDlgBase {
public:
    CSaveNameDlg(class CGruntzMgr* owner, i32 flag); // 0x14b30
    char m_pad04[0x68 - 0x4];
    i32 m_68;     // +0x68  use-custom-prefix flag
    CString m_6c; // +0x6c  entered name
};

// Resets the 17 sound-channel slots (g_64c3f0[17] = 1); SaveGameAs calls it before
// popping the modal. Reloc-masked __cdecl free fn (0xdb1d0).
void ChannelSlots_InitAll(); // 0xdb1d0

// The Win32 dialog procedures handed to RunModalDialog. Their pushed code
// addresses reloc-mask (DIR32 against the named LAB_ symbols); only the push
// shape is load-bearing.
extern "C" void GruntzLoadGameDlgProc();    // LAB_00402167
extern "C" void GruntzDebugGruntTypeProc(); // LAB_004021e9
extern "C" void GruntzSaveGameDlgProc();    // LAB_00401041 (GAME_SAVE)
extern "C" void GruntzSaveMsgDlgProc();     // LAB_004011d1 (GAME_SAVEMSG)

// -------------------------------------------------------------------------
// Engine objects reached through CGruntzMgr's member pointers. Each engine-side
// method is a reloc-masked __thiscall (`mov ecx,obj; call rel32`), so only the
// layout offsets + call shapes are load-bearing; the displacements reloc-mask.
extern "C" {
    extern i32 g_61ab24; // DAT_0061ab24  (last-stored input flag mirror)
    extern i32 g_64557c; // DAT_0064557c  (modal/cursor-busy gate)
    // The clock/scroll/warp globals SaveState streams through the archive.
    extern i32 g_629ad0; // DAT_00629ad0  (save serial counter)
    extern i32 g_64558c; // DAT_0064558c
    extern i32 g_645590; // DAT_00645590
    extern i32 g_645594; // DAT_00645594
    extern i32 g_645598; // DAT_00645598
    extern i32 g_64559c; // DAT_0064559c
    extern i32 g_6455a0; // DAT_006455a0
    extern i32 g_6455e8; // DAT_006455e8
    extern i32 g_6452a4; // DAT_006452a4
    extern i32 g_6452cc; // DAT_006452cc
    extern i32 g_645508; // DAT_00645508
    extern i32 g_64550c; // DAT_0064550c
}

extern "C" {
    extern i32 g_644c54; // DAT_00644c54  (active player/world index)
}

// The reentrancy/run-state gate SetRunState mirrors the new run-state into
// (?g_61ab20@@3HA; reloc-masked DATA store - the same global ChatBox/GameMode touch).
extern i32 g_61ab20; // DAT_0061ab20

// The game registry singleton (?g_gameReg@@3PAUWwdGameReg@@A), modeled here with
// the offsets UpdateScoreHud touches (a per-TU view; the DATA pin reloc-masks the
// `mov eax,ds:g_gameReg` load against the already-named symbol).
struct ScoreNotifier {         // g_gameReg->m_58
    void Bump(i32 wp);         // FUN @ 0x4408 thunk (this, wp)
    void Tick(i32 wp);         // FUN @ 0x1c53 thunk (this, wp)
    i32 Notify(i32 a, i32 wp); // FUN @ 0x2d97 thunk (this, a, wp) -> nonzero=ok
};
struct ScoreSub2c { // g_gameReg->m_2c
    char m_pad0[0x1c];
    i32 m_1c; // +0x1c  cumulative score
};
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// The +0x68 per-world delta tables: two parallel int arrays at +0x20c / +0x21c
// indexed by g_644c54.
struct WorldDeltaTables {
    char m_pad0[0x20c];
    i32 m_arr20c[4]; // +0x20c..+0x21c
    i32 m_arr21c[4]; // +0x21c
};

// The +0x7c HUD/score accumulator object.
struct ScoreHud {
    char m_pad0[0x8];
    i32 m_8; // +0x08  refresh flag
    char m_padc[0x1c - 0xc];
    i32 m_1c;                // +0x1c  accumulator A
    i32 m_20;                // +0x20  accumulator B
    void Refresh(i32 score); // FUN @ 0x1884 thunk (this, score)
    void Seed(i32 v, i32 z); // FUN @ 0x1c8f thunk (this, v, z)
};

// The +0x44 object carrying a one-shot guard at +0x124.
struct HudGuard44 {
    char m_pad0[0x124];
    i32 m_124; // +0x124
};

// The archive/serializer object SaveState streams every clock/scroll/warp field
// through; its Serialize(void* data, int size) lives at vtbl slot +0x30 and is
// reloc-masked. Model it as a typed vtable so each `mov ecx,ar; push n; push &f;
// call [ar+0x30]` falls out (MSVC 5.0 forbids __thiscall on a fn-ptr, so the
// slots are anchors and Serialize is a virtual member).
class CSerializerZ {
public:
    virtual void s00();
    virtual void s01();
    virtual void s02();
    virtual void s03();
    virtual void s04();
    virtual void s05();
    virtual void s06();
    virtual void s07();
    virtual void s08();
    virtual void s09();
    virtual void s0a();
    virtual void Transfer(void* data, i32 size);  // slot 11 (+0x2c) (Load read path)
    virtual void Serialize(void* data, i32 size); // slot 12 (+0x30)
};

// The engine reaches the USER32 cursor through a stored function pointer at
// 0x6c44c4 (`mov edi,ds:...; call edi`), not a direct IAT call; model it as a
// typed global fn-ptr so the indirect call shape (`call reg`) falls out.
extern "C" i32(WINAPI* g_pShowCursor)(i32); // PTR_ShowCursor_006c44c4

// A free helper that flushes/forces a redraw of one map index (reloc-masked).
void RedrawMapIndex(i32 idx); // FUN_00558c70

// The trailing __cdecl command hook in BroadcastCmd's fan-out (reloc-masked;
// `push a3..a0; call; add esp,0x10`).
i32 CmdHook(i32 a, i32 b, i32 c, i32 d); // FUN @ 0x17da thunk

// The +0x60 timer/poll object (reloc-masked thiscall). StoreInputState mirrors
// the latest input-state value into its +0x2c field.
struct TimerObj {
    char m_pad0[0x2c];
    i32 m_inputMirror; // +0x2c
    void Stop();       // (this) reloc-masked
};

// The +0x68 sub-controller (gated on m_10; reloc-masked thiscall).
struct Ctrl68 {
    void Flush(); // (this) reloc-masked
    void Reset(); // (this) reloc-masked (FUN @ 0x15c3 thunk)
};

// The +0x54 input/state object's reset/flush entrypoints (reloc-masked).
struct InputState54 {
    void Method0(); // FUN @ 0x29b9 thunk
    void Method1(); // FUN @ 0x18e8 thunk
};

// One player-status slot in the PLAY state's 4-entry status array (m_curState->+0x520),
// each 0x64 bytes; the status id is at +0x20 (3 == "won/done").
struct PlayStatusSlot {
    char m_pad0[0x20];
    i32 m_status; // +0x20
    char m_pad24[0x64 - 0x24];
};
// A view of the live PLAY state (a CState subclass) exposing the +0x520 status
// array base.
struct CPlayStateView {
    char m_pad0[0x520];
    PlayStatusSlot* m_520; // +0x520
};

// The save-slot info record FillSaveInfo populates: a 0x20-byte snapshot block at
// +0x14, the level name at +0x75, and the two state ints at +0xf8/+0xfc.
struct SaveInfo {
    char m_pad0[0x14];
    char m_14[0x20]; // +0x14  snapshot block
    char m_pad34[0x75 - 0x34];
    char m_75[0x80]; // +0x75  level name buffer
    char m_padf5[0xf8 - 0xf5];
    i32 m_f8; // +0xf8
    i32 m_fc; // +0xfc
};

// The +0x58 manager FillSaveInfo forwards the record + source-state ptr to
// (reloc-masked thiscall).
struct SaveSink58 {
    void Store(SaveInfo* dst, char* src); // (this, dst, src) reloc-masked
};

// The engine's out-of-line block copy (FUN_00520340). Retail calls it here (not
// the inlined rep-movs memcpy intrinsic); modeled as a plain __cdecl free fn so
// the `push n; push src; push dst; call; add esp,0xc` shape falls out.
void EngineCopy(void* dst, void* src, i32 n); // FUN_00520340

// The modal dialog/screen handed to ExitModalUI. Run() is virtual slot 48
// (+0xc0); the leading slots are anchors so the call lands at [vtbl+0xc0] as a
// thiscall (MSVC 5.0 forbids the __thiscall keyword, so model it as a vtable).
class CModalDialog {
public:
    virtual void s00();
    virtual void s01();
    virtual void s02();
    virtual void s03();
    virtual void s04();
    virtual void s05();
    virtual void s06();
    virtual void s07();
    virtual void s08();
    virtual void s09();
    virtual void s0a();
    virtual void s0b();
    virtual void s0c();
    virtual void s0d();
    virtual void s0e();
    virtual void s0f();
    virtual void s10();
    virtual void s11();
    virtual void s12();
    virtual void s13();
    virtual void s14();
    virtual void s15();
    virtual void s16();
    virtual void s17();
    virtual void s18();
    virtual void s19();
    virtual void s1a();
    virtual void s1b();
    virtual void s1c();
    virtual void s1d();
    virtual void s1e();
    virtual void s1f();
    virtual void s20();
    virtual void s21();
    virtual void s22();
    virtual void s23();
    virtual void s24();
    virtual void s25();
    virtual void s26();
    virtual void s27();
    virtual void s28();
    virtual void s29();
    virtual void s2a();
    virtual void s2b();
    virtual void s2c();
    virtual void s2d();
    virtual void s2e();
    virtual void s2f();
    virtual i32 Run(); // slot 48 (+0xc0)
};

// The +0x2dc sub-object's teardown + the active object's own finalize (both
// reloc-masked thiscalls).
struct CActiveSub2dc {
    void Release(); // (this) reloc-masked
};
// The active game object returned by GetActiveObj (its +0x2dc sub-object gets a
// teardown when present, then the object itself is finalized; reloc-masked).
struct CActiveObj {
    char m_pad0[0x2dc];
    CActiveSub2dc* m_2dc; // +0x2dc
    void Finalize();      // (this) reloc-masked
};

extern "C" {
    extern i32 g_6455fc; // DAT_006455fc  (round-robin options cursor)
}

// -------------------------------------------------------------------------
// The packed-color global SetColorDepth writes + the RGB shift/mask globals it
// reads (the engine's per-bit-depth color-format conversion table). All reloc-
// masked DATA refs; only the load/store shapes are load-bearing. g_surfaceColorKey
// is the C++-mangled ?g_surfaceColorKey@@3HA (no extern "C"); the rest are DAT_
// C globals pinned by their DATA() RVA so the DIR32 reloc pairs.
extern "C" {
    DATA(0x00283ea0)
    extern i32 g_683ea0; // DAT_00683ea0  (red shift-up)
    DATA(0x00283ea4)
    extern i32 g_683ea4; // DAT_00683ea4  (green shift-up)
    DATA(0x00283eac)
    extern i32 g_683eac; // DAT_00683eac  (red shift-down)
    DATA(0x00283eb0)
    extern i32 g_683eb0; // DAT_00683eb0  (green shift-down)
    DATA(0x00283eb4)
    extern i32 g_683eb4; // DAT_00683eb4  (blue shift-down)
    // The world-mode reload globals LoadWorldMode resets (reloc-masked).
    DATA(0x002455b4)
    extern i32 g_6455b4; // DAT_006455b4  (alt-flag, reload kind 1/5)
}

// SetGruntColor reaches a keyed lookup table embedded at +0x10 within the object
// held in the world's +0x10 slot. Lookup(key, &out) resolves a color row; the row
// carries a column index (+0x64) into its value table (+0x14). The recolor sink
// passed in by the caller shares the same row layout (value table at +0x14,
// column range [+0x64..+0x68]). Each engine call is reloc-masked.
struct CColorLookup {
    void Lookup(i32 key, i32** out); // FUN_005b8008 (this, key, &out)
};
struct CColorRow {
    char m_pad0[0x14];
    i32* m_14; // +0x14  value table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  column index
    i32 m_68; // +0x68  column max
};
// The object held in CWorldZ +0x10; its +0x10 is the embedded CColorLookup.
struct CWorldLookupHolder {
    char m_pad0[0x10];
    CColorLookup m_10; // +0x10  (sub-object, NOT a pointer)
};
// A TU-local view of the world exposing its +0x10 lookup-holder pointer.
struct CWorldLookupView {
    char m_pad0[0x10];
    CWorldLookupHolder* m_10; // +0x10
};
// The recolor entrypoint (FUN_005532b0): takes the resolved cell pointer by value;
// callee cleans the arg (no `add esp,4` at the call site -> __stdcall). Reloc-masked.
void __stdcall RecolorCell(i32 cell);

// A TU-local view of the world exposing its +0x38 status code (the load-result
// code ReportWorldStatus maps to a message id). The code is UNSIGNED: the switch
// range checks emit `cmp;ja/jbe` (unsigned), not the signed `jg/jle`.
struct CWorldStatusView {
    char m_pad0[0x38];
    u32 m_38; // +0x38  status code
};

// LoadWorldMode's reloc-masked siblings (engine objects reached through the
// manager's member pointers; all are __thiscall, so each is modeled as a method on
// its object so `mov ecx,obj; call` falls out - the displacements reloc-mask).
//   m_34: a 0x94-byte engine surface object built by Build(), configured by
//         Apply(), torn down by Teardown() + the operator-delete wrapper.
//   m_54: the input/state object (0x30 bytes; an embedded CObList at +8 ctor'd
//         CObList(0xa); wired by InitInput(world->m_28, inputFlag); torn down by a
//         state-flush (+0) + the embedded CObList dtor (+8)).
extern "C" void* RezAlloc(u32 n); // operator new (reloc-masked, __cdecl)
extern "C" void RezFree(void* p); // _RezFree (operator delete wrapper, __cdecl)
struct CRezSurface94 {
    void Teardown();                // FUN_0053abc0 (this) reloc-masked
    void Build();                   // FUN_0053aa10 (this) reloc-masked
    i32 Apply(i32 a, i32 b, i32 c); // FUN_0053ad00 (this, *p, 1, 0) reloc-masked
};
struct CObListSub {
    void Init(i32 cap); // CObList ctor (this = obj+8, 0xa) reloc-masked
    void Dtor();        // ~CObList-family (this = obj+8) reloc-masked
};
// The input/state object held at CGruntzMgr +0x54. m_24 is its armed flag; the two
// parameterless thiscalls toggle its active state; Flush() is its +0 teardown
// method; InitInput wires it to the world's +0x28 sub-controller. All reloc-masked.
struct CInput54 {
    char m_pad0[0x24];
    i32 m_24;                                       // +0x24  armed flag
    void Flush();                                   // 0x1082-thunk (this) reloc-masked
    void Arm();                                     // FUN_0040bcf0 (this) reloc-masked
    void Disarm();                                  // FUN_0040bc80 (this) reloc-masked
    i32 InitInput(void* worldSub28, i32 inputFlag); // FUN_0040b5e0 (this, sub28, flag)
};

// The world's polymorphic mode-set vtable (slot 7 = +0x1c notify; slot 6 = +0x18
// SetVideoMode(hwnd, w, h, depth, flag)). MSVC5 forbids __thiscall on a fn-ptr,
// so model it as a typed polymorphic class with anchor slots.
class CWorldModeIface {
public:
    virtual void s00();
    virtual void s01();
    virtual void s02();
    virtual void s03();
    virtual void s04();
    virtual void s05();
    virtual i32 SetVideoMode(i32 hwnd, i32 w, i32 h, i32 depth, i32 flag); // slot 6 (+0x18)
    virtual void Notify();                                                 // slot 7 (+0x1c)
};
// The mode-reset callback registration reached during LoadWorldMode: a non-virtual
// thiscall on the world with a code-address callback (LAB_00403193) handed in. The
// callback is an external function whose pushed address reloc-masks.
extern "C" void ModeResetCallback(); // LAB_00403193
struct CWorldRegistrar {
    void RegisterCallback(void* cb); // UnknownVirtualMethod28 (0x155f50) (this, cb)
};
// The per-load world-object factory (CreateGameObjectByName, __cdecl, reloc-masked).
void CreateWorldObjects(void* world);

// The world view's +0x24 sub-object exposes a two-entry extent pair at +0x64/+0x68
// the mode reload stamps to 0xe (a default tile-region size).
struct CWorldModeView {
    char m_pad0[0x64];
    i32 m_64, m_68; // +0x64/+0x68
};

// ResetWorldState's MFC wait-cursor pair. Retail inlines the global ::Begin/
// EndWaitCursor (AfxGetApp()->...); modeled as reloc-masked free fns here (only
// the call shape is load-bearing - the inlined AfxGetModuleState/[+4]/thiscall
// expansion is part of the documented MFC-inline residual on this @early-stop fn).
void BeginWaitCursor(); // 0x1beafb
void EndWaitCursor();   // 0x1beb10

// The per-frame input/state object at CGruntzMgr +0x54 (reloc-masked thiscall).
struct InputStateObj {
    void StoreFlag(i32 v); // FUN_004385e0-family (this, v)
};

// One element of the 4-entry options array embedded at CGruntzMgr +0x150 (each
// 0x238 bytes). AdvanceOptionsCycle reads its arm flag (+0x14) + loaded flag
// (+0x20) and ticks the +0x38 sub-object (reloc-masked thiscall); BroadcastCmd
// forwards the 4-arg command into the slot itself.
struct OptionsTickSub {
    void Tick(); // (this) reloc-masked
    // SyncOptionsState drives the slot's +0x38 sub-object: LoadConfig pulls the
    // per-slot config from disk (mgr, slotIndex, configSelect) and the second
    // thiscall arms the matched slot's just-loaded config. Both reloc-masked.
    i32 LoadConfig(class CGruntzMgr* mgr, i32 slotIndex, i32 configSelect); // 0x18c0 thunk
    void Activate();                                                        // FUN_0042ade0
};
struct OptionsSlot {
    char m_pad0[0x4];
    CString m_name; // +0x04  per-slot name/config string (LoadOptionsSlotName target)
    char m_pad8[0x10 - 0x8];
    i32 m_10; // +0x10  per-slot config id
    i32 m_14; // +0x14  arm flag
    i32 m_18; // +0x18  slot key (FindOptionsSlot match)
    char m_pad1c[0x20 - 0x1c];
    i32 m_20; // +0x20  loaded flag
    char m_pad24[0x38 - 0x24];
    OptionsTickSub m_38;                     // +0x38
    i32 Command(i32 a, i32 b, i32 c, i32 d); // (this, a..d) reloc-masked
};

// The downstream command sinks BroadcastCmd fans the 4-arg command out to: the
// +0x68 grid, the live source object (via GetSaveSource), the +0x6c sub-mgr, the
// +0x70 polymorphic object (vtbl slot 1), and the +0x7c HUD. Each returns
// nonzero to keep broadcasting. All reloc-masked.
struct CmdSink {
    i32 Command(i32 a, i32 b, i32 c, i32 d); // (this, a..d) reloc-masked
};
// The +0x60 controller's 0-arg tick fired before the broadcast (reloc-masked).
struct CmdTimer60 {
    void Tick(); // (this) reloc-masked
};
// The +0x70 object dispatches the command through vtbl slot 1 (+0x04) as a
// thiscall; model it as a 2-slot polymorphic class.
class CmdSinkV {
public:
    virtual void s0();
    virtual i32 Command(i32 a, i32 b, i32 c, i32 d); // slot 1 (+0x04)
};

// The loaded world's height/influence grid (reached via m_world->m_24->m_5c). m_20
// is the value grid, m_24 the per-column base offset table.
struct CHeightGrid {
    char m_pad0[0x20];
    i32* m_20; // +0x20  value grid
    i32* m_24; // +0x24  per-column base table
};

// The active world view at m_world->m_24; its +0x5c holds the height grid.
struct CWorldViewZ {
    char m_pad0[0x5c];
    CHeightGrid* m_5c; // +0x5c
};

// The scroll/camera view at m_world->m_24 (a CWorldView): a tile rect at
// +0x10..+0x1c and the three scaled-extent output pairs at +0xc8..+0xdc that
// RecomputeViewScale fills; +0x5c is a CWorldEdges sub-object (origin fields at
// +0x40..+0x4c). The view's rescale notify is a reloc-masked thiscall.
struct CWorldEdges {
    char m_pad0[0x40];
    i32 m_40, m_44, m_48, m_4c; // +0x40..+0x4c
};
struct CScrollView {
    char m_pad0[0x10];
    i32 m_10, m_14, m_18, m_1c; // +0x10..+0x1c  tile rect
    char m_pad20[0x5c - 0x20];
    CWorldEdges* m_5c; // +0x5c
    char m_pad60[0xc8 - 0x60];
    i32 m_c8, m_cc; // +0xc8/+0xcc  scale-1 extents
    i32 m_d0, m_d4; // +0xd0/+0xd4  scale-2 extents
    i32 m_d8, m_dc; // +0xd8/+0xdc  scale-3 extents
    void Notify();  // FUN @ 0x160ee0 (this) reloc-masked
};

// The +0x70 notify object (reloc-masked 3-arg thiscall setter).
struct CNotify70 {
    void Set(i32 row, i32 col, i32 value); // (this, row, col, value) reloc-masked
};

// The engine's __cdecl CString-formatting helper (sprintf-style into a CString
// destination; reloc-masked - only the call shape is load-bearing).
extern "C" void Format(CString* dst, const char* fmt, ...);

// The engine's __cdecl struct-returning world-file-name builder (FUN_0003ad90):
// resolves the current world file from the game window handle into a CString
// (returned by value -> hidden return-slot arg). Reloc-masked; the (hwnd, flag)
// arg shape + the cdecl struct-return convention are what is load-bearing.
CString GetWorldFileFromWindow(HWND hwnd, i32 flag);

// The per-frame draw-clock globals PerFrameTick stamps each tick. g_wap32Now /
// g_wap32FrameDelta are the engine's just-refreshed clock (mangled C++ globals,
// stored into the game-side mirror g_645580/g_645584); g_6bf3c0/g_6bf3bc are the
// draw-clock pair (extern "C" -> the _g_* C symbols). All reloc-masked DATA refs.
extern "C" {
    extern u32 g_645580; // game-side now mirror (DAT_00645580)
    extern u32 g_645584; // game-side delta mirror (DAT_00645584)
    extern u32 g_645588; // game-side abs clock (DAT_00645588)
    extern u32 g_6bf3c0; // draw-clock (timeGetTime stamp)
    extern u32 g_6bf3bc; // draw-clock delta (cleared)
    // The clock/scroll-state globals ResetClockGlobals zeroes (reloc-masked).
    extern u32 g_645600; // DAT_00645600
    extern u32 g_6455b0; // DAT_006455b0
    extern u32 g_6455a4; // DAT_006455a4
    extern u32 g_6455a8; // DAT_006455a8
    extern u32 g_6455ac; // DAT_006455ac
    extern u32 g_6455f8; // DAT_006455f8
    extern u32 g_6455f4; // DAT_006455f4
}

// The two engine input/state singletons TickStateMgrs drives once per call
// (DAT_00645570/DAT_00645578; reloc-masked DATA refs). g_645570 is the
// DirectInputMgr2 (its PollAll @0x533080 is the per-frame device poll); g_645578
// is a second mgr (Flush @0x4385e0). Each call is a single reloc-masked
// __thiscall, so only the one-method shape on a tiny helper is load-bearing.
struct DirectInputMgr2 {
    i32 PollAll(); // FUN_00533080
    i32 Flush();   // FUN_00533110 (a second per-frame entrypoint)
};
struct StateMgrBZ {
    void Flush(); // FUN_004385e0
};
extern "C" {
    extern DirectInputMgr2* g_645570; // DAT_00645570
    extern StateMgrBZ* g_645578;      // DAT_00645578
}

// The embedded options object's ctor/dtor are out-of-line NAFXCW-style helpers
// (FUN_0051f5a0 / FUN_0051f640); only the call (reloc-masked) + the 0x238 size
// matter. Empty bodies suffice to give the member its destructible EH state.
CGruntzMgrOptions::CGruntzMgrOptions() {}
CGruntzMgrOptions::~CGruntzMgrOptions() {}

// -------------------------------------------------------------------------
// The world's +0x24 view exposes a layer array (+0x38 base, +0x3c count) plus a
// distinguished sub-layer (+0x5c); each layer carries a flag word at +0x8 whose
// bit 1 (0x2) is a visibility toggle the level-cycle / debug methods flip.
struct WorldLayer {
    char m_pad0[0x8];
    i32 m_8; // +0x08  flag word (bit 0 = locked, bit 1 = visible)
};
struct WorldLayerView {
    char m_pad0[0x38];
    WorldLayer** m_38; // +0x38  layer array
    i32 m_3c;          // +0x3c  layer count
    char m_pad40[0x5c - 0x40];
    WorldLayer* m_5c; // +0x5c  distinguished sub-layer
};

// The live state's +0x1c "current level index" the next/prev cycle reads/writes
// (a CState field inside the +0x1c..+0x24 gap; the offset is load-bearing, so a
// small TU-local view avoids touching the shared CState header).
struct CLevelState {
    char m_pad0[0x1c];
    i32 m_levelIndex; // +0x1c
};

// The +0x5c chat/message-log object: AppendChatMessage routes one message line
// (with type 0 / channel 0x11) into its insert slot (FUN_00421c60, reloc-masked).
struct CChatLog {
    i32 Insert(char* msg, i32 type, i32 channel); // (this, msg, type, channel)
};

// The shared scratch buffer the toggle-message formatter renders "<item> is
// ON/OFF" into before logging it (reloc-masked DATA ref). The format helper is
// the statically-linked CRT sprintf (FUN_0051f890; reloc-masked call).

// -------------------------------------------------------------------------
// CGruntzMgr::GoToNextLevel (0x08d850; ret). Only when the live state is PLAY
// (id 3): clears the world-file name, computes the next level index
// (m_curState->m_levelIndex + 1, wrapping past 0x28 back to 1), and - unless that
// index lands in the reserved band 0x21..0x24 - notifies the state (FrameSlot28
// with the live id) and routes the level switch through Vslot1e(next, 1). On a
// successful switch it re-notifies via Vslot09(live id) and returns 1; any
// reserved-index / failed-switch surfaces a (0x8007, 0x436) error and returns 0.
RVA(0x0008d850, 0x83)
i32 CGruntzMgr::GoToNextLevel() {
    if (m_curState->Update() != 3) {
        return 0;
    }
    m_strWorldFile.Empty();
    CState* st = m_curState;
    i32 next = ((CLevelState*)st)->m_levelIndex + 1;
    if (next > 0x28) {
        next = 1;
    }
    if (next <= 0x20 || next >= 0x25) {
        st->FrameSlot28(st->Update());
        if (st->Vslot1e(next, 1)) {
            st->Vslot09(st->Update());
            return 1;
        }
    }
    ReportError(0x8007, 0x436);
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::GoToPrevLevel (0x08d910; ret). The descending twin of GoToNextLevel:
// the index is m_curState->m_levelIndex - 1, wrapping at/below 0 back to 0x28, and
// the failure error code is (0x8007, 0x437).
RVA(0x0008d910, 0x82)
i32 CGruntzMgr::GoToPrevLevel() {
    if (m_curState->Update() != 3) {
        return 0;
    }
    m_strWorldFile.Empty();
    CState* st = m_curState;
    i32 prev = ((CLevelState*)st)->m_levelIndex - 1;
    if (prev <= 0) {
        prev = 0x28;
    }
    if (prev <= 0x20 || prev >= 0x25) {
        st->FrameSlot28(st->Update());
        if (st->Vslot1e(prev, 1)) {
            st->Vslot09(st->Update());
            return 1;
        }
    }
    ReportError(0x8007, 0x437);
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ReportError  (__thiscall; `ret 8`)
// Forwards the (id, detail) error to the owning CGameApp held in the base
// CGameMgr::m_8 pointer, via its vtable slot +0x1c (CGameApp::ReportError).
// No-op when there is no app bound yet.
RVA(0x0008dc60, 0x19)
void CGruntzMgr::ReportError(WPARAM wParam, LPARAM lParam) {
    CGameApp* pApp = (CGameApp*)m_8;
    if (pApp) {
        pApp->ReportError(wParam, lParam);
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::RestoreVideoMode  (__thiscall; `ret 4`; retail FUN_0048ddd0)
// Re-asserts the standard 640x480 display mode. If the live mode (m_modeW x m_modeH)
// is already 640x480 it is a no-op success - and when `save` is set it also
// records the mode into the saved/last-good pair (m_savedModeW/m_savedModeH). Otherwise it
// drives SetVideoMode(640, 480, save); on failure it surfaces a (0x8008, 0x438)
// error and returns 0.
// @early-stop
// constant-CSE/copy-prop wall: retail uses immediate cmp operands + re-loads the
// fields fresh in the save block (no constant propagation); MSVC 5.0 here either
// hoists 0x280/0x1e0 into entry registers (direct-member compare) or copy-props
// the known value into the m_savedModeW/m_savedModeH store (local-copy compare). Logic is exact;
// see docs/patterns/constant-cse-immediate-vs-hoist.md.
RVA(0x0008ddd0, 0x7e)
i32 CGruntzMgr::RestoreVideoMode(i32 save) {
    i32 w = m_modeW;
    i32 h = m_modeH;
    if (w == 0x280 && h == 0x1e0) {
        if (save) {
            m_savedModeW = w;
            m_savedModeH = h;
        }
        return 1;
    }
    if (SetVideoMode(0x280, 0x1e0, save)) {
        return 1;
    }
    ReportError(0x8008, 0x438);
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::AppendChatMessage (0x08f9c0; ret 4). Routes one message line into
// the +0x5c chat/message-log object (insert with type 0 / channel 0x11),
// returning its result; 0 when no log is bound.
RVA(0x0008f9c0, 0x1d)
i32 CGruntzMgr::AppendChatMessage(char* msg) {
    CChatLog* log = (CChatLog*)m_5c;
    if (log == 0) {
        return 0;
    }
    return log->Insert(msg, 0, 0x11);
}

// -------------------------------------------------------------------------
// CGruntzMgr::ShowToggleMessage (0x08f9f0; ret 8). Formats "<item> is ON/OFF"
// (selected by `on`) into the shared scratch buffer and appends it to the chat
// log, returning AppendChatMessage's result.
// Per-branch sprintf (NOT a ternary on the fmt arg) so each branch pushes
// itemName + its format literal and the optimizer cross-jumps only the shared
// `push g_msgScratch; call sprintf` tail - retail's exact block layout.
RVA(0x0008f9f0, 0x3e)
i32 CGruntzMgr::ShowToggleMessage(char* itemName, i32 on) {
    if (on) {
        sprintf(g_msgScratch, "%s is ON", itemName);
    } else {
        sprintf(g_msgScratch, "%s is OFF", itemName);
    }
    return AppendChatMessage(g_msgScratch);
}

// -------------------------------------------------------------------------
// CGruntzMgr::IsInPlayState  (__thiscall; retail FUN_0048fa40; ret). A bool-
// normalized predicate: 0 when no live state, else CheckPlayState() != 0 (true
// for the PLAY (3) / paused (0x11) states). The CheckPlayState() result lands in
// eax and the neg/sbb/neg idiom normalizes it to 0/1.
RVA(0x0008fa40, 0x16)
i32 CGruntzMgr::IsInPlayState() {
    if (m_curState == 0) {
        return 0;
    }
    return CheckPlayState() != 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::GetGruntzDriveLetter  (__thiscall)
// Returns the CD-ROM drive letter holding the Gruntz disc, memoised in the
// pair (m_driveLetter = letter, m_driveLetterProbed = probed-flag): once probed, return the cached
// letter; otherwise call Utils::WinAPI::GetGruntzDriveLetter(), cache + set the
// flag. (The result is discarded by the engine on the first/uncached path - the
// store IS the return, the cached path returns the byte.)
RVA(0x0008fa70, 0x2c)
char CGruntzMgr::GetGruntzDriveLetter() {
    if (m_driveLetterProbed) {
        return m_driveLetter;
    }
    m_driveLetter = Utils::WinAPI::GetGruntzDriveLetter();
    m_driveLetterProbed = 1;
    return m_driveLetter;
}

// -------------------------------------------------------------------------
// CGruntzMgr::CheckPlayState  (__thiscall; retail FUN_0048ec50)
// A small game-state predicate over the active state's Update() id: 0 if there
// is no state, 1 when it reports PLAY (3), else (id == 0x11 ? 1 : 0) - i.e. true
// for the in-game PLAY state or the paused/hold state. AdvanceFrame uses it to
// decide whether the sound bank should keep running. (Update() is re-evaluated
// on the second compare, matching the retail two-call codegen.)
RVA(0x0008ec50, 0x33)
i32 CGruntzMgr::CheckPlayState() {
    if (m_curState == 0) {
        return 0;
    }
    if (m_curState->Update() == 3) {
        return 1;
    }
    return m_curState->Update() == 0x11;
}

// -------------------------------------------------------------------------
// CGruntzMgr::InitializeLobbyConnectionSettings  (__thiscall)
// One-shot (guarded by m_lobbyProbed) acquisition of the DirectPlay lobby connection
// settings the game was launched with: create an IDirectPlayLobby (m_lobby), then
// pull the launch-time connection-settings blob into a freshly allocated buffer
// (m_connSettings) using the classic size-probe / fill-buffer two-call idiom. Every COM /
// DPLAYX failure routes a CNetMgr::ReportError diagnostic (with this TU's file +
// the call-site line) through the game window. m_lobbyResult records success (1) / failure
// (0); the result also lands in eax for the call site.
RVA(0x0008eca0, 0x164)
i32 CGruntzMgr::InitializeLobbyConnectionSettings() {
    if (m_lobbyProbed) {
        return m_lobbyResult;
    }

    m_lobbyProbed = 1;
    m_lobbyResult = 0;

    if (m_lobby) {
        m_lobby->Release();
        m_lobby = 0;
    }

    i32 hr = DirectPlayLobbyCreate(0, &m_lobby, 0, 0, 0);
    if (hr) {
        CNetMgr::ReportError(
            "C:\\Proj\\Gruntz\\GruntzMgr.cpp",
            0x120d,
            hr,
            ((CGameWnd*)m_4)->m_hwnd
        );
        return 0;
    }
    if (!m_lobby) {
        return 0;
    }

    if (m_connSettings) {
        FreeConnectionSettings(m_connSettings);
        m_connSettings = 0;
    }

    u32 dwSize = 0;
    hr = m_lobby->GetConnectionSettings(0, 0, &dwSize);
    if (hr != 0 && hr != (i32)0x8877001e) { // !DPERR_BUFFERTOOSMALL
        CNetMgr::ReportError(
            "C:\\Proj\\Gruntz\\GruntzMgr.cpp",
            0x1221,
            hr,
            ((CGameWnd*)m_4)->m_hwnd
        );
        m_lobby->Release();
        m_lobby = 0;
        return 0;
    }

    m_connSettings = operator new(dwSize);
    if (!m_connSettings) {
        m_lobby->Release();
        m_lobby = 0;
        return 0;
    }

    hr = m_lobby->GetConnectionSettings(0, m_connSettings, &dwSize);
    if (hr) {
        CNetMgr::ReportError(
            "C:\\Proj\\Gruntz\\GruntzMgr.cpp",
            0x1232,
            hr,
            ((CGameWnd*)m_4)->m_hwnd
        );
        m_lobby->Release();
        m_lobby = 0;
        return 0;
    }

    m_lobbyResult = 1;
    return m_lobbyResult;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ToggleObjectLayer (0x08efe0; ret). Debug visibility toggle for the
// world view's (m_world->m_24) "current" object layer: only when the manager is
// active (base slot-3 gate) and a world+view are loaded. The layer index is
// count-1, biased down one more when the count is exactly 4; once bounds-checked
// and unlocked (bit 0 clear) it flips the layer's visible bit (m_8 ^= 2) and
// returns 1, else 0.
// Errors share one trailing `return 0` (success path nested deepest) so every
// guard `je`s the single fail tail, matching retail's block layout instead of a
// per-guard epilogue; see docs/patterns/nested-if-success-deepest-error-tail.md.
// @early-stop
// 96.7% constant-fold tiebreak: logic byte-exact (guard chain + index + toggle).
// Residual is MSVC folding the index's true-arm `count-1` to `mov eax,3` (with a
// je/jne polarity flip) where retail kept `cmp eax,4; jne; dec eax`; the literal
// `if(idx==4)idx--` form scores worse (88%, view in edx). Documented constant-CSE
// wall, see docs/patterns/constant-cse-immediate-vs-hoist.md (regalloc family).
RVA(0x0008efe0, 0x54)
i32 CGruntzMgr::ToggleObjectLayer() {
    if (Wap32GameMgrVfunc3() && m_world) {
        WorldLayerView* view = (WorldLayerView*)m_world->m_24;
        if (view) {
            i32 count = view->m_3c;
            // (count==4 ? count-1 : count) - 1: best-scoring spelling (96.7%); it
            // recovers retail's ecx/edx view/count regs + the `cmp;jne;dec` shape.
            // The lone residual is MSVC folding the true-arm `count-1` to `mov 3`
            // (+ the je/jne polarity) where retail kept `dec eax`; the literal
            // `if(idx==4)idx--;idx--;` form regresses to 88% (view in edx). The
            // fold is the constant-CSE tiebreak, not source-steerable.
            i32 idx = (count == 4 ? count - 1 : count) - 1;
            WorldLayer* layer = (idx < 0 || idx >= count) ? 0 : view->m_38[idx];
            if (layer && !(layer->m_8 & 1)) {
                layer->m_8 ^= 2;
                return 1;
            }
        }
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ToggleHeightLayer (0x08f060; ret). Visibility toggle for the world
// view's distinguished sub-layer (m_world->m_24->m_5c) - flipped unconditionally
// (no lock check). Active-gated + world/view guarded like ToggleObjectLayer.
RVA(0x0008f060, 0x35)
i32 CGruntzMgr::ToggleHeightLayer() {
    if (Wap32GameMgrVfunc3() && m_world) {
        WorldLayerView* view = (WorldLayerView*)m_world->m_24;
        if (view) {
            WorldLayer* layer = view->m_5c;
            if (layer) {
                layer->m_8 ^= 2;
                return 1;
            }
        }
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ToggleBaseLayer (0x08f0b0; ret). Visibility toggle for the world
// view's first layer (m_world->m_24->m_38[0], present only when the count is
// positive); unlocked-checked (bit 0) then m_8 ^= 2. Active-gated + guarded.
RVA(0x0008f0b0, 0x46)
i32 CGruntzMgr::ToggleBaseLayer() {
    if (Wap32GameMgrVfunc3() && m_world) {
        WorldLayerView* view = (WorldLayerView*)m_world->m_24;
        if (view) {
            WorldLayer* layer = (view->m_3c > 0) ? view->m_38[0] : 0;
            if (layer && !(layer->m_8 & 1)) {
                layer->m_8 ^= 2;
                return 1;
            }
        }
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::PollUnlessIdle (0x08f2f0; ret). Unless the live state is the idle
// state (id 5), runs the play-state poll (CheckPlayState, result discarded);
// always returns 0.
RVA(0x0008f2f0, 0x1b)
i32 CGruntzMgr::PollUnlessIdle() {
    if (m_curState->Update() != 5) {
        CheckPlayState();
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::CaptureWorldFile (0x08f340; /GX EH). Engine helper FUN_0003ad90
// (__cdecl, struct-return) builds the current world-file CString from the game
// window handle; modeled as a value-returning free function so the hidden
// return-slot + (hwnd, 0) args fall out (the call rel32 reloc-masks).
RVA(0x0008f340, 0xf6)
i32 CGruntzMgr::CaptureWorldFile() {
    i32 st = m_curState->Update();
    if (st != 5 && st != 2 && st != 3 && st != 7) {
        return 0;
    }
    CString name = GetWorldFileFromWindow(((CGameWnd*)m_4)->m_hwnd, 0);
    if (name.GetLength() == 0) {
        return 0;
    }
    m_strWorldFile = name;
    m_12c = 0;
    m_128 = 0;
    g_pPostMessageA((i32)((CGameWnd*)m_4)->m_hwnd, 0x111, 0x8005, 0);
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::PerFrameTick  (__thiscall; `ret`)
// The per-frame draw-clock tick. If the active state's Update() reports the
// "paused/hold" id (0x11) the tick is skipped; otherwise it refreshes the engine
// clock (CGameMgr::UnknownMethodInitializeTimeGlobal), optionally re-stamps the
// draw clock (g_6bf3c0 = timeGetTime(), g_6bf3bc = 0) when the draw gate m_world is
// set, and finally mirrors the freshly-refreshed engine clock into the game-side
// pair (g_645580/g_645584).
RVA(0x0008f620, 0x51)
void CGruntzMgr::PerFrameTick() {
    if (m_curState && m_curState->Update() == 0x11) {
        return;
    }

    UnknownMethodInitializeTimeGlobal();

    if (m_world) {
        g_6bf3c0 = timeGetTime();
        g_6bf3bc = 0;
    }

    g_645580 = g_wap32Now;
    g_645584 = g_wap32FrameDelta;
}

// -------------------------------------------------------------------------
// CGruntzMgr::AdvanceFrame  (__thiscall; `ret 8`)
// Retail Ghidra labels this ?VirtualUnknownMethod06@CGruntzMgr@@QAEXXZ (a
// void(void) mistype), but the body reads its first arg ([esp+8]) and `ret 8`s
// two args, so the true shape is void(int,int). The natural MS mangle of that
// signature (?AdvanceFrame@CGruntzMgr@@QAEXHH@Z) is what the base obj emits and
// the delinker names the target through (labels.py authority check), so no
// SYMBOL() override is needed - the match is by SHAPE.
// The per-frame advance gate. Bails when the owning window's "active" virtual
// (slot 3) reports 0. On the draw path (doDraw != 0) it runs the draw tick, then
// - unless mid-transition (m_c set) and only while a level is loaded (m_14) -
// either keeps the sound bank running (CheckPlayState() => PLAY/paused) or stops
// it; on the non-draw path it stops the sound bank once its inner object reports
// idle.
RVA(0x0008f6a0, 0x7d)
void CGruntzMgr::AdvanceFrame(i32 doDraw, i32 /*unused*/) {
    if (Wap32GameMgrVfunc3() == 0) {
        return;
    }

    if (doDraw) {
        PerFrameTick();
        if (m_c != 0) {
            return;
        }
        if (m_14 == 0) {
            return;
        }
        if (CheckPlayState() == 0 && (m_curState == 0 || m_curState->Update() != 8)) {
            return;
        }
        m_sound->StopBank(1);
        return;
    }

    if (m_14 == 0) {
        return;
    }
    if ((m_sound->m_1c ? m_sound->m_1c->IsBusy() : 0) == 0) {
        return;
    }
    m_sound->StopAll();
}

// -------------------------------------------------------------------------
// CGruntzMgr::BuildMoviePath  (__thiscall; returns CString by value; /GX EH)
// Resolves the on-disk path of a numbered intro/cutscene .vob: maps the movie
// id to its file name, then probes first the working directory ("<cwd>\<name>")
// and, failing that, the Movies\ folder on the Gruntz CD ("<letter>:\Movies\
// <name>"), returning the first that exists (empty CString if neither, or for an
// unknown id). The two CString temps + the szPath buffer give the /GX frame.
RVA(0x0008ff30, 0x1ca)
CString CGruntzMgr::BuildMoviePath(i32 movie) {
    CString name;

    switch (movie) {
        case -1:
            name = "Logo.vob";
            break;
        case 0:
            name = "Gruntz0.vob";
            break;
        case 1:
            name = "Gruntz1.vob";
            break;
        case 2:
            name = "Gruntz2.vob";
            break;
        case 3:
            name = "Gruntz3.vob";
            break;
        case 4:
            name = "Gruntz4.vob";
            break;
        case 5:
            name = "Gruntz5.vob";
            break;
        case 6:
            name = "Gruntz6.vob";
            break;
        case 7:
            name = "Gruntz7.vob";
            break;
        case 8:
            name = "Gruntz8.vob";
            break;
    }

    if (name.GetLength() == 0) {
        return name; // unknown id
    }

    CString path;
    char szDir[256];

    // First try the working directory ("<cwd>\<name>").
    if (GetCurrentDirectoryA(0xff, szDir)) {
        Format(&path, "%s\\%s", szDir, (const char*)name);
        if (!Utils::WinAPI::FileExists((char*)(const char*)path)) {
            path.Empty();
        }
    }

    // Fall back to the Movies\ folder on the Gruntz CD.
    if (path.GetLength() == 0) {
        Format(&path, "%c:\\Movies\\%s", GetGruntzDriveLetter(), (const char*)name);
        if (path.GetLength() == 0) {
            return path;
        }
    }

    if (!Utils::WinAPI::FileExists((char*)(const char*)path)) {
        path.Empty();
    }

    return path;
}

// -------------------------------------------------------------------------
// Per-state notification forwarders (0x08d9d0..0x08dbe0). Each dispatches its
// (a,b[,c]) args into the live state's vtable slot 11..20 (+0x2c..+0x50),
// returning the slot's result or 0 when there is no live state. The args are
// re-loaded fresh from the moving stack per push (the natural forwarder codegen).
RVA(0x0008d9d0, 0x1e)
i32 CGruntzMgr::NotifyState0b(i32 a, i32 b) {
    if (m_curState) {
        return m_curState->Vslot0b(a, b);
    }
    return 0;
}
RVA(0x0008da00, 0x1e)
i32 CGruntzMgr::NotifyState0c(i32 a, i32 b) {
    if (m_curState) {
        return m_curState->Vslot0c(a, b);
    }
    return 0;
}
RVA(0x0008da30, 0x1e)
i32 CGruntzMgr::NotifyState0d(i32 a, i32 b) {
    if (m_curState) {
        return m_curState->Vslot0d(a, b);
    }
    return 0;
}
RVA(0x0008da60, 0x23)
i32 CGruntzMgr::NotifyState0e(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->Vslot0e(a, b, c);
    }
    return 0;
}
RVA(0x0008daa0, 0x23)
i32 CGruntzMgr::NotifyState0f(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->Vslot0f(a, b, c);
    }
    return 0;
}
RVA(0x0008dae0, 0x23)
i32 CGruntzMgr::NotifyState10(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->Vslot10(a, b, c);
    }
    return 0;
}
RVA(0x0008db20, 0x23)
i32 CGruntzMgr::NotifyState11(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->Vslot11(a, b, c);
    }
    return 0;
}
RVA(0x0008db60, 0x23)
i32 CGruntzMgr::NotifyState12(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->Vslot12(a, b, c);
    }
    return 0;
}
RVA(0x0008dba0, 0x23)
i32 CGruntzMgr::NotifyState13(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->Vslot13(a, b, c);
    }
    return 0;
}
RVA(0x0008dbe0, 0x23)
i32 CGruntzMgr::NotifyState14(i32 a, i32 b, i32 c) {
    if (m_curState) {
        return m_curState->Vslot14(a, b, c);
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::CheckSavedMode (0x08de70; ret). If the live mode (m_modeW x m_modeH)
// already equals the saved/last-good mode (m_savedModeW x m_savedModeH) it succeeds; otherwise
// it drives SetVideoMode(saved-w, saved-h, save=1), falling back to
// RestoreVideoMode(1), and on total failure surfaces a (0x8008, 0x45e) error.
RVA(0x0008de70, 0x61)
i32 CGruntzMgr::CheckSavedMode() {
    // All success paths short-circuit to one trailing `return 1` (retail tail-
    // merges the mov eax,1; pop; ret epilogue).
    if ((m_modeW == m_savedModeW && m_modeH == m_savedModeH)
        || SetVideoMode(m_savedModeW, m_savedModeH, 1) || RestoreVideoMode(1)) {
        return 1;
    }
    ReportError(0x8008, 0x45e);
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ResetClockGlobals (0x08f4f0). Zeroes the seven game-clock / scroll
// state globals (reloc-masked DATA refs).
RVA(0x0008f4f0, 0x26)
void CGruntzMgr::ResetClockGlobals() {
    g_645600 = 0;
    g_6455b0 = 0;
    g_6455a4 = 0;
    g_6455a8 = 0;
    g_6455ac = 0;
    g_6455f8 = 0;
    g_6455f4 = 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::SetGameClock (0x08f7b0; ret 0xc). Mirrors the three clock args into
// the game-side now/delta/abs globals plus the draw-clock pair (reloc-masked).
RVA(0x0008f7b0, 0x2b)
void CGruntzMgr::SetGameClock(i32 now, i32 delta, i32 abs) {
    g_645580 = now;
    g_645584 = delta;
    g_645588 = abs;
    g_6bf3c0 = now;
    g_6bf3bc = delta;
}

// -------------------------------------------------------------------------
// CGruntzMgr::RunFromState (0x090200; ret, no arg cleanup). Thin forwarder:
// ChangeState_8fab0(1).
RVA(0x00090200, 0x8)
i32 CGruntzMgr::RunFromState() {
    return ChangeState_8fab0(1);
}

// -------------------------------------------------------------------------
// CGruntzMgr::TopState (0x090980). Returns the last pushed state (or 0 when the
// stack is empty).
RVA(0x00090980, 0x18)
CState* CGruntzMgr::TopState() {
    CStateStackZ* st = (CStateStackZ*)&m_stateStack;
    if (st->m_nSize <= 0) {
        return 0;
    }
    return st->m_pData[st->m_nSize - 1];
}

// -------------------------------------------------------------------------
// CGruntzMgr::PushState (0x0909b0; ret 4). Appends s to the state stack via
// SetAtGrow(GetSize(), s); ignores a null push.
RVA(0x000909b0, 0x1b)
void CGruntzMgr::PushState(CState* s) {
    if (!s) {
        return;
    }
    CStateStackZ* st = (CStateStackZ*)&m_stateStack;
    st->SetAtGrow(st->m_nSize, s);
}

// -------------------------------------------------------------------------
// CGruntzMgr::PopTopIfMatches (0x0909e0; ret 4). Saves the last stack slot,
// RemoveAt()s it, and reports whether the removed top was s. The count/data are
// read fresh off `this` (+0xe0/+0xdc); only the helper call forms &m_stateStack.
RVA(0x000909e0, 0x46)
i32 CGruntzMgr::PopTopIfMatches(CState* s) {
    if (!s) {
        return 0;
    }
    i32 n = *(i32*)((char*)&m_stateStack + 8);
    if (n <= 0) {
        return 0;
    }
    CState* top = (*(CState***)((char*)&m_stateStack + 4))[n - 1];
    ((CStateStackZ*)&m_stateStack)->RemoveAt(n - 1, 1);
    return top == s;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ClearStateStack (0x090a50; ret). Deletes every pushed state then
// clears the array via SetSize(0, -1). The loop reads count/data off `this`
// (+0xe0/+0xdc) so the array base is not hoisted into a register.
RVA(0x00090a50, 0x40)
void CGruntzMgr::ClearStateStack() {
    for (i32 i = 0; i < *(i32*)((char*)&m_stateStack + 8); i++) {
        CState* s = (*(CState***)((char*)&m_stateStack + 4))[i];
        if (s) {
            delete s;
        }
    }
    ((CStateStackZ*)&m_stateStack)->SetSize(0, -1);
}

// -------------------------------------------------------------------------
// CGruntzMgr::CheckMovieFileExists (0x090aa0; cdecl ret). Probes whether the
// resolved movie path (m_strMoviePath) exists on disk.
RVA(0x00090aa0, 0x10)
i32 CGruntzMgr::CheckMovieFileExists() {
    return Utils::WinAPI::FileExists((char*)(const char*)m_strMoviePath);
}

// -------------------------------------------------------------------------
// CGruntzMgr::IsMoviePathValid (0x0901d0; cdecl ret). The bool-normalized twin of
// CheckMovieFileExists: FileExists(m_strMoviePath) run through the neg/sbb/neg
// 0/1 canonicalizer (the result is consumed as a boolean here).
RVA(0x000901d0, 0x16)
i32 CGruntzMgr::IsMoviePathValid() {
    return Utils::WinAPI::FileExists((char*)(const char*)m_strMoviePath) != 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ReportWorldStatus (0x090ac0; ret 4). Surfaces the loaded world's
// status code (m_world->m_38) as a (msgId, statusCode) error. Bails to the
// generic (0x800a) error first when there is no world or no status, then maps the
// known status codes to their message ids via a switch; the near-consecutive
// 0x80ea..0x80ed band becomes a dense jump table, the rest a cmp/je tree, and the
// catch-all status reports 0x8011. Cases that report a FIXED code push the literal
// status (matching retail's `push <imm>`); the open-ended ones push the variable.
// The status code is u32 so the range checks emit unsigned `cmp;ja` (not `jg`);
// see docs/patterns/switch-key-unsigned-ja-vs-jg.md.
// @early-stop
// CODE byte-exact: the cmp/je tree, the dense 0x80ea..0x80ed jump table, and every
// ReportError push match retail to the byte. The residual ~3% is the jump-table
// DATA scored mismatched against the reloc-masked `jmp [eax*4+tbl]` base - the
// documented jumptable-data-overlap scoring artifact, NOT a code difference.
RVA(0x00090ac0, 0x1bb)
void CGruntzMgr::ReportWorldStatus(i32 a) {
    if (m_world == 0) {
        ReportError(0x800a, a);
    }
    u32 status = ((CWorldStatusView*)m_world)->m_38;
    if (status == 0) {
        ReportError(0x800a, a);
    }
    switch (status) {
        case 0x3f0:
            ReportError(0x8015, 0x3f0);
            return;
        case 0x3f1:
            ReportError(0x8013, 0x3f1);
            return;
        case 0x3f2:
            ReportError(0x8012, 0x3f2);
            return;
        case 0x7d1:
            ReportError(0x8019, 0x7d1);
            return;
        case 0x7d2:
            ReportError(0x8018, 0x7d2);
            return;
        case 0x7d3:
            ReportError(0x8017, 0x7d3);
            return;
        case 0xbb9:
            ReportError(0x8014, 0xbb9);
            return;
        case 0xbba:
            ReportError(0x8016, status);
            return;
        case 0x80e9:
            ReportError(0x801e, 0x80e9);
            return;
        case 0x80ea:
            ReportError(0x801a, status);
            return;
        case 0x80eb:
            ReportError(0x801b, status);
            return;
        case 0x80ec:
            ReportError(0x801c, status);
            return;
        case 0x80ed:
            ReportError(0x801d, status);
            return;
        default:
            ReportError(0x8011, status);
            return;
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::LoadMonologoSprite (0x090d10). PLAY-state only (m_curState->Update()
// == 3): look "GAME_MONOLITH" up in the world config map (m_world->m_10), then
// find-or-create the "MONOLITH" logo sprite in the world view (m_world->m_24). An
// existing sprite TOGGLES its visible bit (m_8 & 2) + the g_6455e8 shown-flag; a
// fresh sprite gets its cell grid checkerboard-seeded with the config index / -1 and
// the flag set to 1. No destructible local -> no /GX frame (the sprite-grid loop).
// (placeholder fields; only offsets + code bytes load-bearing.)
struct CMonoEntry {
    char m_pad0[0x10];
    i32 m_10; // +0x10  geometry A
    i32 m_14; // +0x14  geometry B
};
struct CMonoConfigRec {
    char m_pad0[0x14];
    CMonoEntry** m_14; // +0x14  entry table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  selected index
};
struct CMonoConfigMap {
    i32 Lookup(const char* key, void** out); // 0x1b8008  CMapStringToOb::Lookup
};
struct CMonoConfigHolder {
    char m_pad0[0x10];
    CMonoConfigMap m_10; // +0x10  embedded string map
};
struct CMonoSprite;
struct CMonoView {
    CMonoSprite* FindSprite(const char* name); // 0x15dde0
    CMonoSprite*
    CreateSprite(i32 w, i32 h, i32 geoA, i32 geoB, i32 ox, i32 oy, const char* name); // 0x15d9a0
};
struct CMonoCellArray {
    void SetAtGrow(i32 i, void* elem); // 0x1b5822  CObArray::SetAtGrow
};
struct CMonoSprite {
    char m_pad0[0x8];
    i32 m_8; // +0x08  flag bits (bit1 = visible)
    char m_pad0c[0x20 - 0xc];
    i32* m_20; // +0x20  cell grid
    i32* m_24; // +0x24  per-row base table
    i32 m_28;  // +0x28  cols
    i32 m_2c;  // +0x2c  rows
    char m_pad30[0x80 - 0x30];
    i32 m_80; // +0x80
    char m_pad84[0x9c - 0x84];
    CMonoCellArray m_9c; // +0x9c  SetAtGrow target
};
struct CMonoWorld {
    char m_pad0[0x10];
    CMonoConfigHolder* m_10; // +0x10  config holder
    char m_pad14[0x24 - 0x14];
    CMonoView* m_24; // +0x24  world view / sprite factory
};

RVA(0x00090d10, 0x18e)
i32 CGruntzMgr::LoadMonologoSprite() {
    if (m_curState == 0) {
        return 0;
    }
    if (m_curState->Update() != 3) {
        return 0;
    }
    if (m_world == 0) {
        return 0;
    }
    void* out = 0;
    ((CMonoWorld*)m_world)->m_10->m_10.Lookup("GAME_MONOLITH", &out);
    CMonoConfigRec* rec = (CMonoConfigRec*)out;
    if (rec == 0) {
        return 0;
    }
    i32 savedIdx = rec->m_64;
    CMonoEntry* e = rec->m_14[savedIdx];
    if (e == 0) {
        return 0;
    }
    i32 geoA = e->m_10;
    i32 geoB = e->m_14;
    CMonoSprite* found = ((CMonoWorld*)m_world)->m_24->FindSprite("MONOLITH");
    if (found == 0) {
        CMonoSprite* spr =
            ((CMonoWorld*)m_world)
                ->m_24->CreateSprite(0x20, 0x20, geoA, geoB, -0x19, -0x19, "MONOLITH");
        if (spr == 0) {
            return 0;
        }
        spr->m_9c.SetAtGrow(0, rec);
        spr->m_8 |= 0xc;
        spr->m_80 = 0xf4241;
        i32 parity = 1;
        for (i32 i = 0; i < spr->m_2c; i++) {
            for (i32 j = 0; j < spr->m_28; j++) {
                i32 val = parity ? savedIdx : -1;
                parity ^= 1;
                spr->m_20[spr->m_24[i] + j] = val;
            }
            parity ^= 1;
        }
        g_6455e8 = 1;
        return 1;
    }
    if (found->m_8 & 2) {
        found->m_8 &= ~2;
        g_6455e8 = 1;
    } else {
        found->m_8 |= 2;
        g_6455e8 = 0;
    }
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::SetGruntColor (0x0910d0; ret 0xc). Recolors one cell of a target
// row (sink): resolves `key`'s color row in the world's +0x10 lookup table, gates
// on that row's current cell (row->m_14[row->m_64] != 0), then - when `col` is in
// the sink's column range [sink->m_64..sink->m_68] - takes the sink's cell
// (sink->m_14[col]) and recolors it. Returns 1 on a hit, 0 otherwise.
// The guard chain nests so the success path is deepest and every reject `je`s the
// single trailing `return 0` (retail's shared `xor eax; pop; ret` tail) instead of
// a per-guard epilogue; see docs/patterns/nested-if-success-deepest-error-tail.md.
// @early-stop
// 95.81% regalloc tiebreak: logic + the nested fail-tail + the range gate are
// byte-exact. The lone residual is the sink-field loads (m_64/m_14): retail keeps
// them in edx (ecx is busy holding the row-cell it loads with `mov ecx,[..];test`),
// MSVC here folds the row-cell test to `cmp [mem],0` (no register) and so the sink
// loads land in ecx (8b4e64 vs retail 8b5664). A 4-instruction ecx<->edx swap with
// no source spelling that flips MSVC's `cmp [mem],0` back to a load-and-test;
// regalloc family (docs/patterns/pin-local-for-callee-saved-reg.md).
RVA(0x000910d0, 0x75)
i32 CGruntzMgr::SetGruntColor(i32 sinkArg, i32 key, i32 idx) {
    CColorRow* sink = (CColorRow*)sinkArg;
    if (sink && key) {
        i32* out = 0;
        ((CWorldLookupView*)m_world)->m_10->m_10.Lookup(key, &out);
        CColorRow* row = (CColorRow*)out;
        if (row && row->m_14[row->m_64]) {
            i32 cell = (idx < sink->m_64 || idx > sink->m_68) ? 0 : sink->m_14[idx];
            if (cell != 0) {
                RecolorCell(cell);
                return 1;
            }
        }
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::SetColorDepth (0x091170; ret 4). Sets the engine's packed RGB color
// (g_surfaceColorKey) for the given display depth. The depth must be 8/16/24 and a
// world must be loaded; otherwise it returns 0. For 8bpp the color clears; for
// 24bpp it is the fixed 0xff0084; for 16bpp it is repacked from the per-channel
// shift/mask globals (red 0xff, green 0, blue 0x84 each sar'd down then shl'd up
// and masked to 16 bits). Returns 1 once stored.
// @early-stop
// CODE byte-exact: the double depth switch, the 8/24bpp stores, and the 16bpp
// per-channel pack (u16-truncated so the three `and 0xffff` stay inline, matching
// retail's schedule) all match to the byte. The residual ~17% is the reloc-typing
// scoring artifact: the g_683ea*/g_surfaceColorKey DIR32 data refs score against
// the differently-typed delinked target (docs/matching-patterns.md fuzzy% note),
// NOT a code difference.
RVA(0x00091170, 0xad)
i32 CGruntzMgr::SetColorDepth(i32 depth) {
    if (depth != 8 && depth != 0x10 && depth != 0x18) {
        return 0;
    }
    if (m_world == 0) {
        return 0;
    }
    if (depth == 8) {
        g_surfaceColorKey = 0;
        return 1;
    }
    if (depth == 0x10) {
        // Each channel is masked to 16 bits and OR'd incrementally (retail emits
        // the `and 0xffff` per channel inline, not one fold over the combined
        // result): red (0xff), green (0), blue (0x84). The masks are spelled as
        // u16 truncations so MSVC keeps them per-channel (an explicit `& 0xffff`
        // on all three gets reassociated to one fold).
        i32 packed = (u16)((0xff >> g_683eac) << g_683ea0);
        packed |= (u16)((0 >> g_683eb0) << g_683ea4);
        packed |= (u16)(0x84 >> g_683eb4);
        g_surfaceColorKey = packed;
        return 1;
    }
    if (depth == 0x18) {
        g_surfaceColorKey = 0xff0084;
        return 1;
    }
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::LoadWorldMode (0x091a40; __thiscall; /GX EH; ret 4). Switches the
// loaded world to a new display color-depth (8 or 16) and rebuilds the per-mode
// engine objects. Bails (0) with no world; succeeds immediately (1) if already in
// that mode. Tears down the input (+0x54) and surface (+0x34) objects, records the
// mode (+0x88), resets the reload globals, re-asserts the display mode through the
// world's mode-set vtable (slot 6, 640x480), registers the mode-reset callback,
// stamps the view's tile region to 0xe, rebuilds the world objects, resolves +
// applies the rez path (a CString temp gives the /GX frame), then rebuilds the
// input object (+0x54) and re-arms it per the m_isAmbientEnabled gate, finally re-storing the
// input flag. Each engine reject surfaces an error and returns 0.
// @early-stop
// big /GX state-machine reload (~27%): the whole flow + per-stage error ladder
// are reconstructed and the /GX frame + the head (world/mode/8|16 guards + the
// m_54/m_34 two-stage teardowns) match. The low % is a big-SEH scoring desync:
// (a) the long chain of reloc-masked engine thiscalls (RezBuild/Apply/Teardown,
// CObList ctor/dtor, the world mode-set vtable, MakeRezPath/ResolveRezRow) each
// fuzzy-mismatch until their whole referent set is named; (b) the entry `push ecx`
// local-slot reservation + the CString-temp EH-state numbering on the fail chain
// (gx-state-machine + eh-state-numbering walls). Logic-complete; deferred to the
// final sweep / a leaf-first redo (docs/patterns/big-seh-fuzzy-desync.md).
RVA(0x00091a40, 0x2f9)
i32 CGruntzMgr::LoadWorldMode(i32 mode) {
    if (m_world == 0) {
        return 0;
    }
    if (m_colorDepth == mode) {
        return 1;
    }
    if (mode != 8 && mode != 0x10) {
        return 0;
    }

    CInput54* in = (CInput54*)m_inputState;
    if (in) {
        in->Flush();
        ((CObListSub*)((char*)in + 8))->Dtor();
        RezFree(in);
    }
    m_inputState = 0;

    CRezSurface94* surf = (CRezSurface94*)m_34;
    if (surf) {
        surf->Teardown();
        RezFree(surf);
    }
    m_34 = 0;

    m_colorDepth = mode;
    g_6455e0 = 0;
    g_6455dc = 0;
    if (m_colorDepth == 0x10) {
        g_6455dc = 1;
    }

    ((CWorldModeIface*)m_world)->Notify();
    i32 kind = (g_6455b4 == 0) ? 1 : 5;
    if (((CWorldModeIface*)m_world)
            ->SetVideoMode((i32)((CGameWnd*)m_4)->m_hwnd, 0x280, 0x1e0, m_colorDepth, kind)
        == 0) {
        ReportWorldStatus(0x43f);
        return 0;
    }

    ((CWorldRegistrar*)m_world)->RegisterCallback((void*)ModeResetCallback);
    CWorldModeView* view = (CWorldModeView*)m_world->m_24;
    view->m_64 = 0xe;
    view->m_68 = 0xe;
    CreateWorldObjects(m_world);
    if (MakeRezPath() == 0) {
        return 0;
    }

    CRezSurface94* old = (CRezSurface94*)m_34;
    if (old) {
        old->Teardown();
        RezFree(old);
        m_34 = 0;
    }
    CRezSurface94* obj = (CRezSurface94*)RezAlloc(0x94);
    if (obj) {
        obj->Build();
    } else {
        obj = 0;
    }
    m_34 = (i32)obj;

    CString path;
    i32* row = ResolveRezRow(&path);
    if (((CRezSurface94*)m_34)->Apply(*row, 1, 0)) {
        ReportError(0x800b, 0x441);
        return 0;
    }

    SetColorDepth(m_colorDepth);

    CInput54* in2 = (CInput54*)m_inputState;
    if (in2) {
        in2->Flush();
        ((CObListSub*)((char*)in2 + 8))->Dtor();
        RezFree(in2);
    }
    m_inputState = 0;

    void* no = RezAlloc(0x30);
    CInput54* ni;
    if (no) {
        ((CObListSub*)((char*)no + 8))->Init(0xa);
        *(i32*)no = 0;
        *(i32*)((char*)no + 4) = 0x64;
        ni = (CInput54*)no;
    } else {
        ni = 0;
    }
    m_inputState = (i32)ni;
    if (ni->InitInput(m_world->m_28, m_inputFlag) == 0) {
        ReportError(0x800a, 0x442);
        return 0;
    }

    CInput54* cur = (CInput54*)m_inputState;
    if (m_isAmbientEnabled != 0) {
        if (cur->m_24 == 0) {
            cur->m_24 = 1;
            cur->Arm();
        }
    } else {
        if (cur->m_24 != 0) {
            cur->m_24 = 0;
            cur->Disarm();
        }
    }
    StoreInputFlag(m_inputFlag);
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ResetWorldState (0x091e20; __thiscall; /GX EH; ret). Quiesces the
// loaded world when the live state is the idle (5) or paused-exit (2) state:
// raises the two busy gates (+0xac/+0xb0), tears the live state down (its vtbl slot
// 0), forces the hardware cursor visible (while ShowCursor(TRUE) < 0), then under
// an MFC wait-cursor runs the mode reload (LoadWorldMode(8 or 0x10)); on failure it
// surfaces a (0x801f) error, drops the wait cursor, and returns 0. On success it
// drives the post-reload transition (slot ?, args (savedId,1,0,0)), clears the
// gates, restores the cursor, and returns 1. A no-op (1) when not idle/exiting.
// @early-stop
// /GX wait-cursor wrapper (~60%): logic + the ShowCursor loops + the per-exit
// Begin/EndWaitCursor pairs are reconstructed. The residual is the /GX EH frame
// itself: retail wraps the body in a `CWaitCursor wc;` RAII local (ctor inlines
// AfxGetApp()->BeginWaitCursor, dtor inlines EndWaitCursor at each exit, + the
// [esp+N] 0/-1 trylevel around its lifetime), which emits the push -1/fs:0 frame
// my frameless Begin/EndWaitCursor free-fn model lacks. Closing it needs <afxwin.h>
// + a real CWaitCursor value member (deferred to the final sweep - heavier include
// that would re-flow this whole TU). Documented EH-frame wall (docs/seh-eh.md).
RVA(0x00091e20, 0x17d)
i32 CGruntzMgr::ResetWorldState(i32 notify) {
    CState* st = m_curState;
    if (st == 0) {
        return 1;
    }
    i32 stateId = st->Update();
    if (stateId != 5 && stateId != 2) {
        return 1;
    }

    CState* s = m_curState;
    m_modalBusy = 1;
    m_b0 = 1;
    if (s) {
        delete s;
        m_curState = 0;
    }

    i32(WINAPI * show)(i32) = g_pShowCursor;
    while (show(1) < 0) {
    }

    BeginWaitCursor();

    if (m_colorDepth == 8) {
        if (LoadWorldMode(0x10) == 0) {
            ReportError(0x801f, 0x443);
            EndWaitCursor();
            return 0;
        }
    } else {
        if (LoadWorldMode(8) == 0) {
            ReportError(0x801f, 0x444);
            EndWaitCursor();
            return 0;
        }
    }

    while (show(0) >= 0) {
    }
    SwitchModeState(stateId, 1, 0, 0);
    m_modalBusy = 0;
    m_b0 = 0;
    EndWaitCursor();
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::StopBankIfActive (0x092000; ret). When a sound object is bound and a
// level is loaded, tail-calls m_sound->StopAll().
RVA(0x00092000, 0x16)
void CGruntzMgr::StopBankIfActive() {
    if (m_sound && m_14) {
        m_sound->StopAll();
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::StopBank0IfActive (0x092030; ret). When a sound object is bound and
// a level is loaded, stops the bank with flag 0.
RVA(0x00092030, 0x18)
void CGruntzMgr::StopBank0IfActive() {
    if (m_sound && m_14) {
        m_sound->StopBank(0);
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::IsLobbyHostReady (0x091500). Null-chain predicate: returns
// m_curState->ReleaseResources-slot result (slot 7, +0x1c) only when m_curState, the
// CGameApp (m_8), its +0x240 sub-object and !m_modalBusy all hold; else 0.
RVA(0x00091500, 0x42)
i32 CGruntzMgr::IsLobbyHostReady() {
    if (m_curState == 0) {
        return 0;
    }
    CGameApp* app = (CGameApp*)m_8;
    if (app == 0) {
        return 0;
    }
    if (app->m_240 == 0) {
        return 0;
    }
    if (m_modalBusy != 0) {
        return 0;
    }
    return m_curState->Vslot07() != 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::StoreInputState (0x091a10; ret 4). Stores v at +0x120, and when the
// +0x60 sub-object is present mirrors it into that object's +0x2c.
RVA(0x00091a10, 0x17)
i32 CGruntzMgr::StoreInputState(i32 v) {
    m_inputStateVal = v;
    TimerObj* timer = (TimerObj*)m_timer;
    if (timer) {
        timer->m_inputMirror = v;
    }
    return v;
}

// -------------------------------------------------------------------------
// CGruntzMgr::TickStateMgrs (0x0920b0). Drives the two engine state singletons
// (g_645570/g_645578) once and reports success.
RVA(0x000920b0, 0x1c)
i32 CGruntzMgr::TickStateMgrs() {
    g_645570->PollAll();
    g_645578->Flush();
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::SetRunState (0x092340; __thiscall; ret 4). Sets the base run-state
// flag (CGameMgr::m_10) and, when it changes AND a world is loaded, runs the
// transition side-effects: tear down the world's inner controller
// (m_world->m_28->m_2c, guarded), mirror the new state into the g_61ab20 gate,
// then flush the +0x54 input object - Method1 when entering the run state
// (m_10 != 0), Method0 when leaving it. A no-op when the value is unchanged, and
// the whole side-effect chain is skipped when no world is loaded.
// @early-stop
// 99.62% global-store regalloc tiebreak: logic byte-exact. The lone residual is
// the g_61ab20 store - retail re-reads m_10 into eax and uses the `a3` accumulator
// store (mov ds:g_61ab20,eax), MSVC here loads it into ecx (mov [g_61ab20],ecx,
// 89 0d). A 1-instruction eax<->ecx pick on the global store; no source spelling
// flips it (see docs/patterns/select-zero-mask-dest-register.md, regalloc family).
RVA(0x00092340, 0x49)
void CGruntzMgr::SetRunState(i32 v) {
    if (v == m_10) {
        return;
    }
    m_10 = v;
    if (m_world == 0) {
        return;
    }
    CWorldSub2c* sub = m_world->m_28->m_2c;
    if (sub) {
        sub->Teardown();
    }
    g_61ab20 = m_10;
    if (m_10) {
        ((InputState54*)m_inputState)->Method1();
    } else {
        ((InputState54*)m_inputState)->Method0();
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::FindStateById (0x092900; ret 4). Returns the live state when its
// Update() id matches `id`; otherwise the first stack entry whose Update() id
// matches (or 0).
RVA(0x00092900, 0x6e)
CState* CGruntzMgr::FindStateById(i32 id) {
    if (m_curState && m_curState->Update() == id) {
        return m_curState;
    }
    CStateStackZ* st = (CStateStackZ*)&m_stateStack;
    for (i32 i = 0; i < st->m_nSize; i++) {
        CState* s = st->m_pData[i];
        if (s && s->Update() == id) {
            return s;
        }
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::PickPlayOrPausedState (0x092990; ret). FindStateById(3).
RVA(0x00092990, 0x8)
CState* CGruntzMgr::PickPlayOrPausedState() {
    return FindStateById(3);
}

// -------------------------------------------------------------------------
// CGruntzMgr::PickPausedThenPlayState (0x0929b0; ret). Prefers the paused/hold
// state (id 0x11), falling back to PLAY (id 3).
RVA(0x000929b0, 0x19)
CState* CGruntzMgr::PickPausedThenPlayState() {
    CState* s = FindStateById(0x11);
    if (s) {
        return s;
    }
    return FindStateById(3);
}

// -------------------------------------------------------------------------
// CGruntzMgr::SetSoundLevelState (0x0923b0; ret 4). Sets the base "level loaded"
// flag (CGameMgr::m_14) and, when it CHANGES and a sound bank is bound, drives the
// bank: clearing it stops everything (StopAll); setting it re-launches or stops the
// current bank depending on its per-bank restart flag (m_pCurrent->m_48).
// @early-stop
// ~80% redundant-null-test elision wall: logic byte-exact, the StopAll-at-tail block
// layout + the m_48 Restart/StopBank split all match. The lone residual is the
// second `if(cur)` guard before StopBank: retail kept the (provably-true) `test
// eax,eax; je` re-test of m_pCurrent that our MSVC5 eliminated (it proved cur!=0
// after the earlier null guard) + a `push 1` schedule shift. Same compiler, no
// source spelling reinstates the dead test; regalloc/elimination family (see
// docs/patterns/reread-member-view-pointer.md).
RVA(0x000923b0, 0x47)
void CGruntzMgr::SetSoundLevelState(i32 loaded) {
    if (loaded == m_14) {
        return;
    }
    m_14 = loaded;
    CGruntzSoundZ* snd = m_sound;
    if (snd == 0) {
        return;
    }
    if (loaded != 0) {
        CGruntzSoundInnerZ* cur = snd->m_1c;
        if (cur == 0) {
            return;
        }
        if (cur->m_48 != 0) {
            snd->Restart_1388c0(1);
        } else if (cur != 0) {
            snd->StopBank(1);
        }
        return;
    }
    snd->StopAll();
}

// -------------------------------------------------------------------------
// CGruntzMgr::RunLoadGameDialog (0x092500; ret). Shows the "GAME_LOAD" modal
// dialog (through the engine's DialogBoxParamA wrapper) and returns 1.
RVA(0x00092500, 0x17)
i32 CGruntzMgr::RunLoadGameDialog() {
    RunModalDialog("GAME_LOAD", (void*)GruntzLoadGameDlgProc, 0);
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::Quicksave (0x092530; __thiscall; /GX EH; ret). Quicksaves the game.
// Bails (0) with no save sink (m_saveSink) or when not in the PLAY state (id 3).
// When the first-frame guard (m_44->m_124) is already set, it pops a localized
// message (resource 0x81aa) through a modal and returns 1. Otherwise, on a valid
// save record (m_saveInfoRec with bit 0 set) and a live save source
// (m_curState + 0x1d0), it stops the timer, fills the save info, then commits the
// save via the registry notifier; success logs "Game Quicksaved successfully." into
// the chat log, failure shows the error modal.
// @early-stop
// 93.4% - logic + the whole control flow (the modal/LoadString branch, the inlined
// save-source guard, the FillSaveInfo/Notify commit, the chat insert) are byte for
// byte. The residual is the reloc-masked g_gameReg/$SG string DIR32 scoring + the
// /GX trylevel state numbering across the destructible CString temp (documented
// eh-state-numbering + reloc-typing walls, see docs/seh-eh.md). Final sweep.
RVA(0x00092530, 0x17c)
i32 CGruntzMgr::Quicksave() {
    if (m_saveSink == 0) {
        return 0;
    }
    if (m_curState->Update() != 3) {
        return 0;
    }
    if (((HudGuard44*)m_44)->m_124 != 0) {
        CString name;
        name.LoadStringA(0x81aa);
        EnterModalUI((i32)(const char*)name);
        return 1;
    }
    if (m_saveInfoRec == 0 || !(*(char*)m_saveInfoRec & 1)) {
        return LoadSaveMessageSprite();
    }
    if ((char*)m_curState + 0x1d0 == 0) { // inlined GetSaveSource() non-null guard
        return 0;
    }
    if (m_timer) {
        ((TimerObj*)m_timer)->Stop();
    }
    FillSaveInfo((SaveInfo*)m_saveInfoRec, 0);
    if (((ScoreNotifier*)g_gameReg->m_58)->Notify((i32)((char*)m_saveInfoRec + 0x35), 0x81a7)) {
        ((CChatLog*)m_5c)->Insert("Game Quicksaved successfully.", 0, 0x11);
        return 1;
    }
    EnterModalUI((i32) "ERROR - Cannot Save Game.");
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::RunDebugGruntTypeDialog (0x0929e0; ret). Only in the PLAY state (id
// 3), shows the "DEBUG_GRUNTTYPE" modal dialog (flag 1); returns whether it ran
// (bool-normalized via setne).
RVA(0x000929e0, 0x32)
i32 CGruntzMgr::RunDebugGruntTypeDialog() {
    i32 ran = 0;
    if (m_curState->Update() == 3) {
        ran = RunModalDialog("DEBUG_GRUNTTYPE", (void*)GruntzDebugGruntTypeProc, 1);
    }
    return ran != 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::LoadOptionsSlotName (0x092d50; ret 0x1c). When in a play-ish state
// (CheckPlayState) and the indexed options slot has not yet been loaded
// (slot->m_20 == 0), assign its per-slot name CString from `val`. Returns 0. Only
// the slot index (arg0) + the value string (arg5) are used; the rest are ignored.
RVA(0x00092d50, 0x3c)
i32 CGruntzMgr::LoadOptionsSlotName(
    i32 slot,
    i32 /*a2*/,
    i32 /*a3*/,
    i32 /*a4*/,
    i32 /*a5*/,
    const char* val,
    i32 /*a7*/
) {
    if (CheckPlayState()) {
        // Authentic codegen idiom: the base folds the array's +0x150 into the
        // field displacements (+0x170 = m_20, +0x154 = m_name), so cl emits the
        // slot lea with disp 0; naming via &m_options[slot] shifts the lea base
        // and drops the match (verified -3%). Kept raw.
        OptionsSlot* s = (OptionsSlot*)((char*)this + slot * 0x238);
        if (*(i32*)((char*)s + 0x170) == 0) {    // s->m_20 (options base +0x150 +0x20)
            *(CString*)((char*)s + 0x154) = val; // s->m_name (options base +0x150 +0x04)
        }
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::CountReadyOptionsSlots (0x092e30; ret 4). Counts the four options
// slots that are loaded (m_20 != 0) and either `anyState` is set or the slot is
// armed (m_14 != 0). The running pointer walks at the slot's m_14 field.
RVA(0x00092e30, 0x39)
i32 CGruntzMgr::CountReadyOptionsSlots(i32 anyState) {
    i32 count = 0;
    char* p = (char*)&m_options[0] + 0x14; // &m_options[0].m_14
    for (i32 d = 4; d != 0; d--) {
        char* slot = p - 0x14; // slot base
        if (slot && *(i32*)(p + 0xc) != 0 && (anyState != 0 || *(i32*)p != 0)) {
            count++;
        }
        p += 0x238;
    }
    return count;
}

// -------------------------------------------------------------------------
// CGruntzMgr::FindOptionsSlot (0x092e80; ret 4). Returns the first options slot
// whose key field (m_18) equals `x`, or 0 when none match.
// @early-stop
// ~83% regalloc reg-swap wall: logic + the dual-induction loop (slot ptr in eax,
// index, +0x238 stride, the per-iteration null guard + m_18 compare) are byte-
// exact. The residual is the scratch-register coloring - retail keeps the key `x`
// in ecx and the loop index in edx; our MSVC5 swaps them (x in edx, i in ecx) +
// the inc/add tail order. No source spelling flips MSVC's register pick here;
// zero-register-pinning family (docs/patterns/zero-register-pinning.md).
RVA(0x00092e80, 0x25)
OptionsSlot* CGruntzMgr::FindOptionsSlot(i32 x) {
    OptionsSlot* slot = (OptionsSlot*)m_options;
    i32 i = 0;
    do {
        if (slot && slot->m_18 == x) {
            return slot;
        }
        i++;
        slot = (OptionsSlot*)((char*)slot + 0x238);
    } while (i < 4);
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ChangeState_8fab0 (0x08fab0) - deferred big method; the thin
// RunFromState wrapper calls it (reloc-masked). Migrated from Discovered.cpp.
// @confidence: high
// @source: call-xref
// @stub
RVA(0x0008fab0, 0x318)
i32 CGruntzMgr::ChangeState_8fab0(i32 /*arg*/) {
    return 0;
}

// CGruntzMgr::UnknownClose (@0x0855e0) is the large member-teardown method the
// dtor calls; its body is still the stub (src/Stub/CGruntzMgr.cpp). It is only
// DECLARED here (in the header) - the dtor's call to it is an external ref whose
// reloc objdiff masks, so no definition is needed in this TU.

// -------------------------------------------------------------------------
// CGruntzMgr::~CGruntzMgr  (virtual; vtable slot 0; own vftable @0x5e9b64)
// The own body just runs UnknownClose(); the compiler then destructs the five
// destructible members (m_options, m_strMoviePath, m_strEC, m_stateStack, m_strWorldFile, in
// reverse-construction order) and chains the base ~CGameMgr - all under the /GX
// C++ EH frame (per-member unwind states 4..0).
RVA(0x00083360, 0xb2)
CGruntzMgr::~CGruntzMgr() {
    UnknownClose();
}

// -------------------------------------------------------------------------
// CGruntzMgr::RecomputeViewScale (0x08f7f0; ret). Recomputes the world view's
// three scaled-extent pairs from its tile rect (width/height + 1) at three zoom
// factors (1.4 / 5.3 / 1.12), notifying the view after each pair, then snapshots
// the view's edge origins (m_5c +0x40..+0x4c, biased by 0x60) into m_viewOriginL..m_148.
// No-op when no world is loaded. The int extents are converted to float once and
// reused across the three scale passes (fild/fst then fld).
// @early-stop
// ~83% x87-scheduling wall: logic is exact (same 1.4/5.3/1.12 scales, same view
// writes + per-pass Notify + the m_viewOriginL..m_148 edge snapshot). Retail keeps the
// freshly-converted extent in st0 with `fst` (store-and-keep) and multiplies it
// in place; MSVC here emits `fstp` (store-and-pop) + a reload `fld`, a 1-2 instr
// FPU-stack scheduling drift per pass. No source spelling flips fst<->fstp;
// interleaving the conversions makes it worse (78%).
RVA(0x0008f7f0, 0x131)
void CGruntzMgr::RecomputeViewScale() {
    if (m_world == 0) {
        return;
    }
    CScrollView* view = (CScrollView*)m_world->m_24;
    float fw = (float)(view->m_18 - view->m_10 + 1);
    float fh = (float)(view->m_1c - view->m_14 + 1);

    view->m_c8 = (i32)(fw * 1.4f);
    view->m_cc = (i32)(fh * 1.4f);
    view->Notify();

    view = (CScrollView*)m_world->m_24;
    view->m_d0 = (i32)(fw * 5.3f);
    view->m_d4 = (i32)(fh * 5.3f);
    view->Notify();

    view = (CScrollView*)m_world->m_24;
    view->m_d8 = (i32)(fw * 1.12f);
    view->m_dc = (i32)(fh * 1.12f);
    view->Notify();

    CScrollView* v = (CScrollView*)m_world->m_24;
    if (v->m_5c == 0) {
        return;
    }
    m_viewOriginL = v->m_5c->m_40 - 0x60;
    m_viewOriginT = ((CScrollView*)m_world->m_24)->m_5c->m_44 - 0x60;
    m_viewOriginR = ((CScrollView*)m_world->m_24)->m_5c->m_48 + 0x60;
    m_viewOriginB = ((CScrollView*)m_world->m_24)->m_5c->m_4c + 0x60;
}

// -------------------------------------------------------------------------
// CGruntzMgr::BroadcastCmd (0x093460; ret 0x10). Fans a 4-arg game command out to
// every subsystem, short-circuiting (return 0) on the first that rejects it. For
// command id 4 it first arms via PrepCmd4 and for id 7 via PrepCmd7 (each must
// succeed), both then ticking the +0x60 controller; other ids skip straight to
// the broadcast. The fan-out order: the four options slots, the +0x68 grid, the
// live source object (GetSaveSource), the +0x6c sub-mgr, the +0x70 object (vtbl
// slot 1) and a free command hook, finishing with the +0x7c HUD whose result
// (normalized to 0/1) is returned.
// @early-stop
// ~78% block-layout wall: logic is exact and the cmd==7 arm + the whole fan-out
// (options loop, m_cmdGrid/source/m_cmdSubMgr/m_cmdNotify/hook/m_scoreHud) match byte for byte. Retail
// emits the cmd==4 arm OUT-OF-LINE at the function tail (`cmp 4; je <end>`; the
// 4-block jmps back into the shared m_timer->Tick), but MSVC here keeps it inline
// between the gate and the cmd==7 test, shifting that one block. Pure basic-block
// placement; case reorder doesn't move it (see docs/patterns/switch-cases-source-order.md).
// NOTE: the trace size was 0x124 but the real function runs to 0x15c (the cmd==4
// tail + the bool-normalizing HUD epilogue past the under-counted Ghidra bound).
RVA(0x00093460, 0x15c)
i32 CGruntzMgr::BroadcastCmd(i32 a0, i32 cmd, i32 a2, i32 a3) {
    if (a0 == 0) {
        return 0;
    }
    switch (cmd) {
        case 4:
            if (PrepCmd4(a0) == 0) {
                return 0;
            }
            ((CmdTimer60*)m_timer)->Tick();
            break;
        case 7:
            if (PrepCmd7(a0) == 0) {
                return 0;
            }
            ((CmdTimer60*)m_timer)->Tick();
            break;
    }

    OptionsSlot* slot = (OptionsSlot*)m_options;
    for (i32 i = 0; i < 4; i++) {
        if (slot == 0 || slot->Command(a0, cmd, a2, a3) == 0) {
            return 0;
        }
        slot = (OptionsSlot*)((char*)slot + 0x238);
    }

    if (((CmdSink*)m_cmdGrid)->Command(a0, cmd, a2, a3) == 0) {
        return 0;
    }
    if (((CmdSink*)GetSaveSource())->Command(a0, cmd, a2, a3) == 0) {
        return 0;
    }
    if (((CmdSink*)m_cmdSubMgr)->Command(a0, cmd, a2, a3) == 0) {
        return 0;
    }
    if (((CmdSinkV*)m_cmdNotify)->Command(a0, cmd, a2, a3) == 0) {
        return 0;
    }
    if (CmdHook(a0, cmd, a2, a3) == 0) {
        return 0;
    }
    return ((CmdSink*)m_scoreHud)->Command(a0, cmd, a2, a3) != 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::UpdateScoreHud (0x0860b0; ret). Per-frame score/HUD accumulator,
// active only while the registry's gate (g_gameReg->m_134) is 1. Folds this
// world's score/time deltas (m_cmdGrid's +0x20c/+0x21c tables, indexed by the active
// world g_644c54) into the +0x7c HUD accumulators. If a level name is loaded
// (m_strWorldFile non-empty) it just refreshes the HUD with 1 and marks it dirty;
// otherwise, on the first frame (m_44->m_124 == 0) it seeds the HUD from the
// registry's cumulative score and fires the score-bump / tick / notify chain,
// then refreshes the HUD with the live score and clears the dirty flag.
RVA(0x000860b0, 0xe8)
void CGruntzMgr::UpdateScoreHud() {
    if (g_gameReg->m_134 != 1) {
        return;
    }
    ScoreSub2c* sub = (ScoreSub2c*)g_gameReg->m_2c;

    ((ScoreHud*)m_scoreHud)->m_1c += ((WorldDeltaTables*)m_cmdGrid)->m_arr20c[g_644c54];
    ((ScoreHud*)m_scoreHud)->m_20 += ((WorldDeltaTables*)m_cmdGrid)->m_arr21c[g_644c54];

    if (m_strWorldFile.GetLength() != 0) {
        ((ScoreHud*)m_scoreHud)->Refresh(1);
        ((ScoreHud*)m_scoreHud)->m_8 = 1;
        return;
    }

    if (((HudGuard44*)m_44)->m_124 == 0) {
        ((ScoreHud*)m_scoreHud)->Seed(sub->m_1c, 0);
        ((ScoreNotifier*)g_gameReg->m_58)->Bump(sub->m_1c);
        ((ScoreNotifier*)g_gameReg->m_58)->Tick((sub->m_1c % 0x28) + 1);
        ((ScoreNotifier*)g_gameReg->m_58)->Notify(0, 0x81a6);
    }
    ((ScoreHud*)m_scoreHud)->Refresh(sub->m_1c);
    ((ScoreHud*)m_scoreHud)->m_8 = 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::SaveState (0x093620; ret 4; returns 1/0). Streams the whole
// game-clock / scroll / warp snapshot through the supplied archive. Bails (0)
// when no archive or no loaded world. Bumps the global save serial, writes the
// world file name (m_strWorldFile, copied into a fresh 0x80 buffer), then the per-frame
// state block (m_114..m_13c) and the clock/scroll/warp globals - each via the
// archive's Serialize(&field, size) slot. Returns 1.
RVA(0x00093620, 0x254)
i32 CGruntzMgr::SaveState(CSerializerZ* ar) {
    if (ar == 0) {
        return 0;
    }
    if (m_world == 0) {
        return 0;
    }
    g_629ad0++;

    char buf[0x80];
    memset(buf, 0, 0x80);
    strcpy(buf, (const char*)m_strWorldFile);
    ar->Serialize(buf, 0x80);

    ar->Serialize(&m_114, 4);
    ar->Serialize(&m_inputFlag, 4);
    ar->Serialize(&m_128, 4);
    ar->Serialize(&m_12c, 4);
    ar->Serialize(&m_130, 4);
    ar->Serialize(&m_134, 4);
    ar->Serialize(&m_optionsCount, 4);
    ar->Serialize(&m_viewOriginL, 0x10); // the 0x10-byte view-origin block (+0x13c..+0x148)
    ar->Serialize(&g_645580, 4);
    ar->Serialize(&g_645584, 4);
    ar->Serialize(&g_645588, 4);
    ar->Serialize(&g_64558c, 4);
    ar->Serialize(&g_645590, 4);
    ar->Serialize(&g_645594, 4);
    ar->Serialize(&g_645598, 4);
    ar->Serialize(&g_64559c, 4);
    ar->Serialize(&g_6455a0, 4);
    ar->Serialize(&g_6455b0, 4);
    ar->Serialize(&g_6455a8, 4);
    ar->Serialize(&g_6455a4, 4);
    ar->Serialize(&g_6455ac, 4);
    ar->Serialize(&g_6455f8, 4);
    ar->Serialize(&m_isEasyMode, 4);
    ar->Serialize(&g_6455e8, 4);
    ar->Serialize(&g_6452a4, 4);
    ar->Serialize(&g_6452cc, 4);
    ar->Serialize(&g_645508, 4);
    ar->Serialize(&g_64550c, 4);
    ar->Serialize(&g_warpX, 4);
    ar->Serialize(&g_warpY, 4);
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::LoadState (0x093920; ret 4). The deserialize counterpart of
// SaveState: streams the SAME clock/scroll/warp run back IN through the
// serializer's read slot (+0x2c, vs SaveState's write slot +0x30). Bails unless a
// reader is supplied and the GLOBAL manager's world is loaded (`g_pMgr->m_world`,
// the singleton at 0x64556c - not `this`), bumps the save serial, reads the world
// file name into a scratch buffer and assigns it to m_strWorldFile, then transfers
// each scalar field/global in the identical order as SaveState.
RVA(0x00093920, 0x22f)
i32 CGruntzMgr::LoadState(CSerializerZ* ar) {
    if (ar == 0) {
        return 0;
    }
    if (((CGruntzMgr*)g_gameReg)->m_world == 0) {
        return 0;
    }
    g_629ad0++;

    char buf[0x80];
    ar->Transfer(buf, 0x80);
    m_strWorldFile = buf;

    ar->Transfer(&m_114, 4);
    ar->Transfer(&m_inputFlag, 4);
    ar->Transfer(&m_128, 4);
    ar->Transfer(&m_12c, 4);
    ar->Transfer(&m_130, 4);
    ar->Transfer(&m_134, 4);
    ar->Transfer(&m_optionsCount, 4);
    ar->Transfer(&m_viewOriginL, 0x10); // view-origin block (+0x13c..+0x148)
    ar->Transfer(&g_645580, 4);
    ar->Transfer(&g_645584, 4);
    ar->Transfer(&g_645588, 4);
    ar->Transfer(&g_64558c, 4);
    ar->Transfer(&g_645590, 4);
    ar->Transfer(&g_645594, 4);
    ar->Transfer(&g_645598, 4);
    ar->Transfer(&g_64559c, 4);
    ar->Transfer(&g_6455a0, 4);
    ar->Transfer(&g_6455b0, 4);
    ar->Transfer(&g_6455a8, 4);
    ar->Transfer(&g_6455a4, 4);
    ar->Transfer(&g_6455ac, 4);
    ar->Transfer(&g_6455f8, 4);
    ar->Transfer(&m_isEasyMode, 4);
    ar->Transfer(&g_6455e8, 4);
    ar->Transfer(&g_6452a4, 4);
    ar->Transfer(&g_6452cc, 4);
    ar->Transfer(&g_645508, 4);
    ar->Transfer(&g_64550c, 4);
    ar->Transfer(&g_warpX, 4);
    ar->Transfer(&g_warpY, 4);
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::FillSaveInfo (0x0927b0; ret 8). Populates a save-slot record from
// the live game: bails if there is no record or no live source state, copies the
// level name into the record (inline strcpy), stamps the win/level state flags
// (m_134==3 -> m_fc, m_130 -> m_f8), hands the record + the source state's data
// block (+0x1d0) to the +0x58 sink, remembers the record at m_saveInfoRec, and - when a
// snapshot block is supplied - copies its 0x20 bytes into the record (+0x14).
// @early-stop
// ~99.2% regalloc tail tiebreak: logic + the EH-frame elision (the name temp is
// scoped so no throwing call is live during it) + the out-of-line EngineCopy all
// match; the residual is the m_134 compare landing in esi here vs retail's edi
// (freed by the inline-strcpy rep-movs) - a pure esi<->edi naming swap.
RVA(0x000927b0, 0xc4)
i32 CGruntzMgr::FillSaveInfo(SaveInfo* dst, void* snapshot) {
    if (dst == 0) {
        return 0;
    }
    char* src = (char*)GetSaveSource();
    if (src == 0) {
        return 0;
    }
    // Consume GetLevelName()'s returned CString temp inline (no named local):
    // MSVC reads m_pszData straight off the live return pointer in eax (`mov
    // edi,[eax]`) instead of re-addressing a stack slot, and - with no throwing
    // call live during the temp's range (the strcpy is inlined rep-movs) - /GX
    // still elides the EH frame, matching retail's frameless body.
    strcpy(dst->m_75, (const char*)GetLevelName());
    dst->m_fc = (m_134 == 3);
    dst->m_f8 = m_130;
    ((SaveSink58*)m_saveSink)->Store(dst, src + 0x1d0);
    m_saveInfoRec = (i32)dst;
    if (snapshot) {
        EngineCopy(dst->m_14, snapshot, 0x20);
    }
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::FinishLevel (0x08e980; ret 8). The end-of-level / leave-level path.
// When the live state is paused (0x11) and any of its 4 player-status slots
// reports "done" (status id 3), it runs the win hook under the m_c guard and
// returns 1. Otherwise it tears the level down: if `full` (arg0) it flushes the
// +0x54 input object, tears the world's inner controller chain, stops the sound
// bank when the bank is busy and arg1 is set, and notifies the live state via
// slot 24 (+0x60); then (always) when a level is loaded it stops the bank if the
// game is still in play, flushes the +0x54/+0x68 controllers, notifies slot 25
// (+0x64), ticks the input singleton and runs the post-switch hook. Returns 1.
// @early-stop
// regalloc wall (~84%): logic is exact + the paused-status block matches byte for
// byte. Retail reserves edi at entry (push edi) and reuses it - first as the
// status-done counter, then as arg0 (`full`) across the teardown - so the second
// half's [esp+N] offsets all sit +4 from mine (MSVC here keeps the counter in edx
// and re-reads `full`/`stopBank` off the stack, no edi push). Pure register-
// reuse/stack-offset shift; see docs/patterns/pin-local-for-callee-saved-reg.md.
RVA(0x0008e980, 0x11e)
i32 CGruntzMgr::FinishLevel(i32 full, i32 stopBank) {
    if (m_curState && m_curState->Update() == 0x11) {
        PlayStatusSlot* base = ((CPlayStateView*)m_curState)->m_520;
        char* p = (char*)base + 0x20;
        i32 done = 0;
        for (i32 d = 4; d != 0; d--) {
            if (p && *(i32*)p == 3) {
                done++;
            }
            p += 0x64;
        }
        if (done > 0) {
            m_c = 1;
            RunWinHook();
            m_c = 0;
            return 1;
        }
    }

    if (full) {
        if (m_inputState) {
            ((InputState54*)m_inputState)->Method0();
        }
        if (m_world) {
            CWorldSub28* sub = m_world->m_28;
            if (sub && sub->m_2c) {
                sub->m_2c->Teardown();
            }
        }
        if ((m_sound->m_1c ? m_sound->m_1c->IsBusy() : 0) && stopBank) {
            m_sound->StopAll();
        }
        m_curState->Vslot18();
        if (full) {
            return 1;
        }
    }

    if (m_14) {
        if (CheckLevelActive()) {
            m_sound->StopBank(1);
        }
    }
    if (m_10) {
        ((InputState54*)m_inputState)->Method1();
        if (m_cmdGrid && m_10) {
            ((Ctrl68*)m_cmdGrid)->Reset();
        }
    }
    m_curState->Vslot19();
    g_645570->Flush();
    PostSwitchHook();
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::EnterModalUI (0x08ef10; ret 4). Suspends the in-game world for a
// modal screen: stops the +0x60 timer if any, forces a map redraw + ticks the
// world's dispatch object, then brings the hardware cursor visible
// (while (ShowCursor(TRUE) < 0)), runs the modal handler on the app
// (m_8->RunModal(arg, hwnd)) with the cursor-busy gate raised, clears the gate,
// and - if the cursor was already shown on entry - hides it again
// (while (ShowCursor(FALSE) >= 0)). No-op when there is no app bound.
// @early-stop
// regalloc tiebreak (~93%): logic + the cached ShowCursor fn-ptr are exact; MSVC
// here keeps `this` in edi and the fn-ptr in esi, retail does the reverse (esi
// for `this`, edi for the ptr) - a pure esi<->edi naming swap (call ff d6 vs
// ff d7). Not steerable from source; see docs/patterns/zero-register-pinning.md.
RVA(0x0008ef10, 0x9e)
void CGruntzMgr::EnterModalUI(i32 arg) {
    CGameApp* app = (CGameApp*)m_8;
    if (app == 0) {
        return;
    }
    if (m_timer) {
        ((TimerObj*)m_timer)->Stop();
    }
    if (m_world) {
        RedrawMapIndex(m_world->m_4->m_14);
        CWorldDispatch* d = *m_world->m_1c;
        d->Slot0a();
    }

    i32(WINAPI * show)(i32) = g_pShowCursor;
    i32 shown = show(1);
    while (show(1) < 0) {
    }

    m_modalBusy = 1;
    app->RunModal(arg, ((CGameWnd*)m_4)->m_hwnd);
    g_64557c = 0;
    m_modalBusy = 0;
    if (shown <= 0) {
        while (show(0) >= 0) {
        }
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::ExitModalUI (0x0903f0; ret 8; returns the modal result). Runs a
// modal dialog/screen (dlg->vtbl[+0xc0]) over the suspended game: stops the
// +0x60 timer, flushes the +0x68 controller (when m_10 is set), and - if the
// world is loaded and `notify` is set and the live state isn't already idle (id
// 5) - notifies the live state it is exiting (NotifyExit(0x32)) and ticks the
// world dispatch object; otherwise clears `notify`. It then forces the cursor
// visible, runs the dialog with the busy gate raised, clears the gate, optionally
// polls the live state (Vslot06) when notify held, restores the cursor, runs the
// post-switch hook, and finalizes the freshly-activated object (+0x2dc sub +
// self). The dialog's return value is the function's result.
RVA(0x000903f0, 0x10c)
i32 CGruntzMgr::ExitModalUI(CModalDialog* dlg, i32 notify) {
    if (m_timer) {
        ((TimerObj*)m_timer)->Stop();
    }
    if (m_cmdGrid && m_10) {
        ((Ctrl68*)m_cmdGrid)->Flush();
    }
    if (m_world) {
        if (notify && m_curState && m_curState->Update() != 5) {
            m_curState->NotifyExit(0x32);
        } else {
            notify = 0;
        }
        CWorldDispatch* d = *m_world->m_1c;
        d->Slot0a();
    }

    i32(WINAPI * show)(i32) = g_pShowCursor;
    i32 shown = show(1);
    while (show(1) < 0) {
    }

    m_modalBusy = 1;
    i32 result = dlg->Run();
    g_64557c = 0;
    m_modalBusy = 0;
    if (m_curState && notify) {
        m_curState->Vslot06();
    }

    if (shown <= 0) {
        while (show(0) >= 0) {
        }
    }

    PostSwitchHook();
    CActiveObj* o = GetActiveObj();
    if (o) {
        if (o->m_2dc) {
            o->m_2dc->Release();
        }
        o->Finalize();
    }
    return result;
}

// -------------------------------------------------------------------------
// CGruntzMgr::SwitchToNextState (0x08d6a0; ret). If the manager is ready
// (vtbl slot 3) and a next state can be built (MakeNextState), and it differs
// from the live state, it tears the live state down (notify slot 10 with the new
// state's id, then delete it), installs the new state (m_curState = next; Activate),
// then notifies slot 9 (Vslot09) with the old state's id and polls slot 6
// (Vslot06); on either success it arms the app's resume flag (m_8->+0x244) and
// runs the post-switch hook, returning 1. Otherwise returns 0.
RVA(0x0008d6a0, 0xaf)
i32 CGruntzMgr::SwitchToNextState() {
    if (Wap32GameMgrVfunc3() == 0) {
        return 0;
    }
    CState* next = MakeNextState();
    if (next == 0) {
        return 0;
    }
    if (m_curState == next) {
        return 0;
    }
    i32 oldId = 0;
    if (m_curState) {
        oldId = m_curState->Update();
        m_curState->FrameSlot28(next->Update());
        if (m_curState) {
            delete m_curState;
        }
        m_curState = 0;
    }
    m_curState = next;
    ActivateState(next);
    if (m_curState->Vslot09(oldId) == 0 && m_curState->Vslot06() == 0) {
        return 0;
    }
    ((CGameApp*)m_8)->m_244 = 1;
    PostSwitchHook();
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::PassClickToPlayState (0x08d780; ret 0xc). When the live state is
// the PLAY (3) or paused (0x11) state and the gate arg a1 is clear, it forwards
// the click into the state: notify slot 10 (FrameSlot28) with the state id, then
// route the (a0, a2) hit through slot 30 (Vslot1e); on a hit it also notifies
// slot 9 (Vslot09) and returns 1, otherwise returns 0. When not in PLAY/paused
// it pushes a fresh PLAY transition via ChangeToPlayState(3, a0, 0, 0).
// @early-stop
// regalloc wall: in the hit block retail caches m_curState's vtbl in callee-saved ebx
// across the two virtual calls (push ebx at entry, stack args shift +4); MSVC
// here keeps the vtbl in edi and re-reads it, so no ebx push. Logic is exact;
// the residual is the vtbl-CSE register choice (see docs/patterns/
// pin-local-for-callee-saved-reg.md - no clean source spelling for vtbl pinning).
RVA(0x0008d780, 0x95)
i32 CGruntzMgr::PassClickToPlayState(i32 a0, i32 a1, i32 a2) {
    i32 inPlay = 0;
    if (m_curState->Update() == 3) {
        inPlay = 1;
    }
    if (m_curState->Update() == 0x11) {
        inPlay = 1;
    }
    if (inPlay && a1 == 0) {
        m_curState->FrameSlot28(m_curState->Update());
        if (m_curState->Vslot1e(a0, a2) == 0) {
            return 0;
        }
        m_curState->Vslot09(m_curState->Update());
        return 1;
    }
    return ChangeToPlayState(3, a0, 0, 0);
}

// -------------------------------------------------------------------------
// CGruntzMgr::UnloadSoundChain (0x08f740; ret). Tears down the loaded world's
// inner controller object (m_world->m_28->m_2c->Teardown(), each guarded) then, if
// the sound object's inner busy-poll reports busy, stops the bank via StopBank2.
RVA(0x0008f740, 0x46)
void CGruntzMgr::UnloadSoundChain() {
    if (m_world) {
        CWorldSub28* sub = m_world->m_28;
        if (sub) {
            CWorldSub2c* obj = sub->m_2c;
            if (obj) {
                obj->Teardown();
            }
        }
    }
    CGruntzSoundZ* snd = m_sound;
    if (snd && (snd->m_1c ? snd->m_1c->IsBusy() : 0)) {
        m_sound->StopBank2();
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::StoreInputFlag (0x0919d0; ret 4). Records the input flag at +0x11c;
// when the loaded world's +0x28 sub-object is present it also mirrors the flag
// into the g_61ab24 global; finally forwards the flag to the +0x54 input object.
RVA(0x000919d0, 0x30)
void CGruntzMgr::StoreInputFlag(i32 v) {
    m_inputFlag = v;
    if (m_world && m_world->m_28) {
        g_61ab24 = v;
    }
    InputStateObj* in = (InputStateObj*)m_inputState;
    if (in) {
        in->StoreFlag(v);
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::ClearOptionsSlots (0x092ec0; ret). Walks the 4-element options
// array at +0x150 and zeroes each element's +0x20/+0x24 pair (the array base +
// the `if (&elem)` guard fall out of the manual per-element loop).
RVA(0x00092ec0, 0x24)
void CGruntzMgr::ClearOptionsSlots() {
    char* p = (char*)&m_options[0] + 0x24; // &m_options[0].m_24
    for (i32 i = 4; i != 0; i--) {
        char* elem = p - 0x24;
        if (elem) {
            *(i32*)(p - 4) = 0; // elem.m_20
            *(i32*)p = 0;       // elem.m_24
        }
        p += 0x238;
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::GetWorldFileName (0x0928c0; returns CString by value; ret 4).
// Returns a copy of the world-file CString at +0xc8 (the copy-ctor is reloc-
// masked; the dest pointer comes in as the hidden NRVO arg).
RVA(0x000928c0, 0x23)
CString CGruntzMgr::GetWorldFileName() {
    return m_strWorldFile;
}

// -------------------------------------------------------------------------
// CGruntzMgr::AdvanceOptionsCycle (0x0933e0; ret). Bumps the global round-robin
// cursor g_6455fc (mod 4); for the options slot whose index matches the cursor,
// if it is armed (m_164 == 0) and loaded (m_170 != 0) it ticks the slot's +0x38
// sub-object. The loop runs i in [0, m_optionsCount]; both the count and the cursor are
// re-read each iteration (the cursor after the reloc-masked tick).
RVA(0x000933e0, 0x5e)
i32 CGruntzMgr::AdvanceOptionsCycle() {
    i32 cursor = (g_6455fc + 1) & 3;
    g_6455fc = cursor;
    for (i32 i = 0; i < m_optionsCount + 1; i++) {
        OptionsSlot* slot = (OptionsSlot*)&m_options[i];
        if (cursor == i && slot->m_14 == 0 && slot->m_20 != 0) {
            slot->m_38.Tick();
            cursor = g_6455fc;
        }
    }
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::SyncOptionsState (0x093170; __thiscall; /GX EH; ret 1/0). Reloads
// every options slot's saved config from disk and (re)arms the slots, then
// reseeds the RNG. A scratch CString loads resource 0x81ab and is strcmp'd vs
// the world-file name (+0xc8): a match clears the per-slot config selector (so
// the loaded config is replaced by 0). The reload walks the slots with three
// running pointers (m_10 config / m_14 arm / m_38 sub-object, +0x238 apart);
// the slot whose index hits the active world g_644c54 is the "current" one - it
// is armed (m_14=1), its sub-object Activated, and its SUCCESSOR slot is handled
// in the same pass (the dual-slot unroll: idx jumps by 2 but the loop counter by
// 1). Any LoadConfig failure aborts (delete the CString, return 0).
// @early-stop
// 92.95% - logic byte-exact (inline strcmp, srand(time(0)), the dual-slot
// unroll). Residual is the loop preheader placement: the `for` form floats the
// invariant `lea` preheader above the i<count guard + spills `this`; the
// `do-while` form keeps it inside but inverts the exit-block order. Neither
// MSVC5 spelling reaches both at once - see docs/patterns/loop-preheader-vs-exit-block-order.md.
RVA(0x00093170, 0x1e3)
i32 CGruntzMgr::SyncOptionsState() {
    i32 matched = 0;
    CString s;
    if (s.LoadString(0x81ab)) {
        bool eq;
        eq = (strcmp((const char*)s, (const char*)m_strWorldFile) == 0);
        if (eq) {
            matched = 1;
        }
    }
    srand((u32)time(0));
    g_6455fc = 0;

    i32 idx = 0;
    OptionsTickSub* tick = &((OptionsSlot*)m_options)->m_38;
    i32* cfgp = &((OptionsSlot*)m_options)->m_10;
    i32* arm = &((OptionsSlot*)m_options)->m_14;
    for (i32 i = 0; i < m_optionsCount; i++) {
        i32 cfg;
        if (idx == g_644c54) {
            *arm = 1;
            cfg = *cfgp;
            if (matched) {
                cfg = 0;
            }
            if (!tick->LoadConfig(this, idx, cfg)) {
                return 0;
            }
            tick->Activate();
            arm = (i32*)((char*)arm + 0x238);
            cfgp = (i32*)((char*)cfgp + 0x238);
            idx++;
            tick = (OptionsTickSub*)((char*)tick + 0x238);
            *arm = 0;
            cfg = *cfgp;
            if (matched) {
                cfg = 0;
            }
            if (!tick->LoadConfig(this, idx, cfg)) {
                return 0;
            }
        } else {
            *arm = 0;
            cfg = *cfgp;
            if (matched) {
                cfg = 0;
            }
            if (!tick->LoadConfig(this, idx, cfg)) {
                return 0;
            }
        }
        idx++;
        arm = (i32*)((char*)arm + 0x238);
        cfgp = (i32*)((char*)cfgp + 0x238);
        tick = (OptionsTickSub*)((char*)tick + 0x238);
    }
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::SetCellHeight (0x111ec0; ret 0xc). Writes value into the loaded
// world's height grid: idx = view->m_24[col] + row; view->m_20[idx] = value.
// Then forwards (row, col, value) to the +0x70 notify object (reloc-masked).
RVA(0x00111ec0, 0x37)
void CGruntzMgr::SetCellHeight(i32 row, i32 col, i32 value) {
    CHeightGrid* grid = ((CWorldViewZ*)m_world->m_24)->m_5c;
    i32 idx = grid->m_24[col] + row;
    grid->m_20[idx] = value;
    ((CNotify70*)m_cmdNotify)->Set(row, col, value);
}

// -------------------------------------------------------------------------
// CGruntzMgr::UnknownClose (0x0855e0; vtbl slot 2). The full manager teardown the
// dtor runs: deregister the world's mode-reset callback, flush the live config block
// (each WriteInt names one setting), clear the state stack + delete the live state,
// then tear down + delete every owned sub-object (most a parameterless thiscall +
// operator delete; m_30/m_3c through their own vtable slot 1; m_38 the writer; the
// two engine state singletons), and finally chain the base CGameMgr::UnknownClose and
// drop the registry singleton.
// @early-stop
// big mechanical teardown (~512 named-extern set): the control flow + the WriteInt
// settings block + the per-member delete ladder are reconstructed. The residual is
// the long run of reloc-masked teardown thiscalls (each owned sub-object's Teardown
// is a distinct engine address modeled as one shared EngObj symbol, so those relocs
// stay fuzzy until each is named) - the documented reloc-typing wall, not a code diff.
RVA(0x000855e0, 0x448)
void CGruntzMgr::UnknownClose() {
    if (m_world) {
        ((CWorldRegistrar*)m_world)->RegisterCallback(0);
    }
    OpenSettingsStore();
    CSettingsWriter* cfg = (CSettingsWriter*)m_38;
    if (cfg) {
        cfg->WriteInt("Num_Runs", m_numRuns);
        cfg->WriteInt("Num_Movies", m_numMovies);
        cfg->WriteInt("Sound", m_10);
        cfg->WriteInt("Voice", m_isVoiceEnabled);
        cfg->WriteInt("Ambient", m_isAmbientEnabled);
        cfg->WriteInt("Music", m_14);
        cfg->WriteInt("Interlaced", m_isInterlaced);
        cfg->WriteInt("High_Detail", m_isHighDetail);
        cfg->WriteInt("Effects", m_isEffectsEnabled);
        cfg->WriteInt("Disable_Joystick", g_6455c8);
        if (m_sound) {
            cfg->WriteInt("Music_Volume", m_sound->GetMusicVolume());
        }
        if (m_timer) {
            cfg->WriteInt("Voice_Volume", ((TimerObj*)m_timer)->m_inputMirror);
        }
        if (m_world && m_world->m_28) {
            cfg->WriteInt("Sound_Volume", g_61ab24);
        }
        cfg->WriteInt("Scroll_Speed", m_scrollSpeed);
        cfg->WriteInt("Easy_Mode", m_isEasyMode);
        i32 res = RES_640x480;
        if (m_savedModeW == 0x400 && m_savedModeH == 0x300) {
            res = RES_1024x768;
        } else if (m_savedModeW == 0x320 && m_savedModeH == 0x258) {
            res = RES_800x600;
        }
        cfg->WriteInt("Resolution", res);
        cfg->WriteInt("Checkpoint_Prompts", m_isCheckpointPrompts);
        cfg->WriteInt("Enable_HiColor", m_colorDepth == 0x10 ? 1 : 0);
        cfg->WriteInt("Enable_TrueColor", 0);
    }
    ClearStateStack();
    if (m_curState) {
        delete m_curState;
        m_curState = 0;
    }
    if (m_74) {
        ((EngObj*)m_74)->Teardown();
        operator delete((void*)m_74);
        m_74 = 0;
    }
    if (m_cmdGrid) {
        ((EngObj*)m_cmdGrid)->Teardown();
        operator delete((void*)m_cmdGrid);
        m_cmdGrid = 0;
    }
    if (m_cmdNotify) {
        ((EngObj*)m_cmdNotify)->Teardown();
        operator delete((void*)m_cmdNotify);
        m_cmdNotify = 0;
    }
    if (m_scoreHud) {
        ((EngObj*)m_scoreHud)->Teardown();
        operator delete((void*)m_scoreHud);
        m_scoreHud = 0;
    }
    if (m_cmdSubMgr) {
        ((EngObj*)m_cmdSubMgr)->Teardown();
        operator delete((void*)m_cmdSubMgr);
        m_cmdSubMgr = 0;
    }
    if (g_645578) {
        StateMgr578Z* v = (StateMgr578Z*)g_645578;
        v->m_0 = 0;
        v->m_4 = 0;
        v->m_8 = 0;
        v->m_10 = 0;
        v->m_14 = 0;
        operator delete(v);
        g_645578 = 0;
    }
    if (g_645570) {
        ((EngObj*)g_645570)->Teardown();
        operator delete(g_645570);
        g_645570 = 0;
    }
    if (m_44) {
        ((EngObj*)m_44)->Teardown();
        operator delete((void*)m_44);
        m_44 = 0;
    }
    if (m_sound) {
        ((EngObj*)m_sound)->Teardown();
        operator delete(m_sound);
        m_sound = 0;
    }
    if (m_inputState) {
        ((EngObj*)m_inputState)->Teardown();
        operator delete((void*)m_inputState);
        m_inputState = 0;
    }
    if (m_40) {
        ((EngObj*)m_40)->Teardown();
        operator delete((void*)m_40);
        m_40 = 0;
    }
    if (m_5c) {
        ((EngObj*)m_5c)->Teardown();
        operator delete((void*)m_5c);
        m_5c = 0;
    }
    if (m_timer) {
        ((EngObj*)m_timer)->Teardown();
        operator delete((void*)m_timer);
        m_timer = 0;
    }
    if (m_world) {
        ((CWorldDelete*)m_world)->Slot1(1);
        m_world = 0;
    }
    if (m_34) {
        ((EngObj*)m_34)->Teardown();
        operator delete((void*)m_34);
        m_34 = 0;
    }
    if (m_38) {
        ((EngObj*)m_38)->Teardown();
        operator delete((void*)m_38);
        m_38 = 0;
    }
    if (m_3c) {
        ((CWorldDelete*)m_3c)->Slot1(1);
        m_3c = 0;
    }
    if (m_50) {
        ((EngObj*)m_50)->Teardown();
        operator delete((void*)m_50);
        m_50 = 0;
    }
    if (m_saveSink) {
        ((EngObj*)m_saveSink)->Teardown();
        operator delete((void*)m_saveSink);
        m_saveSink = 0;
    }
    if (m_78) {
        ((EngObj*)m_78)->Teardown();
        operator delete((void*)m_78);
        m_78 = 0;
    }
    CloseSettingsStore();
    if (m_lobby) {
        m_lobby->Release();
        m_lobby = 0;
    }
    if (m_connSettings) {
        operator delete(m_connSettings);
        m_connSettings = 0;
    }
    this->CGameMgr::UnknownClose();
    g_gameReg = 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::AccrueScoreTime (0x0861e0; the timeGetTime-wrapping per-state HUD
// time/score accrual). When the level state (m_134) is "active" (1) it refreshes the
// HUD only if the grid's scored flag is set, then pushes state 0xa. Otherwise it
// refreshes the registry HUD with the live tally id and, for the "won" state (3),
// folds the clamped 64-bit level-clock delta (g_645588 - clock->m_38) into the HUD
// total; for any other state it folds the live timeGetTime() delta. Each finishes by
// pushing state 0x12.
RVA(0x000861e0, 0xc5)
void CGruntzMgr::AccrueScoreTime() {
    CState* st = m_curState;
    if (m_134 == 1) {
        if (((CmdGridFlagView*)m_cmdGrid)->m_288 == 1) {
            UpdateScoreHud();
        }
        SwitchModeState(0xa, 1, 0, 0);
        return;
    }
    ((GameRegHudView*)g_gameReg)->m_7c->Refresh(((StateScoreView*)st)->m_1c);
    if (m_134 == 3) {
        LevelClock* clk = ((StateScoreView*)st)->m_3f4;
        i64 d = (i64)g_645588 - clk->m_38;
        ((GameRegHudView*)g_gameReg)->m_7c->m_10 += (d < 0) ? 0 : (i32)d;
        SwitchModeState(0x12, 1, 0, 0);
        return;
    }
    RegScoreHud* hud = ((GameRegHudView*)g_gameReg)->m_7c;
    u32 now = g_pTimeGetTime();
    hud->m_10 += (now - g_648ce8);
    SwitchModeState(0x12, 1, 0, 0);
}

// -------------------------------------------------------------------------
// CGruntzMgr::OnCheckpointReached (0x08e6c0; /GX EH; the SendMessageA-wrapping
// checkpoint prompt). When checkpoint prompts are enabled (m_isCheckpointPrompts), pops a modal
// dialog and - if it returns 1 ("yes") - SendMessageA WM_COMMAND 0x80cf to the game
// window. The destructible dialog local gives the /GX frame.
RVA(0x0008e6c0, 0x85)
void CGruntzMgr::OnCheckpointReached() {
    if (m_isCheckpointPrompts == 0) {
        return;
    }
    CCheckpointDlg dlg(0);
    if (ExitModalUI((CModalDialog*)&dlg, 0) == 1) {
        g_pSendMessageA((i32)((CGameWnd*)m_4)->m_hwnd, 0x111, 0x80cf, 0);
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::DelayedQuit (0x08f530; the PostMessageA-wrapping delayed shutdown).
// One-shot (guarded by m_a4): resolve the world's "MENU_ACTIVATE" menu node to a base
// timestamp (its +0x10->+0x28 plus 0x1f4), busy-wait via timeGetTime until that
// deadline passes, clear the app's resume flag (m_244), then PostMessageA WM_CLOSE to
// the game window.
// @early-stop
// zero-register-pinning wall (~3.5%): logic + control flow are structurally byte-for-
// byte (verified via llvm-objdump). Retail keeps `this` in ebx and tests each null via
// `mov;test` (no 0 constant); our MSVC5 materializes 0 in ebx (for the `out = 0` init
// store + the four null compares) and so spills `this` into ebp + an extra callee-saved
// push - every this-relative modrm then differs. No source spelling flips MSVC's
// zero-register pick here (docs/patterns/zero-register-pinning.md, regalloc family).
RVA(0x0008f530, 0xbd)
void CGruntzMgr::DelayedQuit() {
    if (m_a4 != 0) {
        return;
    }
    m_a4 = 1;
    void* out = 0;
    ((CWorldMenuHolder*)m_world->m_28)->m_10.Lookup("MENU_ACTIVATE", &out);
    i32 base;
    if (out != 0) {
        ((CWorldMenuHolder*)m_world->m_28)->m_10.Lookup("MENU_ACTIVATE", &out);
        base = ((CMenuNode*)out)->m_10->m_28 + 0x1f4;
    } else {
        base = 0;
    }
    u32(WINAPI * tgt)() = g_pTimeGetTime;
    u32 deadline = base + tgt();
    while (tgt() < deadline) {
    }
    if (m_8) {
        ((CGameApp*)m_8)->m_244 = 0;
    }
    if (m_4) {
        g_pPostMessageA((i32)((CGameWnd*)m_4)->m_hwnd, 0x10, 0, 0);
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::RunModalDialog (0x090260; ret 0xc). The DialogBoxParamA-driven modal
// runner (the ExitModalUI sibling): bails (0) on a null template/proc, quiesces the
// game (stop timer, flush +0x68, notify-exit + dispatch the world when `flag`), forces
// the cursor visible, runs DialogBoxParamA(hInstance, tmpl, hwnd, dlgProc, 0) under the
// busy gate, then optionally polls the live state, restores the cursor, runs the per-
// frame tick, and finalizes the picked play/paused state. Returns the dialog result.
// @early-stop
// regalloc free-list-pick wall (docs/patterns/select-zero-mask-dest-register.md):
// body byte-exact up to the world-dispatch `*m_world->m_1c; ->vtbl->Slot0a(d)`,
// where retail seeds the container in ecx + vtbl in edx and our cl picks edx +
// ecx; the swap is a global 3-cycle {eax->edx->ecx} that re-colours the whole
// downstream (incl. the DialogBoxParamA arg setup) - same instructions, rotated
// registers. Not source-steerable (~99.4%).
RVA(0x00090260, 0x13e)
i32 CGruntzMgr::RunModalDialog(const char* tmpl, void* dlgProc, i32 flag) {
    if (tmpl == 0) {
        return 0;
    }
    if (dlgProc == 0) {
        return 0;
    }
    if (m_timer) {
        ((TimerObj*)m_timer)->Stop();
    }
    if (m_cmdGrid && m_10) {
        ((Ctrl68*)m_cmdGrid)->Flush();
    }
    if (m_world) {
        if (flag && m_curState && m_curState->Update() != 5) {
            m_curState->NotifyExit(0x32);
        } else {
            flag = 0;
        }
        CWorldDispatch* d = *m_world->m_1c;
        d->Slot0a();
    }

    i32(WINAPI * show)(i32) = g_pShowCursor;
    i32 shown = show(1);
    while (show(1) < 0) {
    }

    m_modalBusy = 1;
    i32 result = g_pDialogBoxParamA(
        (i32)((CGameApp*)m_8)->m_c,
        tmpl,
        (i32)((CGameWnd*)m_4)->m_hwnd,
        dlgProc,
        0
    );
    g_64557c = 0;
    m_modalBusy = 0;
    if (m_curState && flag) {
        m_curState->Vslot06();
    }
    if (shown <= 0) {
        while (show(0) >= 0) {
        }
    }

    PerFrameTick();
    CState* o = PickPausedThenPlayState();
    if (o) {
        if (((CActiveObj*)o)->m_2dc) {
            ((CActiveObj*)o)->m_2dc->Release();
        }
        ((CActiveObj*)o)->Finalize();
    }
    return result;
}

// -------------------------------------------------------------------------
// CGruntzMgr::LoadSaveMessageSprite (0x092420; /GX EH; ret 1). The save-feedback
// path Quicksave falls into: when the first-frame guard (m_44->m_124) is set it pops a
// localized message (resource 0x81aa) through a modal; otherwise it runs the GAME_SAVE
// dialog and, on success, the GAME_SAVEMSG dialog. Returns 1.
RVA(0x00092420, 0xa4)
i32 CGruntzMgr::LoadSaveMessageSprite() {
    if (((HudGuard44*)m_44)->m_124 != 0) {
        CString name;
        name.LoadStringA(0x81aa);
        EnterModalUI((i32)(const char*)name);
    } else if (RunModalDialog("GAME_SAVE", (void*)GruntzSaveGameDlgProc, 0) == 1) {
        RunModalDialog("GAME_SAVEMSG", (void*)GruntzSaveMsgDlgProc, 0);
    }
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::SaveGameAs (0x092f00; /GX EH; ret 1/0). The save-as name dialog: when
// the live state is playable (Update() in {5,2,3,7}), reset the sound channels, pop the
// name dialog, and - if accepted - capture the entered name into m_strWorldFile (prefixed
// "custom\" and flag m_128=0 when the dialog's custom flag is set, else the raw name with
// m_128=1). If the resulting name is non-empty, PostMessageA WM_COMMAND 0x80e3. The dialog
// (with its CString member) is the compound /GX frame's two destructibles.
RVA(0x00092f00, 0x1ef)
i32 CGruntzMgr::SaveGameAs() {
    CSaveNameDlg dlg(this, 0);
    i32 st = m_curState->Update();
    if (st != 5 && st != 2 && st != 3 && st != 7) {
        return 0;
    }
    ChannelSlots_InitAll();
    if (ExitModalUI((CModalDialog*)&dlg, 1) != 1) {
        return 0;
    }
    if (dlg.m_68 != 0) {
        m_128 = 0;
        m_strWorldFile = "custom\\" + dlg.m_6c;
    } else {
        m_128 = 1;
        m_strWorldFile = dlg.m_6c;
    }
    if (m_strWorldFile.GetLength() == 0) {
        return 0;
    }
    g_pPostMessageA((i32)((CGameWnd*)m_4)->m_hwnd, 0x111, 0x80e3, 0);
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::CGruntzMgr (0x083030; /GX EH; returns this). Constructs the base
// CGameMgr, then the four destructible members in declaration order (m_strWorldFile
// CString state 0, m_stateStack CByteArray state 1, m_strEC CString state 2,
// m_strMoviePath CString state 3, the 4-slot m_options array via __ehvec_ctor state
// 4), stamps the derived vftable, and seeds the flat scalar field block. The field
// stores are written in retail's exact (non-offset-sorted) source order so the
// compiler's edi(=0)/ebx(=1) constant reuse and store schedule fall out byte-exact.
RVA(0x00083030, 0x1b6)
CGruntzMgr::CGruntzMgr() {
    m_curState = 0;
    m_world = 0;
    m_34 = 0;
    m_38 = 0;
    m_scoreHud = 0;
    m_3c = 0;
    m_40 = 0;
    m_44 = 0;
    m_sound = 0;
    m_4c = 0;
    m_50 = 0;
    m_64 = 0;
    m_lobby = 0;
    m_inputState = 0;
    m_saveSink = 0;
    m_5c = 0;
    m_timer = 0;
    m_cmdGrid = 0;
    m_cmdSubMgr = 0;
    m_cmdNotify = 0;
    m_74 = 0;
    m_78 = 0;
    m_lobbyResult = 0;
    m_lobbyProbed = 0;
    m_a4 = 0;
    m_a8 = 0;
    m_modalBusy = 0;
    m_b0 = 0;
    m_b4 = 0;
    m_114 = 0;
    m_isCheckpointPrompts = 1;
    m_connSettings = 0;
    m_saveInfoRec = 0;
    m_numRuns = 0;
    m_numMovies = 0;
    m_cc = 0x1e;
    m_modeW = 0;
    m_modeH = 0;
    m_colorDepth = 0x10;
    m_f4 = 1;
    m_f8 = 0;
    m_fc = 0;
    m_14 = 1;
    m_10 = 1;
    m_isVoiceEnabled = 1;
    m_isAmbientEnabled = 1;
    m_isInterlaced = 0;
    m_isEasyMode = 0;
    m_130 = 0;
    m_128 = 0;
    m_12c = 0;
    m_134 = 0;
    m_isHighDetail = 1;
    m_isEffectsEnabled = 1;
    m_optionsCount = 3;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ScalarDeletingDtor (0x083330; ??_G; ret 4). Run the dtor body, then
// (when the hidden low flag bit is set) operator delete this; return this.
RVA(0x00083330, 0x1e)
void* CGruntzMgr::ScalarDeletingDtor(u32 flags) {
    this->CGruntzMgr::~CGruntzMgr(); // qualified -> direct (non-virtual) dtor call
    if (flags & 1) {
        operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// CheckDisplayBoundsA/B (0x08e1d0 / 0x08e2b0). When the game is in PLAY/PAUSED
// (m_curState->Update() == 3 || 0x11), resolve a screen point through the world
// coord-resolver (m_world->m_1c); if it falls off the expected field, force the
// display back to 640x480 and pop the matching ReportError. The two variants
// differ only in the resolver entry + the in/out-of-bounds test + the error id.
//
// The resolver result is filled into a caller `pt` and returned (the function
// returns the same out-pointer in eax). m_world->m_1c is reached as a distinct
// coord-resolver view of the world's +0x1c sub-object (a reinterpret of the
// shared CWorldDispatch** member). 0x143510/0x143590 + SetVideoMode are out-of-
// line / reloc-masked; ReportError (0x08dc60) is the matched sibling.
struct CPointXY {
    i32 x;
    i32 y;
};
struct CWorldCoordResolver {
    CPointXY* ResolveHi(CPointXY* out, i32 a, i32 b, i32 c); // 0x143510
    CPointXY* ResolveLo(CPointXY* out, i32 a, i32 b, i32 c); // 0x143590
};

// @early-stop
// reloc-masked plateau (~97%): code bytes exact; the only residual is the
// call-rel32 operands to the unmatched engine callees ResolveHi (0x143510) and
// SetVideoMode (0x8df00) - they pair when those siblings get named.
RVA(0x0008e1d0, 0xa5)
i32 CGruntzMgr::CheckDisplayBoundsA() {
    if (m_curState->Update() != 3 && m_curState->Update() != 0x11) {
        return 1;
    }
    CPointXY pt;
    CPointXY* p =
        ((CWorldCoordResolver*)m_world->m_1c)->ResolveHi(&pt, m_modeW, m_modeH, m_colorDepth);
    i32 x = p->x;
    i32 y = p->y;
    if (x > 0x514 || x == -1 || y == -1) {
        return 1;
    }
    if (SetVideoMode(x, y, 1)) {
        return 1;
    }
    if (SetVideoMode(0x280, 0x1e0, 1)) {
        return 1;
    }
    ReportError(0x8008, 0x439);
    return 0;
}

// @early-stop
// reloc-masked plateau (~97%): code bytes exact; residual is the call-rel32
// operands to unmatched engine callees ResolveLo (0x143590) + SetVideoMode
// (0x8df00).
RVA(0x0008e2b0, 0xb1)
i32 CGruntzMgr::CheckDisplayBoundsB() {
    if (m_curState->Update() != 3 && m_curState->Update() != 0x11) {
        return 1;
    }
    CPointXY pt;
    CPointXY* p =
        ((CWorldCoordResolver*)m_world->m_1c)->ResolveLo(&pt, m_modeW, m_modeH, m_colorDepth);
    i32 x = p->x;
    i32 y = p->y;
    if (x == -1 || y == -1 || x < 0x140 || y < 0xc8) {
        return 1;
    }
    if (SetVideoMode(x, y, 1)) {
        return 1;
    }
    if (SetVideoMode(0x280, 0x1e0, 1)) {
        return 1;
    }
    ReportError(0x8008, 0x43a);
    return 0;
}

// ---------------------------------------------------------------------------
// CGruntzMgr::SetVideoMode (0x08df00; ret 0xc). Switch the display to (w,h) at
// the current bit depth (m_colorDepth). No-op if already at (w,h). When the live state is
// playable (Update() in {3,0x11}) and the new size exceeds the loaded map's
// playable field (m_world->m_24->m_5c->{m_30,m_34}), it refuses: pokes the HUD
// guts subsystem (m_2dc) and surfaces the "map too small" modal, returning 0.
// Otherwise it applies the mode through the engine, re-hides the cursor, stamps
// m_modeW/m_modeH (+ the saved pair when arg3 is set), re-pokes the guts, runs the
// two post-mode resync siblings, and (once) logs "Resolution is now %ix%ix%i".
//
// The SetVideoMode symbol pairs the @early-stop CheckDisplayBoundsA/B and the
// RestoreVideoMode/CheckSavedMode call sites (previously the Boundary_08df00 stub).
struct SvmField { // m_world->m_24->m_5c: the loaded map's playable extent
    char p0[0x30];
    i32 m_30, m_34; // +0x30/+0x34  field width/height limits
};
struct SvmWorldView { // m_world->m_24
    char p0[0x5c];
    SvmField* m_5c; // +0x5c
};
struct SvmGuts { // m_curState->m_2dc: the HUD/guts subsystem
    i32 m_0;     // +0x00  state (0/1 -> which poke order)
    char p4[0x614 - 4];
    i32 m_614;    // +0x614  receives the mode height
    void CallA(); // 0x2b8a (thiscall) reloc-masked
    void CallB(); // 0x2d5b (thiscall) reloc-masked
};
struct SvmStateView { // a view of m_curState (the live play state)
    char p0[0x2dc];
    SvmGuts* m_2dc; // +0x2dc
    void Prep();    // 0x3d55 (thiscall) reloc-masked
};
// The engine display-mode apply (0x155f60, __stdcall(w,h,depth) -> nonzero ok).
extern "C" i32 __stdcall SvmApply(i32 w, i32 h, i32 depth);

// @early-stop
// regalloc wall (~92%): the control flow, every branch/call, the two play-state
// Update() pairs, the guts (m_2dc) poke order, the ShowCursor hide loop and the
// "Resolution is now" log are byte-for-byte the same SHAPE. The residual is a
// pure register-allocation tiebreak: retail dedicates ebp (the 4th callee-saved
// reg) to `w` and keeps m_curState in edi separately, while MSVC5 here coalesces
// `w` into edi (reusing it for m_curState once w is dead) -> 3 saved regs not 4,
// so `sub esp,0x70` vs retail's 0x80 and every later [esp+N] sits -4. Logic is
// complete + correct; defining this symbol also pairs the SetVideoMode call in
// CheckSavedMode (-> 100%) + CheckDisplayBoundsA/B. See pin-local-for-callee-saved-reg.md.
RVA(0x0008df00, 0x238)
i32 CGruntzMgr::SetVideoMode(i32 w, i32 h, i32 flag) {
    if (w == m_modeW && h == m_modeH) {
        return 1;
    }
    if (m_world == 0) {
        return 0;
    }
    if (m_curState->Update() == 3 || m_curState->Update() == 0x11) {
        if (m_world->m_24 != 0) {
            SvmField* f = ((SvmWorldView*)m_world->m_24)->m_5c;
            if (f != 0) {
                if (w > f->m_30 || h > f->m_34) {
                    SvmStateView* st = (SvmStateView*)m_curState;
                    st->Prep();
                    if (st->m_2dc != 0) {
                        st->m_2dc->m_614 = m_modeH;
                        if (st->m_2dc->m_0 == 0) {
                            st->m_2dc->CallA();
                            st->m_2dc->CallB();
                            ReportMapTooSmall(
                                "This map is too small to be displayed under your "
                                "desired video resolution. Default resolution will "
                                "be used."
                            );
                            return 0;
                        }
                        if (st->m_2dc->m_0 == 1) {
                            st->m_2dc->CallB();
                            st->m_2dc->CallA();
                        }
                    }
                    ReportMapTooSmall(
                        "This map is too small to be displayed under your desired "
                        "video resolution. Default resolution will be used."
                    );
                    return 0;
                }
            }
        }
    }
    if (!SvmApply(w, h, m_colorDepth)) {
        return 0;
    }
    while (g_pShowCursor(0) >= 0) {
    }
    m_modeW = w;
    m_modeH = h;
    if (m_curState->Update() == 3 || m_curState->Update() == 0x11) {
        if (flag) {
            m_savedModeW = w;
            m_savedModeH = h;
        }
        SvmStateView* st = (SvmStateView*)m_curState;
        st->Prep();
        if (st->m_2dc != 0) {
            st->m_2dc->m_614 = h;
            if (st->m_2dc->m_0 == 0) {
                st->m_2dc->CallA();
                st->m_2dc->CallB();
            } else if (st->m_2dc->m_0 == 1) {
                st->m_2dc->CallB();
                st->m_2dc->CallA();
            }
        }
    }
    Step1db6();
    Step3d23();
    if (g_645600 != 0) {
        g_645600 = 0;
        char buf[0x70];
        sprintf(buf, "Resolution is now %ix%ix%i", m_modeW, m_modeH, m_colorDepth);
        LogLine(buf);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzMgr::IsBattlezMapFile(CString path) @0x093be0 (/GX, ret 4)
// Open the file at `path`; if it carries a full WWD header (>= 0x5f4 bytes), read
// the header and report whether the literal "Battlez" appears past the 0x10-byte
// magic. `this` is unused. The path CString is taken by value (callee destroys it),
// so cl emits the /GX frame: the destructible stack file reader + the arg CString
// each unwind at their own trylevel.
// ---------------------------------------------------------------------------
// The stack-local WWD file reader is the engine CFileIO (include/Io/FileStream.h);
// its virtual dtor makes the local destructible -> /GX frame.

// The strstr-class substring helper (0x120090); nonzero when `needle` occurs.
extern "C" i32 SubstringMatch(const char* haystack, const char* needle);

// @early-stop
// zero-register-pinning + /GX EH-frame wall (docs/patterns/zero-register-pinning.md):
// the file open/read/SubstringMatch logic, the >= 0x5f4 header gate, the "Battlez"
// constant and the by-value CString-arg teardown all pair byte-faithfully. Residue:
// retail pins `0` in ebx (xor ebx,ebx + push ebx, reusing %bl for the /GX trylevel
// byte writes), which shifts every stack local up 4 (CFile at [esp+4] not [esp+0]);
// our cl emits immediate-0 trylevel writes + no ebx save. No source lever forces the
// pin under /O2; also the EH scope-table cookie (Unwind vs $L) is not steerable.
RVA(0x00093be0, 0x107)
i32 CGruntzMgr::IsBattlezMapFile(CString path) {
    CFileIO file;
    if (file.Open(path, 0, 0)) {
        if (file.GetLength() >= 0x5f4) {
            char hdr[0x5f4];
            file.Read(hdr, 0x5f4);
            file.Close();
            if (SubstringMatch(hdr + 0x10, "Battlez")) {
                return 1;
            }
        } else {
            file.Close();
        }
    }
    return 0;
}

// size 0xa30 recovered from operator-new sites (gruntz.analysis.news)

SIZE_UNKNOWN(CActiveObj);
SIZE_UNKNOWN(CActiveSub2dc);
SIZE_UNKNOWN(CChatLog);
SIZE_UNKNOWN(CColorLookup);
SIZE_UNKNOWN(CColorRow);
SIZE_UNKNOWN(CHeightGrid);
SIZE_UNKNOWN(CInput54);
SIZE_UNKNOWN(CLevelState);
SIZE_UNKNOWN(CWorldMenuMap);
SIZE_UNKNOWN(CMenuNode);
SIZE_UNKNOWN(CMenuNodeSub);
SIZE_UNKNOWN(CModalDialog);
SIZE_UNKNOWN(CMonoCellArray);
SIZE_UNKNOWN(CMonoConfigHolder);
SIZE_UNKNOWN(CMonoConfigMap);
SIZE_UNKNOWN(CMonoConfigRec);
SIZE_UNKNOWN(CMonoEntry);
SIZE_UNKNOWN(CMonoSprite);
SIZE_UNKNOWN(CMonoView);
SIZE_UNKNOWN(CMonoWorld);
SIZE_UNKNOWN(CNotify70);
SIZE_UNKNOWN(CObListSub);
SIZE_UNKNOWN(CPlayStateView);
SIZE_UNKNOWN(CPointXY);
SIZE_UNKNOWN(CRezSurface94);
SIZE_UNKNOWN(CSaveDlgBase);
SIZE_UNKNOWN(CSaveNameDlg);
SIZE_UNKNOWN(CScrollView);
SIZE_UNKNOWN(CSerializerZ);
SIZE_UNKNOWN(CSettingsWriter);
SIZE_UNKNOWN(CWorldCoordResolver);
SIZE_UNKNOWN(CWorldDelete);
SIZE_UNKNOWN(CWorldEdges);
SIZE_UNKNOWN(CWorldLookupHolder);
SIZE_UNKNOWN(CWorldLookupView);
SIZE_UNKNOWN(CWorldMenuHolder);
SIZE_UNKNOWN(CWorldModeIface);
SIZE_UNKNOWN(CWorldModeView);
SIZE_UNKNOWN(CWorldRegistrar);
SIZE_UNKNOWN(CWorldStatusView);
SIZE_UNKNOWN(CWorldViewZ);
SIZE_UNKNOWN(CmdGridFlagView);
SIZE_UNKNOWN(CmdSink);
SIZE_UNKNOWN(CmdSinkV);
SIZE_UNKNOWN(CmdTimer60);
SIZE_UNKNOWN(Ctrl68);
SIZE_UNKNOWN(DirectInputMgr2);
SIZE_UNKNOWN(EngObj);
SIZE_UNKNOWN(GameRegHudView);
SIZE_UNKNOWN(HudGuard44);
SIZE_UNKNOWN(InputState54);
SIZE_UNKNOWN(InputStateObj);
SIZE_UNKNOWN(LevelClock);
SIZE_UNKNOWN(OptionsSlot);
SIZE_UNKNOWN(OptionsTickSub);
SIZE_UNKNOWN(PlayStatusSlot);
SIZE_UNKNOWN(RegScoreHud);
SIZE_UNKNOWN(SaveInfo);
SIZE_UNKNOWN(SaveSink58);
SIZE_UNKNOWN(ScoreHud);
SIZE_UNKNOWN(ScoreNotifier);
SIZE_UNKNOWN(ScoreSub2c);
SIZE_UNKNOWN(StateMgr578Z);
SIZE_UNKNOWN(StateMgrBZ);
SIZE_UNKNOWN(StateScoreView);
SIZE_UNKNOWN(SvmField);
SIZE_UNKNOWN(SvmGuts);
SIZE_UNKNOWN(SvmStateView);
SIZE_UNKNOWN(SvmWorldView);
SIZE_UNKNOWN(TimerObj);
SIZE_UNKNOWN(WorldDeltaTables);
SIZE_UNKNOWN(WorldLayer);
SIZE_UNKNOWN(WorldLayerView);
SIZE_UNKNOWN(CGameRegistry);
