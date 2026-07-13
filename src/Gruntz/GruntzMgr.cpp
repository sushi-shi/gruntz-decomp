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
//   CGruntzMgr::~CGruntzMgr        - virtual dtor: runs Close() then the
//       compiler-generated member/base teardown under the /GX C++ EH frame.
//   the scalar-deleting destructor (vtable slot 0) is auto-emitted by MSVC.
//
// The ctor is the member-construction-heavy 0x083030 (deferred to the stub).
// <Mfc.h> brings <windows.h> KERNEL32 (GetCurrentDirectoryA; DWORD) and the central
// WINMM timeGetTime decl (the per-frame draw clock).
#include <Mfc.h>
// The REAL MFC CDialog (ExitModalUI's argument - proof in <Gruntz/GruntzMgr.h>). afx.h
// arrives first via <Mfc.h>, so there is no windows.h-first C1189 here. The afxwin*.inl
// bodies are skipped for the clang LABEL step only (they carry an implicit-int
// CMenu::operator== that clang rejects and wine cl accepts); without this the label pass
// emits NO IR and the entire TU silently vanishes from symbol_names.csv.
// See docs/patterns/afxwin-clang-label-step-skip-inl.md.
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <new>
#include <Gruntz/LeafCue.h>
#include <Gruntz/SpriteFactory.h> // CSpriteFactory/CSpriteListNode (m_world->m_8 live-object list)
#include <Gruntz/UserLogic.h> // CGameObject (the scanned live objects: m_screenX/Y, m_collCategory)
#include <Image/ImageFrame.h> // CImageFrame/CImageFormat (the "Gruntz" set's frames the cheats read)
#include <Gruntz/BoundaryUpperViews.h>
#include <DDrawMgr/DirectDrawMgr.h> // CDirectDrawMgr::FindFwd/FindBack (display-mode pool)
#include <Io/SaveGame.h>
#include <Gruntz/Play.h>
#include <Gruntz/Demo.h> // canonical CDemo (the CPlay-derived demo state; its dtor lives in this obj)
#include <Gruntz/Attract.h>   // canonical CAttract (was a reduced local ODR-landmine twin)
#include <Gruntz/HelpState.h> // canonical CHelpState (same; extracted out of HelpState.cpp)
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzPlayer.h> // GruntzPlayer::Reset (0xda9e0) - the options slots ARE GruntzPlayer
#include <Gruntz/BattlezData.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/BattlezMapConfig.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Viewport.h>      // the shared world-plane object (was local CWorldLayer)
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h>     // the ONE CTriggerMgr (m_cmdGrid; was the CCmdGrid view)
#include <Gruntz/SpriteRefTable.h> // CSpriteRefTable (m_spriteFactory @+0x74; Reset teardown)
#include <Gruntz/LightFxMgr.h>     // CLightFxMgr (m_logicPump @+0x78; Reset teardown @0x9dc80)
#include <Gruntz/WorldSoundSet.h>  // CWorldSoundSet (m_inputState @+0x54; the +0x54 sound object)
#include <Gruntz/FontConfig.h>     // CFontConfig (m_chatLog @+0x5c; AddItem @0x21c60)
#include <Gruntz/Enums.h>
#include <Io/FileStream.h> // CFileIO (the engine file reader IsBattlezMapFile opens)
#include <dplobby.h>       // real DirectPlay lobby SDK: IDirectPlayLobby + DirectPlayLobbyCreate.
// Brings the full windows.h/objbase.h COM chain (STDMETHOD_/HRESULT/CLSID)
// - so the old hand-rolled COM mirror is not needed here (it starved cguid.h of CLSID);
// Mfc.h already provides the STDMETHODs the CWorldDispatch view uses.
#include <rva.h>
#include <stdio.h>                // engine sprintf (reloc-masked) for the toggle-message formatter
#include <string.h>               // engine strstr (reloc-masked) for the Battlez header probe
#include <Utils/RegistryHelper.h> // Utils::RegistryHelper (the settings/registry writer)
#include <Gruntz/GruntzCmdMgr.h>  // CGruntzCmdMgr - the REAL +0x6c sub-manager (~ @0x85bd0)
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
    // Non-virtual pre-level reset (@0x08dd80, reached via ILT thunk 0x1406); the
    // asset-key register step calls it twice on the world dispatch object.
    void Prepare(); // 0x08dd80
};

// CGruntzMgr::m_lobby is the REAL DirectPlay lobby COM interface (IDirectPlayLobby,
// from the vendored <dplobby.h>); only Release (slot 2) + GetConnectionSettings
// (slot 8, called twice in the size-probe / fill idiom) are dispatched. The former
// hand-rolled IDirectPlayLobbyZ view is gone: the real interface's slots line up
// exactly (v01=AddRef, v03-v07=Connect/CreateAddress/EnumAddress/EnumAddressTypes/
// EnumLocalApplications, slot 8=GetConnectionSettings), so dispatch is byte-identical.

// The free CD-ROM / file-probe helpers CGruntzMgr calls (plain file-scope free
// functions; reloc-masked). GetGruntzDriveLetter (0x1ffe0, WinAPICdRom.cpp) collides
// by name with the CGruntzMgr member below, so its call site qualifies with `::`.
char GetGruntzDriveLetter();  // 0x1ffe0 (WinAPICdRom.cpp)
i32 FileExists(char* szPath); // 0x1189c0 (HeapDiag.cpp)

// DirectPlayLobbyCreate (DPLAYX ordinal 4) now comes from the real <dplobby.h>
// (macro -> DirectPlayLobbyCreateA); reached as [IAT] - the displacement reloc-masks,
// so only the arg shapes are load-bearing. CNetMgr::ReportError is a `call rel32`.
// `m_connSettings` (the connection-settings buffer) is freed through a small
// NAFXCW-style operator-delete helper (out-of-line, reloc-masked); a plain free fn.

class CNetMgr {
public:
    static void ReportError(char* file, i32 line, i32 hr, void* hWnd);
};

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

// Game-clock/registry globals reached by AccrueScoreTime / Close.
extern "C" {
    DATA(0x00248ce8)
    extern i32 g_648ce8; // DAT_00648ce8  (timeGetTime base stamp)
}

// AccrueScoreTime's engine views. g_gameReg->m_7c is the registry's HUD/score
// accumulator (Refresh at the 0x1884 thunk; a running total at +0x10). The live
// state carries a tally id at +0x1c and a 64-bit level clock pointer at +0x3f4
// (->m_38). m_cmdGrid carries a "scored" flag at +0x288.
struct GameRegHudView {
    char m_pad0[0x7c];
    CBattlezData* m_7c; // +0x7c
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
// (m_cmdGrid's scored flag +0x288 is CTriggerMgr::m_288.)

// DelayedQuit's menu lookup goes through the world's CSndHost (+0x28): the former
// CWorldMenuMap/CMenuNode/CMenuNodeSub/CWorldMenuHolder views were the SoundCue.h
// canonicals (FUN_005b8438 == RVA 0x1b8438 == CSndFinder::Lookup; the "menu node"
// is the LeafCue whose m_10 CSoundCueMgr carries the +0x28 cue duration).

// Close's teardown vocabulary. Most owned sub-objects share a parameterless
// thiscall teardown then operator delete (modeled as one EngObj type - the per-call
// displacement reloc-masks). m_30/m_3c are torn down through their own vtable slot 1
// (a flagged scalar-delete), m_settings is the settings/registry writer (WriteInt per
// named key), and g_645578 is zeroed field-by-field before delete. Each engine
// entrypoint is out-of-line / reloc-masked.
struct EngObj {
    // Teardown @0x3b1b IS ~CTriggerMgr; cast at each call.
};
class CWorldDelete {
public:
    virtual void s0();            // slot 0 (+0x00)
    virtual void Slot1(i32 flag); // slot 1 (+0x04) flagged scalar-delete
};
struct CSettingsWriter {
    // WriteInt IS Utils::RegistryHelper::SetValueDword; cast at each call.
    // Teardown @0x3b1b IS ~CTriggerMgr; cast at the call.
};
// The settings store open/close brackets around the WriteInt block (reloc-masked
// __cdecl free fns; no this).
void OpenSettingsStore();  // FUN_005158f0
void CloseSettingsStore(); // FUN_004f8e20

// The modal dialog OnCheckpointReached pops: a destructible stack local built by
// FUN_004234a0(this, 0) and torn down by FUN_005ba51d, handed to ExitModalUI as a
// CModalScreen. Its ctor/dtor reloc-mask; only the size + destructibility (the /GX
// frame) are load-bearing.
// It IS a real MFC CDialog subclass: the "dtor" the old shell declared at 0x1ba51d is
// CDialog::~CDialog itself, and its 0x5c body is exactly sizeof(CDialog). Deriving from the
// real base binds that teardown to ??1CDialog@@UAE@XZ in NAFXCW.LIB (it used to be an
// UNBOUND ??1CCheckpointDlg) and lets it reach ExitModalUI CAST-FREE. The dtor stays
// IMPLICIT so retail's inlined stack-local teardown still falls out.
class CCheckpointDlg : public CDialog {
public:
    CCheckpointDlg(CWnd* a); // 0x234a0  ??0CCheckpointDlg@@QAE@PAVCWnd@@@Z
};

// The save-as name dialog SaveGameAs pops IS a CBattlezDlg (proven: its ctor @0x14b30
// == ??0CBattlezDlg@@QAE@HPAVCWnd@@@Z, and its +0x68 custom-name flag + +0x6c CString
// name match <Gruntz/Dialogs.h>'s CBattlezDlg exactly). It is modeled LOCALLY here
// (not via the canonical Dialogs.h) for ONE codegen reason: retail INLINES this local
// object's destruction (~CString m_6c + CDialog::~CDialog 0x1ba51d, no vptr restamp),
// whereas Dialogs.h's OUT-OF-LINE ~CBattlezDlg (0x14c90, needed for the vtable slot)
// would force a `call ??1CBattlezDlg` and diverge SaveGameAs's /GX frame. So this TU
// keeps an implicit-dtor twin - a per-TU dtor-emission split, not a conflation. The
// ctor is declared-only, so its call reloc-binds to the real 0x14b30; CSaveDlgBase
// stands in for CDialog's reloc-masked virtual dtor (0x1ba51d).
// RESOLVED: the base is the REAL MFC CDialog (0x5c) now. The old CSaveDlgBase shell -
// whose ??1CSaveDlgBase@@UAE@XZ was a PHANTOM that no obj and no .LIB could ever define -
// is deleted, and the same reloc-masked dtor call binds to ??1CDialog@@UAE@XZ in
// NAFXCW.LIB (0x1ba51d). BOTH "measured blockers" recorded here were wrong: the C2011 was
// OUR OWN class named CModalDialog being macro-rewritten by afxwin.h's
// `#define CModalDialog CDialog`, and this TU never included <Gruntz/Dialogs.h> at all.
class CBattlezDlg : public CDialog {
public:
    CBattlezDlg(i32 a0, CWnd* pParent); // 0x14b30  ??0CBattlezDlg@@QAE@HPAVCWnd@@@Z
    // The dtor stays IMPLICIT here - do NOT declare it out-of-line. Retail's SaveGameAs
    // (0x92f00) INLINES the CBattlezDlg teardown at the stack local's scope exit:
    //     lea ecx,[esp+0x78]; call 0x1b9cde   ; ~CString  (the m_6c member)
    //     lea ecx,[esp+0xc];  call 0x1ba51d   ; ~CDialog  (the base)
    // - there is no `call ??1CBattlezDlg` anywhere in it. So cl saw an INLINE (compiler-
    // generated) dtor for this class; the out-of-line 0x14c90 body is just the COMDAT copy
    // emitted where its address is needed (the vtable slot). Declaring it out-of-line here
    // forces a call and desyncs the whole function (measured: SaveGameAs 100% -> broken).
    // ??1CBattlezDlg IS still an ODR-divergent duplicate (dialogs models this class as
    // `: public CDialog`, we model it as `: CSaveDlgBase`) - but the fix for THAT is to
    // unify the class shape, not to change the dtor's linkage. Reported, not bodged.
    char m_pad5c[0x68 - 0x5c];
    i32 m_68;     // +0x68  use-custom-prefix flag (== CBattlezDlg::m_customNameFlag)
    CString m_6c; // +0x6c  entered name
};

// Resets the 17 sound-channel slots (g_64c3f0[17] = 1); SaveGameAs calls it before
// popping the modal. Reloc-masked __cdecl free fn (0xdb1d0).
void ChannelSlots_InitAll(); // 0xdb1d0

// The Win32 dialog procedures handed to RunModalDialog. Each pushed code address is
// the proc's ILT jmp-thunk (retail /INCREMENTAL routes an address-taken function
// through its <0x7c20 thunk), so the DIR32 references bind to the THUNK rva - not the
// proc body. The bodies live in their own TUs (LoadGameMenu / AppDialogs). Modeled as
// extern-C thunk symbols bound to the thunk rvas (the GameObjectFactory _CreateXxx idiom).
// @data-symbol: _GruntzLoadGameDlgProc 0x00002167
// @data-symbol: _GruntzDebugGruntTypeProc 0x000021e9
// @data-symbol: _GruntzSaveGameDlgProc 0x00001041
// @data-symbol: _GruntzSaveMsgDlgProc 0x000011d1
// @data-symbol: _LevelNumberDialogProcThunk 0x00002ab8
extern "C" void GruntzLoadGameDlgProc();    // thunk 0x2167 -> body 0x9dff0 (LoadGameMenu.cpp)
extern "C" void GruntzDebugGruntTypeProc(); // thunk 0x21e9
extern "C" void GruntzSaveGameDlgProc();    // thunk 0x1041 (GAME_SAVE)
extern "C" void GruntzSaveMsgDlgProc();     // thunk 0x11d1 (GAME_SAVEMSG)
// The "go to level" developer dialog proc body (AppDialogs.cpp @0x08e7c0); DebugJumpLevel
// pushes its ILT thunk 0x2ab8 (LevelNumberDialogProcThunk), not the body address.
extern "C" void LevelNumberDialogProcThunk(); // thunk 0x2ab8 -> body 0x8e7c0
INT_PTR CALLBACK LevelNumberDialogProc8e7c0(HWND, UINT, WPARAM, LPARAM);

// -------------------------------------------------------------------------
// Engine objects reached through CGruntzMgr's member pointers. Each engine-side
// method is a reloc-masked __thiscall (`mov ecx,obj; call rel32`), so only the
// layout offsets + call shapes are load-bearing; the displacements reloc-mask.
// The serialize sequence counter is C++-linkage (?g_serialCounter@@3HA @0x229ad0),
// the canonical name folded in wave5-R4 - kept OUT of the extern "C" block below.
extern i32 g_serialCounter;
// SaveState/LoadState stream these; their canonical C++-linkage (?g_...@@3HA)
// bindings live in the globals/lightfxrender units - reference those directly so the
// DIR32 reloc pairs (an extern "C" _g_* twin would collide on the same RVA).
extern i32 g_645594;  // ?g_645594@@3HA @0x245594 (lightfxrender)
extern i32 g_jitterX; // ?g_jitterX@@3HA @0x2452a4 (was g_6452a4)
extern i32 g_jitterY; // ?g_jitterY@@3HA @0x2452cc (was g_6452cc)
extern i32 g_panMinX; // ?g_panMinX@@3HA @0x245508 (was g_645508)
extern i32 g_panMaxX; // ?g_panMaxX@@3HA @0x24550c (was g_64550c)
extern "C" {
    extern i32 g_64557c; // DAT_0064557c  (modal/cursor-busy gate)
    // The clock/scroll/warp globals SaveState streams through the archive.
    extern i32 g_64558c; // DAT_0064558c
    extern i32 g_645590; // DAT_00645590
    DATA(0x00245598)
    extern i32 g_645598; // DAT_00645598
    extern i32 g_64559c; // DAT_0064559c
    extern i32 g_6455a0; // DAT_006455a0
    DATA(0x002455e8)
    extern i32 g_monologoShown; // (the MONOLITH logo is on screen)
}

extern "C" {
    extern "C" i32 g_curPlayer; // DAT_00644c54  (active player/world index)
}

// The two shared sound globals, DEFINED here (owner TU: CGruntzMgr::SetRunState mirrors
// the run-state into g_sndEnabled and StoreInputFlag latches the cue tag; ~20 TUs read
// them). C++ linkage, so every `extern i32 g_snd*` reference tree-wide binds to
// ?g_sndEnabled@@3HA / ?g_sndCueTag@@3HA. (They were previously only DATA-pinned on
// `extern "C"` DECLARATIONS in LevelPreview.cpp, which bound _g_sndEnabled/_g_sndCueTag
// and left every C++-mangled reference - the overwhelming majority - UNBOUND.)
// The plain externs live in <Globals.h>.
DATA(0x0021ab20)
i32 g_sndEnabled = 0; // 0x61ab20  sound-on gate (mirrors CGruntzMgr::m_soundEnabled)
DATA(0x0021ab24)
i32 g_sndCueTag = 0;           // 0x61ab24  the cue-item id played through PlayIfElapsed
extern "C" u32 g_killCueClock; // DAT_006bf3c0 (wrap-safe draw clock)

// The game registry singleton (?g_gameReg@@3PAUWwdGameReg@@A), modeled here with
// the offsets UpdateScoreHud touches (a per-TU view; the DATA pin reloc-masks the
// `mov eax,ds:g_gameReg` load against the already-named symbol).
struct ScoreSub2c { // g_gameReg->m_curState
    char m_pad0[0x1c];
    i32 m_1c; // +0x1c  cumulative score
};
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// The +0x68 world command-grid object is the ONE CTriggerMgr (<Gruntz/TriggerMgr.h>,
// included above) - the former CCmdGrid facet view is dissolved onto it, thunk-proven:
// "Teardown" 0x3b1b == ~CTriggerMgr, "Reset"/"Flush" 0x15c3 == DestroyAllAnims,
// "Command" 0x4250 == RebuildOverlay; the +0x20c/+0x21c delta tables are
// m_rowStateB/m_rowStateC and the +0x288 scored flag is m_288.

// The +0x7c HUD/score accumulator object (CGruntzMgr::m_scoreHud). One object: the
// score/refresh fields + Refresh/Seed (UpdateScoreHud/AccrueScoreTime), the 4-arg
// command sink (BroadcastCmd), and the shared Teardown (Close).
// HudGuard44 (+0x44 one-shot guard) now comes from <Gruntz/SaveInfo.h> (via
// GruntzMgr.h) - shared with the WM_COMMAND dispatcher TU.

// The archive/serializer object SaveState/LoadState stream every clock/scroll/warp
// field through: the shared WAP32 CSerialArchive stream interface (Read @ vtbl +0x2c,
// the Load read path; Write @ +0x30, the Save write path), now the one modeled class
// in <Gruntz/SerialArchive.h> - the former local `CSerializerZ` view is folded away.
// Return values are unused; only the +0x2c/+0x30 dispatch bytes bind.

// The engine reaches the USER32 cursor through a stored function pointer at
// 0x6c44c4 (`mov edi,ds:...; call edi`), not a direct IAT call; model it as a
// typed global fn-ptr so the indirect call shape (`call reg`) falls out.
extern "C" i32(WINAPI* g_pShowCursor)(i32); // PTR_ShowCursor_006c44c4

// A free helper that flushes/forces a redraw of one map index (reloc-masked).
void RedrawMapIndex(i32 idx); // FUN_00558c70

// The trailing __cdecl command hook in BroadcastCmd's fan-out (reloc-masked;
// `push a3..a0; call; add esp,0x10`).
i32 CmdHook(i32 a, i32 b, i32 c, i32 d); // FUN @ 0x17da thunk

// The +0x60 timer/poll object (CGruntzMgr::m_timer; reloc-masked thiscalls).
// StoreInputState mirrors the latest input-state value into its +0x2c field; Stop
// pauses it (Quicksave/AdvanceFrame/UnloadSoundChain); Tick fires it during a cmd-4/7
// broadcast (BroadcastCmd); Teardown + operator delete tear it down (Close).

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

// SaveInfo (the save-slot record) and SaveSink58 (the +0x58 sink) now come from
// <Gruntz/SaveInfo.h> (via GruntzMgr.h) - shared with the WM_COMMAND dispatcher TU.

// The engine's out-of-line block copy (FUN_00520340). Retail calls it here (not
// the CRT __strncpy at 0x120340 (`push n; push src; push dst; call; add esp,0xc`).

// CModalScreen is GONE: it WAS MFC's CDialog. Binary proof (see GruntzMgr.h): ExitModalUI
// (0x903f0) dispatches `call [vtbl+0xc0]` = slot 48, and CDialog's retail vtable (0x1eb174)
// holds 0x1ba9d2 = CDialog::DoModal at slot 48 (slots 49/51 likewise = OnInitDialog 0x1bac5e
// / OnOK 0x1bacc3). So the invented Run()@slot-48 was DoModal all along.

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
// MFC CMapStringToPtr (Lookup @0x1b8008); cast at each call.
struct CColorLookup {};
struct CColorRow {
    char m_pad0[0x14];
    i32* m_14; // +0x14  value table
    char m_pad18[0x64 - 0x18];
    i32 m_64; // +0x64  column index
    i32 m_68; // +0x68  column max
};
// The object held in CWorldZ +0x10; its +0x10 is the embedded CColorLookup.
struct CWorldLookupHolder {
    // Register a level asset-namespace key (@0x155460, /GX; reloc-masked). Called
    // with a null key or one of the "GRUNTZ"/"LEVEL"/"ACTION" prefixes + a flag.
    i32 RegisterKey(const char* key, i32 flag); // 0x155460
    char m_pad0[0x10];
    CColorLookup m_10; // +0x10  (sub-object, NOT a pointer)
};
// The recolor entrypoint (FUN_005532b0): takes the resolved cell pointer by value;
// callee cleans the arg (no `add esp,4` at the call site -> __stdcall). Reloc-masked.
void __stdcall RecolorCell(i32 cell);

// The world's +0x38 load-status code (mapped to a message id by ReportWorldStatus)
// is now a real CWorldZ member (GruntzMgr.h). It is UNSIGNED: the switch range checks
// emit `cmp;ja/jbe` (unsigned), not the signed `jg/jle`.

// LoadWorldMode's reloc-masked siblings (engine objects reached through the
// manager's member pointers; all are __thiscall, so each is modeled as a method on
// its object so `mov ecx,obj; call` falls out - the displacements reloc-mask).
//   m_recolorSurface: a 0x94-byte engine surface object built by Build(), configured by
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
// The +0x54 object is the real CWorldSoundSet (<Gruntz/WorldSoundSet.h>) - the same
// object the ambient-sound TU reads; its +0x08 is that class's CObList voice list.

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
// The per-load world-object factory (CreateGameObjectByName, __cdecl, reloc-masked).
void CreateWorldObjects(void* world);

// ResetWorldState's MFC wait-cursor pair. Retail inlines the global ::Begin/
// EndWaitCursor (AfxGetApp()->...); modeled as reloc-masked free fns here (only
// the call shape is load-bearing - the inlined AfxGetModuleState/[+4]/thiscall
// expansion is part of the documented MFC-inline residual on this @early-stop fn).
void BeginWaitCursor(); // 0x1beafb
void EndWaitCursor();   // 0x1beb10

// One element of the 4-entry options array embedded at CGruntzMgr +0x150 (each
// 0x238 bytes). AdvanceOptionsCycle reads its arm flag (+0x14) + loaded flag
// (+0x20) and ticks the +0x38 sub-object (reloc-masked thiscall); BroadcastCmd
// forwards the 4-arg command into the slot itself.
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
    CBattlezMapConfig m_38; // +0x38
    // Command @0x4250 IS CTriggerMgr::RebuildOverlay; cast at the call.
    i32 Reset(); // 0xda9e0 (external; reloc-masked) - clear/rewind the slot
};

// The downstream command sinks BroadcastCmd fans the 4-arg command out to: the
// +0x68 grid (CTriggerMgr), the live source object (via GetSaveSource), the +0x6c
// sub-mgr (m_cmdSubMgr), the +0x70 tile/map board (m_tileGrid, CGruntzMapMgr, vtbl slot 1),
// and the +0x7c HUD (ScoreHud). Each returns nonzero to keep broadcasting. The
// +0x6c sub-mgr also shares the Teardown + operator delete (Close). All
// reloc-masked.
// The +0x6c sub-manager is the REAL RTTI class CGruntzCmdMgr (<Gruntz/GruntzCmdMgr.h>):
// retail's Close() +0x6c teardown leg calls thunk 0x4066 -> ~CGruntzCmdMgr @0x85bd0. The
// former invented `CmdSink` class that stood here is GONE. NOTE: BroadcastCmd's +0x6c
// command leg really does target CTriggerMgr::RebuildOverlay @0x7a5e0 (retail thunk 0x4250)
// - a genuine cross-class call in the original, so that ONE cast stays (binding-correct).
// The +0x70 object is the REAL RTTI class CGruntzMapMgr (<Gruntz/GruntzMapMgr.h>, pulled by
// GruntzMgr.h): its slot-1 MapCommand is the 4-arg command dispatch BroadcastCmd drives, and
// its ~CGruntzMapMgr @0x85d10 is the teardown Close() calls (retail thunk 0x35b7). The former
// invented `CmdSinkV` class that stood here is GONE - see the note in <Gruntz/GruntzMgr.h>.

// The world's layer/plane object is the shared CViewport (<Gruntz/Viewport.h>): an
// element of the active view's +0x38 layer array AND the object its +0x5c distinguished-
// layer slot points at (same memory, so one type). Its visibility flag word (+0x08),
// height grid (value grid +0x20, per-column base table +0x24), playable field limits
// (+0x30/+0x34) and edge origins (+0x40..+0x4c) all live in the folded CViewport - each
// caller reads only the facet it needs.

// The active world view held at m_world->m_24. One object; each manager method reads a
// different facet: the tile rect (+0x10..+0x1c) + the three scaled-extent output pairs
// (+0xc8..+0xdc) RecomputeViewScale fills, the layer array (+0x38 base / +0x3c count),
// the distinguished layer/plane (+0x5c), and the mode-reload extent pair (+0x64/+0x68
// LoadWorldMode stamps to 0xe). The rescale notify is a reloc-masked thiscall.

// (the cell-height SetCellHeight() is a method of the real CGruntzMapMgr.)

// The engine's __cdecl CString-formatting helper (sprintf-style into a CString
// destination; reloc-masked - only the call shape is load-bearing).
extern "C" void Format(CString* dst, const char* fmt, ...);

// The engine's __cdecl struct-returning world-file-name builder (FUN_0003ad90):
// resolves the current world file from the game window handle into a CString
// (returned by value -> hidden return-slot arg). The real callee at 0x3ad90 is
// ?RunCustomWorldDialog@@YA?AVCString@@HPAV1@@Z (customworlddialog); the (hwnd, 0)
// push shape matches its (int, CString*) params byte-for-byte. Reloc-masked.
CString RunCustomWorldDialog(i32 hwnd, CString* out);

// The per-frame draw-clock globals PerFrameTick stamps each tick. g_wap32Now /
// g_wap32FrameDelta are the engine's just-refreshed clock (mangled C++ globals,
// stored into the game-side mirror g_645580/g_645584); g_6bf3c0/g_6bf3bc are the
// draw-clock pair (extern "C" -> the _g_* C symbols). All reloc-masked DATA refs.
extern "C" {
    extern u32 g_645580; // game-side now mirror (DAT_00645580)
    extern u32 g_645584; // game-side delta mirror (DAT_00645584)
    extern u32 g_645588; // game-side abs clock (DAT_00645588)
    extern u32 g_6bf3bc; // draw-clock delta (cleared) - g_6bf3c0 is g_killCueClock
    // The chat-message sprintf scratch buffer (owner-TU .bss definition; canonical
    // extern in <Globals.h>). RVA-ascending: 0x2452d8 precedes g_645600 below.
    DATA(0x002452d8)
    char g_msgScratch[256]; // 0x6452d8
    // The clock/scroll-state globals ResetClockGlobals zeroes (reloc-masked); bound
    // here (their VA-typo'd C++ ?g_...@@3HA twins in gruntzmgrcmd are a separate defect).
    DATA(0x00245600)
    u32 g_645600; // DAT_00645600 (owner-TU definition, .bss)
    DATA(0x002455b0)
    extern u32 g_traitorMode; // ("Traitor Mode" cheat toggle)
    DATA(0x002455a4)
    extern u32 g_gruntDestruction; // ("Grunt destruction" cheat toggle)
    DATA(0x002455a8)
    extern u32 g_gruntCreation; // ("Grunt creation" cheat toggle)
    DATA(0x002455ac)
    extern u32 g_gooPuddlez; // ("Goo puddlez" cheat toggle)
    DATA(0x002455f8)
    extern u32 g_explosionz; // ("Explosionz" cheat toggle)
    // g_debugFlags (0x6455f4, the u8 debug-overlay byte) comes from <Globals.h>;
    // ResetClockGlobals over-wide dword-zeroes it via `*(u32*)&g_debugFlags`, which
    // reloc-binds the real ?g_debugFlags@@3EA symbol while keeping the dword store.
}

// The two engine input/state singletons TickStateMgrs drives once per call
// (DAT_00645570/DAT_00645578; reloc-masked DATA refs). g_645570 is the
// DirectInputMgr2 (its PollAll @0x533080 is the per-frame device poll); g_645578
// is a second mgr (Flush @0x4385e0). Each call is a single reloc-masked
// __thiscall, so only the one-method shape on a tiny helper is load-bearing.
struct DirectInputMgr2 {
    i32 PollAll();   // FUN_00533080
    i32 Flush();     // FUN_00533110 (a second per-frame entrypoint)
    void Teardown(); // (this) reloc-masked (Close)
    void ReadAll();  // re-arm after a state install (CGruntzMgr::TransitionState tail, waveP)
};
// The +0x578 state manager (g_645578). One object: TickStateMgrs drives its Flush;
// Close zeroes its field block (+0x00..+0x14) before delete.
extern "C" {
    DATA(0x00245570)
    extern DirectInputMgr2* g_645570; // DAT_00245570
    DATA(0x00245578)
    extern StateMgrBZ* g_645578; // DAT_00245578 (canonical binding; also decl'd in Play.h)
}

// The embedded options object's ctor/dtor are out-of-line helpers whose real bodies
// (FUN_0051f5a0 / FUN_0051f640) live in another TU. CGruntzMgr's ctor/dtor pass their
// addresses to the __ehvec ctor/dtor iterators building m_options[4]; the retail /
// INCREMENTAL link routes those address-of pushes through the element ctor/dtor ILT
// thunks (0x2a7c ctor, 0x1465 dtor). Declared-only here (no body) so the references
// are undefined externals bound to the thunk rvas below (binding the empty-body
// definitions would make objdiff compare them against the thunk bytes).
// @data-symbol: ??0CGruntzMgrOptions@@QAE@XZ 0x00002a7c
// @data-symbol: ??1CGruntzMgrOptions@@QAE@XZ 0x00001465
//
// The base WAP32::CGameMgr vtable (0x1e9b8c): the CGruntzMgr dtor + its scalar-deleting
// ??_G restore the base subobject's vptr to it during teardown. Namespaced vtable, so
// bound via @data-symbol (VTBL() only lowers simple global-namespace class names).
// @data-symbol: ??_7CGameMgr@WAP32@@6B@ 0x001e9b8c

// -------------------------------------------------------------------------
// The world's +0x24 view (CWorldView, defined above) exposes a layer array (+0x38
// base, +0x3c count) plus a distinguished sub-layer (+0x5c); each layer (CViewport)
// carries a flag word at +0x8 whose bit 1 (0x2) is a visibility toggle the level-cycle
// / debug methods flip.

// The live state's +0x1c "current level index" the next/prev cycle reads is now the
// real CState::m_levelIndex (<Gruntz/State.h>); the TU-local CLevelState view is gone.

// The +0x5c chat/message-log object: AppendChatMessage routes one message line
// (with type 0 / channel 0x11) into its insert slot (FUN_00421c60, reloc-masked).

// The shared scratch buffer the toggle-message formatter renders "<item> is
// ON/OFF" into before logging it (reloc-masked DATA ref). The format helper is
// the statically-linked CRT sprintf (FUN_0051f890; reloc-masked call).

// Owner-TU .data definitions, RVA-ascending. The deferred per-frame pump flag
// (0x60fac8, init 1); the two warp-target ints (0x612610/0x612614, init -1; canonical
// externs in <Globals.h>).
DATA(0x0020fac8)
i32 g_pendingFrame = 1;
DATA(0x00212610)
i32 g_warpX = -1;
DATA(0x00212614)
i32 g_warpY = -1;

// The game-manager singleton (*0x64556c) - the CGruntzMgr (MFC) view of the datum the
// engine TUs see as CGameRegistry* g_gameReg; same _g_mgrSettings symbol.
extern "C" CGameRegistry* g_gameReg;

// -------------------------------------------------------------------------
// PumpIdleFrame (0x08b8c0; ret) - the deferred per-frame pump. When the pending flag is
// set, clear it and, if the game manager + its world/lookup + live state are all up,
// poll the state (InputVirtual): on idle (0) surface an idle error (0x8006/0x435) and
// bail; else run PerFrameTick and re-arm the pending flag. A free function (reads the
// singleton, no `this`).
// @early-stop
// 88%: complete + correct (structure/branches/singleton re-reads all aligned; PerFrameTick
// called non-virtually). Residual is a pure regalloc coin-flip: retail pins the g_gameReg
// pointer chain (mgr -> m_world / m_curState) in ecx so the InputVirtual thiscall's `this`
// is already there; our cl pins it in eax and copies (`mov ecx,eax`) into the thiscall - a
// base-register choice the permuter can't rewrite (operand-order only), whose eax<->ecx swap
// ripples through the null-check chain. topic:regalloc.
RVA(0x0008b8c0, 0x76)
i32 PumpIdleFrame() {
    if (g_pendingFrame == 0) {
        return 0;
    }
    CGruntzMgr* mgr = (CGruntzMgr*)g_gameReg;
    g_pendingFrame = 0;
    if (mgr == 0) {
        return 0;
    }
    CWorldZ* world = mgr->m_world;
    if (world == 0) {
        return 0;
    }
    if (world->m_10 == 0) {
        return 0;
    }
    if (mgr->m_curState == 0) {
        return 0;
    }
    if (mgr->m_curState->InputVirtual() == 0) {
        g_gameReg->ReportError(0x8006, 0x435);
        return 0;
    }
    ((CGruntzMgr*)g_gameReg)->PerFrameTick();
    g_pendingFrame = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// The CPlay/CDemo /GX destructors (ex PlayDtor.cpp; merged wave3-J - the FLAGS
// table's 0x08b8c0-0x093ce7 group gruntzmgr+playdtor+appdialogs is ONE obj,
// GruntzMgr.cpp, __FILE__-anchored). Uses the ONE canonical CPlay
// (<Gruntz/Play.h>): its five destructible MFC members are typed there (CString
// m_1b4, CByteArray m_startMarkers, CByteArray m_3a4[4], CString m_cueText,
// CByteArray m_488), so the dtor's /GX member-teardown machinery + the
// most-derived vptr stamp fall out from cl. The members fold in reverse decl
// (= reverse offset) order under descending /GX trylevels:
//   +0x488  CByteArray   state 4
//   +0x410  CString      state 3
//   +0x3a4  CByteArray[4] (__ehvec_dtor over CByteArray::~CByteArray)  state 2
//   +0x370  CByteArray   state 1
//   +0x1b4  CString      state 0
// The dtor body first runs the explicit teardown CPlayDtorBody (0xc8700) at the
// top trylevel (state 5), then the members fold, then the CState base subobject
// restamps its dtor vtable (0x5ea21c, reloc-masked) and runs the CState dtor
// body (0xfa150). cl auto-emits ??_7CPlay@@6B@ (masks retail 0x5ea0bc, paired
// via vtable_names.csv) + ??_7CDemo@@6B@ (masks 0x5e9f0c).

// 0x8c830 - CPlay::~CPlay (/GX): stamp the CPlay vtable (prologue), run CPlayDtorBody
// at the top trylevel, fold the five members (reverse decl order, descending /GX
// states), then fold the CState base subobject (restamp 0x5ea21c, call CState body).
// INLINE so it folds into CDemo's dtor (0x8d0d0) exactly as retail inlines the base
// dtor; cl still emits one out-of-line COMDAT copy (driven by ??_GCPlay), which lands
// at 0x8c830. An inline dtor can't hang an RVA() (it would also tag the synthesized
// ??_G -> duplicate-RVA), so it is pinned by mangled name:
// @rva-symbol: ??1CPlay@@UAE@XZ 0x0008c830 0xaf
inline CPlay::~CPlay() {
    CPlayDtorBody();
}

// 0x8c470 - CState::~CState: the STANDALONE out-of-line copy of the (inline, header-
// defined) base-state dtor, referenced only by /GX EH-unwind funclets (13+ sites: the
// base-subobject cleanup when a CState-derived ctor throws). ??_GCState @0x8c710 and
// every derived dtor keep folding their own inline copies; this #pragma inline_depth(0)
// forcer (UserLogicCtorEmit pattern) emits the out-of-line COMDAT so the unwind refs
// resolve and the RVA matches (stamp ??_7CState + tail-jmp to the 0xfa150 base body).
// @rva-symbol: ??1CState@@UAE@XZ 0x0008c470 0xb
static CState* volatile g_forceEmitCState;
#pragma inline_depth(0)
void ForceEmitCStateDtor() {
    g_forceEmitCState->CState::~CState();
}
#pragma inline_depth()
// ===========================================================================
// CGruntzMgr::TransitionState (0x08b960; re-homed from the former gruntzmgrtransition
// unit, waveP -> 0x8b8c0 gruntzmgr). The /GX game-state factory; the CState leaf views
// are reduced local layouts stamping the retail vtables by hand, CPlay is the canonical
// Play.h class, g_645570 reuses this TU's local DirectInputMgr2 (ReadAll added).
struct CTsState {
    virtual void VDtor(u32 flags);                         // slot 0  scalar-deleting dtor
    virtual i32 Activate(CGruntzMgr* mgr, i32 a2, i32 a3); // slot 1  activate
    virtual void VSlot2();
    virtual void VSlot3();
    virtual i32 Id(); // slot 4  id/update
    virtual void VSlot5();
    virtual void VSlot6();
    virtual void VSlot7();
    virtual void VSlot8();
    virtual i32 Slot9(i32 a);  // slot 9
    virtual i32 Slot10(i32 a); // slot 10 (+0x28)
    char m_pad2c[0x1c - 0x0c]; // (kept exactly)
    i32 m_1c;                  // sub-object saved on teardown
    void CallDtor(u32 flags) {
        VDtor(flags);
    }
    i32 CallActivate(CGruntzMgr* mgr, i32 a2, i32 a3) {
        return Activate(mgr, a2, a3);
    }
    i32 CallId() {
        return Id();
    }
    i32 CallSlot9(i32 a) {
        return Slot9(a);
    }
    i32 CallSlot10(i32 a) {
        return Slot10(a);
    }
    virtual void VtSlotFill0();  // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill1();  // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill2();  // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill3();  // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill4();  // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill5();  // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill6();  // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill7();  // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill8();  // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill9();  // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill10(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill11(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill12(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill13(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill14(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill15(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill16(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill17(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill18(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill19(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill20(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill21(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill22(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill23(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill24(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill25(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill26(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill27(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill28(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill29(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill30(); // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill31(); // vtable-slot filler (real slot; declared-only)
};

// The minimal destructible MFC members that force the per-object EH-state ladder;
// their ctors/dtors are the reloc-masked NAFXCW bodies (0x1b9b93 / 0x1b4f0b ...).
// (MfcStr / MfcBytes are GONE: they were fake views of the REAL MFC CString and
// CByteArray. Identical layout - CString is one LPTSTR; CByteArray is a CObject vptr +
// m_pData + m_nSize/m_nMaxSize/m_nGrowBy - and the same reloc-masked NAFXCW ctor/dtor
// calls. But the real types RESOLVE at link out of NAFXCW.LIB, whereas ??0MfcStr@@QAE@XZ
// / ??1MfcStr@@QAE@XZ / ??0MfcBytes / ??1MfcBytes were PHANTOMS: mangled names that no
// obj and no .LIB defines. <Mfc.h> is already included above.)

// The two out-of-line base ctors (0x8c750 = CState base; 0x8c9d0 = CPlay base for
// the multiplayer/param-7 states). Declared no-body -> reloc-masked base calls. Both
// are NON-polymorphic sized layouts: +0x00 is an explicit vptr slot the derived leaf
// stamps by hand from its g_st<Class>Vtbl extern (no compiler-emitted ??_7 here).

// Two extra member sub-objects the credits state (param 8) builds (a small ctor
// @0x8c3b0 and the two 4-arg Set @0x8c380 initializers).
struct CTsSub45 {
    CTsSub45(); // 0x8c3b0
    char m_pad[8];
};
void Ts_Set(void* self, i32 a, i32 b, i32 c, i32 d); // 0x8c380 (member Set, 4 args)

// The ten retail state vtables, referenced by RELOC-MASKED EXTERNAL reference. Each
// leaf ctor stamps `m_vptr = &g_st<Class>Vtbl`; the extern's address IS the retail
// ??_7<Class>, but its DIFFERENT name (+ no DATA() binding) means cl emits no local
// vtable and the delinker keeps the RVA bound to the real class's TU (noted). The
// switch key is the state id TransitionState is asked for.
// (CPlay uses the canonical `class CPlay : public CState` - cl auto-stamps
// ??_7CPlay in its ctor, so no g_stCPlayVtbl manual-stamp extern is needed.)

// ---- the CState-derived state objects (reduced local layouts of the real classes;
// the retail vtable is stamped by hand from the externals above) ----------------
//
// EVERY LEAF BELOW DECLARES ITS DTOR OUT-OF-LINE (declaration only, no body). This is
// load-bearing, not style. Left IMPLICIT, cl5 synthesises a per-TU inline dtor for each
// leaf and emits it as a COMDAT under the leaf's REAL mangled name (??1CAttract@@UAE@XZ
// ...). Because CState::~CState is inline in <Gruntz/State.h>, that synthesised body
// inlines the base dtor, and /O2 dead-stores away the derived vptr write that precedes
// it - leaving a 16-byte stub:
//     mov dword ptr [ecx], offset ??_7CState@@6B@
//     jmp  ?BaseCleanup@CGameModeBase@@QAEXXZ
// which is an ODR-DIVERGENT DUPLICATE of the real 96-byte /GX chain dtor that the leaf's
// own TU (attractstate/menustate/helpstate/bootystateactivate/creditsstate/multi/dialogs)
// defines at its retail rva. cl5 keeps ONE COMDAT per name and picks arbitrarily, so the
// linker could bind every `delete state` to the stub and skip the real teardown. It also
// made reloc_fidelity score the STUB's relocs against retail's real body - the phantom
// "MISBOUND ??_7CState@@6B@ @+0x2 / ?BaseCleanup @+0x7" rows, which were never a defect
// in the state TUs at all (their bodies match retail byte-for-byte).
// Declared-only => no definition is emitted here, so each reference binds to the one real
// body. The classes are polymorphic already (CState's dtor is virtual), so this changes
// no layout and no vtable slot - it is byte-neutral.
// CAttract is the CANONICAL <Gruntz/Attract.h> class (included above). The reduced local
// twin is GONE: its all-CState-base-slot ??_7CAttract@@6B@ was an ODR landmine, and the
// canonical class emits the vtable attractstate emits. This fold needed Attract.h's
// CAttract::Update to be UN-INLINED first (an RVA() on an inline header body is claimed
// by every TU that emits the COMDAT - see the note on Update in Attract.h).
// NOT foldable onto the canonical <Gruntz/GameMode.h> `CMenuState : CState` (Bucket-C
// base-fold wall): a canonical polymorphic CMenuState would make cl emit + stamp a
// LOCAL ??_7CMenuState here (the ctor references its own vtable), claiming the retail
// vtable RVA 0x5e9e84 that gamemode already owns. This non-poly shell + manual g_st*
// stamp is the deliberate whole-family pattern (see file header); the loader-view fold
// happened in MenuStateAssets.cpp, which does not construct the class.
// EVERY leaf below RE-DECLARES ITS CANONICAL VIRTUAL OVERRIDES. This is the ODR fix, not
// decoration. A `new CX` here forces cl to emit ??_7CX@@6B@ in THIS obj (the ctor is
// inlined at the new-site and stamps the vptr - see retail 0x8bacf..). When the leaf
// declared no overrides, that emitted vtable was 26 CState BASE slots - byte-different
// from the real vtable its own TU emits. cl5 keeps ONE vtable COMDAT per name and picks
// arbitrarily, so the linker could bind every CMenuState/CBootyState/... to a vtable that
// dispatches straight to CState: a silent, shipping-severity mis-dispatch that costs 0%
// in objdiff (vtable contents are reloc-masked). Declaring the same overrides in the same
// slot order makes this obj's vtable byte-identical to the leaf TU's, so the duplicate is
// benign. Bodies stay in the leaf TU (declared-only here => reloc-masked references).
struct CMenuState : CState { // param 5, 0x1c0
    i32 m_1b4;               // +0x1b4
    char m_pad1b8[0x1c0 - 0x1b8];
    CMenuState() {
        // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
        m_1b4 = 0;
    }
    virtual ~CMenuState() OVERRIDE;              // 0x0008ce60 (MenuState.cpp)
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE;  // slot 1
    virtual void ReleaseResources() OVERRIDE;    // slot 2
    virtual GameStateId Update() OVERRIDE;       // slot 4
    virtual i32 Render() OVERRIDE;               // slot 5
    virtual i32 Vslot06() OVERRIDE;              // slot 6
    virtual i32 Vslot07() OVERRIDE;              // slot 7
    virtual i32 InputVirtual() OVERRIDE;         // slot 8
    virtual i32 Vslot09(i32) OVERRIDE;           // slot 9
    virtual i32 FrameSlot28(i32) OVERRIDE;       // slot 10
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14
    virtual i32 Vslot10(i32, i32, i32) OVERRIDE; // slot 16
    virtual i32 Vslot14(i32, i32, i32) OVERRIDE; // slot 20
};
// CHelpState is the canonical <Gruntz/HelpState.h> class (included above) - the reduced
// local twin is GONE (its all-base-slot ??_7CHelpState was the same ODR landmine).
struct CSplashState : CState { // param 14, 0x1bc
    i32 m_1b4;                 // +0x1b4
    char m_pad1b8[0x1bc - 0x1b8];
    CSplashState() {
        // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
        m_1b4 = 0;
    }
    virtual ~CSplashState() OVERRIDE;            // 0x0008d000 (HelpState.cpp)
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE;  // slot 1
    virtual void ReleaseResources() OVERRIDE;    // slot 2
    virtual GameStateId Update() OVERRIDE;       // slot 4
    virtual i32 Render() OVERRIDE;               // slot 5
    virtual i32 Vslot06() OVERRIDE;              // slot 6
    virtual i32 InputVirtual() OVERRIDE;         // slot 8
    virtual i32 Vslot09(i32) OVERRIDE;           // slot 9
    virtual i32 FrameSlot28(i32) OVERRIDE;       // slot 10
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14
};
// CDemo is the canonical <Gruntz/Demo.h> class (included below); its ctor + the
// `new CDemo` factory case + CDemo::Vslot15 (0x3c030) live in this TU.
struct CMultiBootyState : CState { // param 18, 0x244
    i32 m_1b4;                     // +0x1b4
    i32 m_1b8;                     // +0x1b8
    char m_pad1bc[0x244 - 0x1bc];
    CMultiBootyState() {
        // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
        m_1b4 = 0;
        m_1b8 = 0x64;
    }
    virtual ~CMultiBootyState() OVERRIDE;        // 0x0008d510 (BootyStateActivate.cpp)
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE;  // slot 1
    virtual void ReleaseResources() OVERRIDE;    // slot 2
    virtual GameStateId Update() OVERRIDE;       // slot 4
    virtual i32 Render() OVERRIDE;               // slot 5
    virtual i32 Vslot06() OVERRIDE;              // slot 6
    virtual i32 Vslot07() OVERRIDE;              // slot 7
    virtual i32 InputVirtual() OVERRIDE;         // slot 8
    virtual i32 Vslot09(i32) OVERRIDE;           // slot 9
    virtual i32 FrameSlot28(i32) OVERRIDE;       // slot 10
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14
    virtual i32 Vslot11(i32, i32, i32) OVERRIDE; // slot 17
};
struct CBootyState : CState { // param 10, 0x320
    i32 m_1b4;                // +0x1b4
    i32 m_1b8;                // +0x1b8
    i32 m_1bc;                // +0x1bc
    i32 m_1c0;                // +0x1c0
    i32 m_1c4;                // +0x1c4
    i32 m_1c8;                // +0x1c8
    i32 m_1cc;                // +0x1cc
    i32 m_1d0;                // +0x1d0
    i32 m_1d4;                // +0x1d4
    char m_pad1d8[0x1ec - 0x1d8];
    i32 m_1ec; // +0x1ec
    i32 m_1f0; // +0x1f0
    i32 m_1f4; // +0x1f4
    i32 m_1f8; // +0x1f8
    char m_pad1fc[0x200 - 0x1fc];
    i32 m_200; // +0x200
    char m_pad204[0x284 - 0x204];
    i32 m_284[8]; // +0x284
    i32 m_2a4[8]; // +0x2a4
    i32 m_2c4;    // +0x2c4
    char m_pad2c8[0x2e8 - 0x2c8];
    i32 m_2e8; // +0x2e8
    i32 m_2ec; // +0x2ec
    i32 m_2f0; // +0x2f0
    i32 m_2f4; // +0x2f4
    char m_pad2f8[0x320 - 0x2f8];
    CBootyState();
    virtual ~CBootyState() OVERRIDE;             // 0x0008d440 (BootyStateActivate.cpp)
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE;  // slot 1
    virtual void ReleaseResources() OVERRIDE;    // slot 2
    virtual GameStateId Update() OVERRIDE;       // slot 4
    virtual i32 Render() OVERRIDE;               // slot 5
    virtual i32 Vslot06() OVERRIDE;              // slot 6
    virtual i32 Vslot07() OVERRIDE;              // slot 7
    virtual i32 InputVirtual() OVERRIDE;         // slot 8
    virtual i32 Vslot09(i32) OVERRIDE;           // slot 9
    virtual i32 FrameSlot28(i32) OVERRIDE;       // slot 10
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14
    virtual i32 Vslot11(i32, i32, i32) OVERRIDE; // slot 17
};
// NOTE for matcher-4 (the vtbl-stamp owner of this file): the canonical
// CCreditsState : CState EXISTS in <Gruntz/GameMode.h> (RTTI vtbl@0x1e9c64) and
// CreditzAssets.cpp now folds onto it. This local shell + its g_stCCreditsStateVtbl
// ctor stamp is the SAME Bucket-C whole-family pattern documented on CMenuState
// above; when the family converts, union THIS view's field knowledge into the
// canonical (m_1c8/m_1d8 rect subs; m_1e8's CTsSub45 ctor 0x8c3b0 is the canonical
// CCreditsImageList's; m_1f0 == m_caption; m_208/m_210 == m_videoPlaying/
// m_videoHandle - offsets agree, no conflation).
struct CCreditsState : CState { // param 8, 0x218
    i32 m_1b4;                  // +0x1b4
    i32 m_1b8;                  // +0x1b8
    i32 m_1bc;                  // +0x1bc
    i32 m_1c0;                  // +0x1c0
    i32 m_1c4;                  // +0x1c4
    char m_1c8[0x10];           // +0x1c8  Set-initialized rect sub-object
    char m_1d8[0x10];           // +0x1d8  Set-initialized rect sub-object
    CTsSub45 m_1e8;             // +0x1e8
    CString m_1f0;              // +0x1f0
    i32 m_1f4;                  // +0x1f4
    i32 m_1f8;                  // +0x1f8
    i32 m_1fc;                  // +0x1fc
    i32 m_200;                  // +0x200
    i32 m_204;                  // +0x204
    i32 m_208;                  // +0x208
    i32 m_20c;                  // +0x20c
    i32 m_210;                  // +0x210
    char m_pad214[0x218 - 0x214];
    CCreditsState();
    virtual ~CCreditsState() OVERRIDE;           // 0x0008d5e0 (CreditsState.cpp)
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE;  // slot 1
    virtual void ReleaseResources() OVERRIDE;    // slot 2
    virtual GameStateId Update() OVERRIDE;       // slot 4
    virtual i32 Render() OVERRIDE;               // slot 5
    virtual i32 Vslot06() OVERRIDE;              // slot 6
    virtual i32 InputVirtual() OVERRIDE;         // slot 8
    virtual i32 Vslot09(i32) OVERRIDE;           // slot 9
    virtual i32 FrameSlot28(i32) OVERRIDE;       // slot 10
    virtual i32 Vslot0c(i32, i32) OVERRIDE;      // slot 12
    virtual i32 Vslot0e(i32, i32, i32) OVERRIDE; // slot 14
};
// CPlay (param 3, 0x520) is the canonical `class CPlay : public CState` from
// <Gruntz/Play.h>; its five destructible MFC members + CState base give the same
// inline-construction shape the factory needs, and its ctor (defined below) stamps
// ??_7CPlay via cl (no manual g_stCPlayVtbl stamp).
struct CMulti : CPlay { // param 17, 0x660
    i32 m_520;          // +0x520
    i32 m_524;          // +0x524
    char m_pad528[0x590 - 0x528];
    i32 m_590; // +0x590
    char m_pad594[0x598 - 0x594];
    CString m_598, m_59c, m_5a0; // +0x598/+0x59c/+0x5a0
    char m_pad5a4[0x5b0 - 0x5a4];
    i32 m_5b0;            // +0x5b0
    CString m_5b4, m_5b8; // +0x5b4/+0x5b8
    char m_pad5bc[0x600 - 0x5bc];
    i32 m_600;        // +0x600
    CByteArray m_604; // +0x604
    char m_pad618[0x660 - 0x618];
    CMulti();
    virtual ~CMulti() OVERRIDE;                 // 0x0008d270 (Multi.cpp)
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE; // slot 1  (CState)
    virtual void ReleaseResources() OVERRIDE;   // slot 2  (CState)
    virtual GameStateId Update() OVERRIDE;      // slot 4  (CState)
    virtual i32 Render() OVERRIDE;              // slot 5  (CState)
    virtual i32 Vslot09(i32) OVERRIDE;          // slot 9  (CState)
    virtual i32 FrameSlot28(i32) OVERRIDE;      // slot 10 (CState)
    virtual i32 Vslot0b(i32, i32) OVERRIDE;     // slot 11 (CState)
    virtual i32 Vslot15() OVERRIDE;             // slot 21 (CState)
    virtual i32 Vslot1a() OVERRIDE;             // slot 26 (CPlay)
    virtual i32 GetFrame() OVERRIDE;            // slot 27 (CPlay)
    virtual i32 Vslot1e(i32, i32) OVERRIDE;     // slot 30 (CPlay)
    virtual void Vslot20() OVERRIDE;            // slot 32 (CPlay)
    virtual void Vslot26() OVERRIDE;            // slot 38 (CPlay)
};

// Field-heavy leaf ctors defined out-of-line for readability (still inline-folded
// into the factory's new-expressions). The manual vptr stamp leads each body (the
// ctor body runs after the base + member sub-object ctors, matching the retail
// position of the `mov [this],offset ??_7<Class>` store).
CBootyState::CBootyState() {
    // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
    m_1c0 = 0;
    m_1c8 = 0;
    m_1c4 = 0;
    m_1cc = 0;
    m_1b8 = 0;
    m_1bc = 0x64;
    m_2c4 = 0;
    m_2e8 = 0;
    m_2ec = 0;
    m_2f0 = 0;
    m_1b4 = 0;
    m_2f4 = 0;
    m_200 = 0;
    m_1d0 = 0;
    m_1d4 = 0;
    m_1ec = 0;
    m_1f0 = 0;
    m_1f4 = 0;
    m_1f8 = 0;
    for (i32 i = 0; i < 8; i++) {
        m_284[i] = 0;
        m_2a4[i] = 0;
    }
}
CCreditsState::CCreditsState() {
    // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
    m_1b8 = 0;
    m_1bc = 0;
    m_1c0 = 0;
    m_1c4 = 0;
    m_1f4 = 0;
    m_1f8 = 0;
    m_1fc = 0;
    m_200 = 0;
    m_204 = 0;
    Ts_Set(m_1c8, 0, 0, 0x280, 0x1e0);
    Ts_Set(m_1d8, 0, 0, 0x280, 0x1e0);
    m_20c = 1;
    m_210 = 0;
    m_208 = 0;
    m_1b4 = 0;
}
CPlay::CPlay() {
    // cl runs the CState base ctor + the five member ctors, then auto-stamps
    // ??_7CPlay, then this field-init body (matching the retail inlined construction).
    char* p = (char*)this;
    *(i32*)(p + 0x328) = 0;
    *(i32*)(p + 0x330) = 0;
    *(i32*)(p + 0x32c) = 0;
    *(i32*)(p + 0x334) = 0;
    *(i32*)(p + 0x338) = 0;
    *(i32*)(p + 0x340) = 0;
    *(i32*)(p + 0x33c) = 0;
    *(i32*)(p + 0x344) = 0;
    *(i32*)(p + 0x350) = 0;
    *(i32*)(p + 0x358) = 0;
    *(i32*)(p + 0x354) = 0;
    *(i32*)(p + 0x35c) = 0;
    *(i32*)(p + 0x3f8) = 0;
    *(i32*)(p + 0x400) = 0;
    *(i32*)(p + 0x3fc) = 0;
    *(i32*)(p + 0x404) = 0;
    *(i32*)(p + 0x1bc) = 0;
    *(i32*)(p + 0x1c0) = 0;
    *(i32*)(p + 0x1c8) = 0;
    *(i32*)(p + 0x2e0) = 0;
    *(i32*)(p + 0x3f4) = 0;
    *(i32*)(p + 0x2dc) = 0;
    *(i32*)(p + 0x2e4) = 0;
    *(i32*)(p + 0x4cc) = 0;
    *(i32*)(p + 0x4e4) = 0;
    *(i32*)(p + 0x2f0) = 0;
    *(i32*)(p + 0x2d0) = 0;
    *(i32*)(p + 0x2d4) = 0;
    *(i32*)(p + 0x2f4) = 0;
    *(i32*)(p + 0x2f8) = -1;
    *(i32*)(p + 0x320) = 0;
    *(i32*)(p + 0x4d4) = 0;
    *(i32*)(p + 0x4b0) = 0;
    *(i32*)(p + 0x348) = 1;
    *(i32*)(p + 0x510) = 0;
    *(i32*)(p + 0x518) = 0;
    *(i32*)(p + 0x30c) = 0;
    *(i32*)(p + 0x2e8) = 0;
    *(i32*)(p + 0x4f0) = 0;
    *(i32*)(p + 0x368) = 0;
    *(i32*)(p + 0x36c) = 0;
    *(i32*)(p + 0x2ec) = 0;
    *(i32*)(p + 0x504) = 0;
}
CMulti::CMulti() {
    // foreign/base vptr install dropped (compiler-managed / not C++-nameable; % ok per drive-to-0)
    m_520 = 0;
    m_524 = 0;
    m_590 = 1;
    m_5b0 = 0;
    m_600 = 1;
}

// ===========================================================================
// CGruntzMgr::TransitionState (0x8b960) - see the file header.
// @early-stop
// /GX EH-state-numbering wall (same family as the CProjectile ctor / ApplySwitch /
// DestroyGroup plateaus): the teardown, the m_a4 replay fast-path, the whole
// per-state new/ctor/member/vtable/field-init switch and the install/activate tail
// are reconstructed with the retail sizes, vtables and field sets. The residue is
// the __ehfuncinfo state ladder across the ~13 nested new-expression try-regions
// (each `mov [esp+0x1c],N` state id) + the interleave of the per-object field
// zeroing with the member ctors - MSVC5's state numbering + store scheduling for a
// factory of this size is not source-steerable. Logic complete. topic:wall topic:eh.
// ===========================================================================
RVA(0x0008b960, 0x7c4)
i32 CGruntzMgr::TransitionState(i32 stateId, i32 a2, i32 keepCurrent, i32 a4) {
    (void)a4;
    CTsState* cur = (CTsState*)m_curState;
    i32 local10 = 0;
    if (cur != 0) {
        local10 = cur->CallId();
        i32 savedSub = cur->m_1c;
        cur->CallSlot10(stateId);
        if (keepCurrent != 0) {
            PushState(m_curState);
            a2 = savedSub; // retail reuses the arg2 stack slot as scratch for cur->m_1c
            m_curState = 0;
        } else {
            if (m_curState != 0) {
                ((CTsState*)m_curState)->CallDtor(1);
            }
            m_curState = 0;
            FlushStateStack();
            m_curState = 0;
        }
    } else if (keepCurrent == 0) {
        FlushStateStack();
    }

    if (m_a4 != 0) {
        // The replay fast-path constructs a bare, fully-modeled CState (its ctor is
        // the standalone CState::CState @0x8c750). No shell, no (CState*)this view -
        // this is the real class. The external `call ??0CState` vs retail's inlined
        // field init is a call-vs-inline delta inside this factory's @early-stop EH
        // residue (measured neutral, 43.10% either way); ??_7CState stays owned by
        // its real TU (gamemode) - the ctor stamps it, this new-expression doesn't.
        m_curState = new CState;
        return 1;
    }

    CTsState* obj;
    switch (stateId) {
        case 2:
            obj = (CTsState*)new CAttract;
            break;
        case 3:
            obj = (CTsState*)new CPlay;
            break;
        case 5:
            obj = (CTsState*)new CMenuState;
            break;
        case 7:
            obj = (CTsState*)new CDemo;
            break;
        case 8:
            obj = (CTsState*)new CCreditsState;
            break;
        case 9:
            obj = (CTsState*)new CHelpState;
            break;
        case 10:
            obj = (CTsState*)new CBootyState;
            break;
        case 14:
            obj = (CTsState*)new CSplashState;
            break;
        case 17:
            obj = (CTsState*)new CMulti;
            break;
        case 18:
            obj = (CTsState*)new CMultiBootyState;
            break;
        default:
            goto install;
    }
    m_curState = (CState*)obj;

install:
    if (m_curState == 0) {
        *(i32*)(*(char**)((char*)this + 0x8) + 0x244) = 0;
        return 0;
    }
    PerFrameTick();
    {
        CTsState* st = (CTsState*)m_curState;
        i32 ok = st->CallActivate(this, a2, local10);
        st = (CTsState*)m_curState;
        if (ok == 0) {
            if (st != 0) {
                st->CallDtor(1);
            }
            m_curState = 0;
            return 0;
        }
        st->CallSlot9(local10);
        *(i32*)(*(char**)((char*)this + 0x8) + 0x244) = 1;
        g_645570->ReadAll();
        PerFrameTick();
        return 1;
    }
}

SIZE_UNKNOWN(CBootyState);
SIZE_UNKNOWN(CCreditsState);
SIZE_UNKNOWN(CDInputMgrZ);
VTBL(CHelpState, 0x001e9dfc); // vtable_names -> code (RTTI game class)
SIZE_UNKNOWN(CMenuState);
SIZE_UNKNOWN(CMulti);
VTBL(CMulti, 0x001e9fe4); // vtable_names -> code (RTTI game class)
SIZE_UNKNOWN(CMultiBootyState);
SIZE_UNKNOWN(CSplashState);
SIZE_UNKNOWN(CState);
SIZE_UNKNOWN(CPlay);
SIZE_UNKNOWN(CTsState);
SIZE_UNKNOWN(CTsSub45);

VTBL(CPlay, 0x001ea0bc);

// 0x8d0d0 - CDemo::~CDemo (/GX): stamp the derived vtable (0x5e9f0c), run the derived
// cleanup (0x3c010) at trylevel 0, then INLINE-fold CPlay's teardown (restamp 0x5ea0bc,
// CPlayDtorBody, the five members, the CState base).
RVA(0x0008d0d0, 0xc4)
CDemo::~CDemo() {
    // The retail +0x2b call targets the ILT thunk 0x3c010, itself a 5-byte jmp to
    // CPlayDtorBody (0xc8700) - so the "derived cleanup" resolves to the CPlay
    // teardown body. Bind to CPlayDtorBody (0xc8700) so the reloc is faithful (the
    // former ?DerivedCleanup@CDemo view was unbound). The compiler then inline-folds
    // the ~CPlay base teardown (its own CPlayDtorBody call at +0x40). Byte-neutral.
    CPlay::CPlayDtorBody();
}

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
    i32 next = st->m_levelIndex + 1;
    if (next > 0x28) {
        next = 1;
    }
    if (next <= 0x20 || next >= 0x25) {
        st->FrameSlot28(st->Update());
        if (((CPlay*)st)->Vslot1e(next, 1)) {
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
    i32 prev = st->m_levelIndex - 1;
    if (prev <= 0) {
        prev = 0x28;
    }
    if (prev <= 0x20 || prev >= 0x25) {
        st->FrameSlot28(st->Update());
        if (((CPlay*)st)->Vslot1e(prev, 1)) {
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
    CGameApp* pApp = m_owner;
    if (pApp) {
        pApp->ReportError(wParam, lParam);
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::XorLiveObjectFlags  (@0x08dc20, ret 4)
// Toggle the given flag bits (m_stateFlags @+0x40) on every live object in the
// world sprite factory's created-object list (m_world->m_8->m_liveObjects).
// @early-stop
// byte-proven (llvm-objdump -dr base vs target): the loop body is byte-identical;
// the ONLY difference is the list-head access. Retail computes `m_8+0x10` then reads
// `[+4]` with a dead null-preserving check (`add eax,0x10; je`), i.e. it reaches the
// head through a CObList/CPtrList sub-object at factory+0x10 (head @+0x14 = the
// sub-object's m_pNodeHead) rather than the flat `m_liveObjects@+0x14` field the
// ScanObjects walkers use. No source spelling over the flat field reproduces the
// +0x10 intermediate + its dead guard; the 3-instr shift cascades to 87.6%.
RVA(0x0008dc20, 0x2b)
void CGruntzMgr::XorLiveObjectFlags(i32 mask) {
    CSpriteListNode* node = m_world->m_8->m_liveObjects;
    while (node) {
        CSpriteListNode* cur = node;
        node = node->next;
        CGameObject* obj = cur->m_sprite;
        if (obj) {
            obj->m_stateFlags ^= mask;
        }
    }
}

// The asset-namespace key at 0x60ba44 (an unlabeled .rdata datum; reloc-masked).
extern const char s_assetKeyBa44[]; // 0x60ba44

// -------------------------------------------------------------------------
// CGruntzMgr::RegisterLevelAssetKeys  (@0x08dc90, ret 0)
// (Re)register the level asset-namespace keys into the world's lookup holder
// (m_10, with a flag) and its sound host (m_28), resetting the world coord
// dispatch (m_1c) twice in between. No-op with no world loaded.
RVA(0x0008dc90, 0xb1)
void CGruntzMgr::RegisterLevelAssetKeys() {
    CWorldZ* w = m_world;
    if (w == 0) {
        return;
    }
    w->m_10->RegisterKey(0, 1);
    w->m_28->RegisterKey(0);
    ((CWorldDispatch*)w->m_1c)->Prepare();
    ((CWorldDispatch*)w->m_1c)->Prepare();
    w->m_10->RegisterKey(0, 1);
    w->m_10->RegisterKey("GRUNTZ", 1);
    w->m_10->RegisterKey(s_assetKeyBa44, 1);
    w->m_10->RegisterKey("LEVEL", 1);
    w->m_10->RegisterKey("ACTION", 1);
    w->m_28->RegisterKey(0);
    w->m_28->RegisterKey("GRUNTZ");
    w->m_28->RegisterKey(s_assetKeyBa44);
    w->m_28->RegisterKey("LEVEL");
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
// CGruntzMgr::IsStandardMode (0x08f980; ret 0). True when the live video mode is
// 640x480 (m_modeW == 0x280 && m_modeH == 0x1e0).
RVA(0x0008f980, 0x21)
i32 CGruntzMgr::IsStandardMode() {
    if (m_modeW == 0x280 && m_modeH == 0x1e0) {
        return 1;
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::AppendChatMessage (0x08f9c0; ret 4). Routes one message line into
// the +0x5c chat/message-log object (insert with type 0 / channel 0x11),
// returning its result; 0 when no log is bound.
RVA(0x0008f9c0, 0x1d)
i32 CGruntzMgr::AppendChatMessage(char* msg) {
    CFontConfig* log = m_chatLog;
    if (log == 0) {
        return 0;
    }
    return log->AddItem(msg, 0, 0x11);
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

// IsInPlayState (0x08fa40): live-state playability probe - 0 when no current
// state, else CheckPlayState() as a bool. Out-of-line (retail emits it standalone;
// the inline member folded into its callers and never emitted).
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
// letter; otherwise call the free ::GetGruntzDriveLetter() (WinAPICdRom.cpp), cache + set the
// flag. (The result is discarded by the engine on the first/uncached path - the
// store IS the return, the cached path returns the byte.)
RVA(0x0008fa70, 0x2c)
char CGruntzMgr::GetGruntzDriveLetter() {
    if (m_driveLetterProbed) {
        return m_driveLetter;
    }
    m_driveLetter = ::GetGruntzDriveLetter();
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
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x120d, hr, m_gameWnd->m_hwnd);
        return 0;
    }
    if (!m_lobby) {
        return 0;
    }

    if (m_connSettings) {
        // 0x1b9b82 = ??3@YAXPAX@Z (global ::operator delete; the `::` forces the
        // global over any member delete) - retail frees the connection settings blob.
        ::operator delete(m_connSettings);
        m_connSettings = 0;
    }

    DWORD dwSize = 0; // real GetConnectionSettings takes LPDWORD (unsigned long*)
    hr = m_lobby->GetConnectionSettings(0, 0, &dwSize);
    if (hr != 0 && hr != (i32)DPERR_BUFFERTOOSMALL) {
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x1221, hr, m_gameWnd->m_hwnd);
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
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x1232, hr, m_gameWnd->m_hwnd);
        m_lobby->Release();
        m_lobby = 0;
        return 0;
    }

    m_lobbyResult = 1;
    return m_lobbyResult;
}

// -------------------------------------------------------------------------
// 0x08ee70 (spatially re-homed from src/Stub/ApiCallers.cpp). Pause audio
// (slot 0x28), force the cursor visible, MessageBoxA, then hide it.
// @orphan: the +0x30 audio object / +0x04 window owner class is unrecovered.
// @early-stop
// regalloc free-list-pick wall (docs/patterns/select-zero-mask-dest-register.md):
// body byte-exact, but every value-holding register is rotated vs retail - the
// `m_30->m_4->m_14` chain (ecx/eax vs our eax/ecx), the `*m_30->m_1c; ->vtbl
// ->Slot28(p)` dispatch (container ecx + vtbl edx vs our edx + ecx) and the
// MessageBoxA arg setup all carry the same global re-colouring. Same
// instructions, swapped registers; not source-steerable (~98.5%).
struct Poly08 { // real polymorphic; Slot28 is slot 10 (+0x28)
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot09();
    virtual void __stdcall Slot28(); // slot 10 (+0x28)
};
struct AudioSub_08ee70 {
    char m_pad0[0x14];
    i32 m_14; // +0x14 = audio handle
};
struct AudioObj_08ee70 {
    char m_pad0[4];
    AudioSub_08ee70* m_4; // +0x04 (its [+0x14] is the audio handle)
    char m_pad8[0x1c - 8];
    Poly08** m_1c; // +0x1c
};
struct MsgWnd_08ee70 {
    char m_pad0[4];
    HWND m_4; // +0x04 -> HWND
};
struct MsgHost_08ee70 {
    char m_pad0[4];
    MsgWnd_08ee70* m_4; // +0x04
    char m_pad8[0x30 - 8];
    AudioObj_08ee70* m_30; // +0x30
    i32 Show(i32 text, i32 type);
};
// The shared caption buffer (DAT_0060aac8) passed as the MessageBoxA title.
DATA(0x0020aac8)
extern char g_msgCaption[];
void __stdcall Stop_158c70(i32 handle); // RVA 0x158c70
RVA(0x0008ee70, 0x7c)
i32 MsgHost_08ee70::Show(i32 text, i32 type) {
    if (m_30) {
        Stop_158c70(m_30->m_4->m_14);
        Poly08* p = *m_30->m_1c;
        p->Slot28();
    }
    i32 wasShown = ShowCursor(1);
    while (ShowCursor(1) < 0) {
    }
    i32 result = MessageBoxA(m_4->m_4, (LPCSTR)text, g_msgCaption, type);
    if (wasShown <= 0) {
        while (ShowCursor(0) >= 0) {
        }
    }
    return result;
}
SIZE_UNKNOWN(Poly08);
SIZE_UNKNOWN(AudioSub_08ee70);
SIZE_UNKNOWN(AudioObj_08ee70);
SIZE_UNKNOWN(MsgWnd_08ee70);
SIZE_UNKNOWN(MsgHost_08ee70);

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
        CGameLevel* view = m_world->m_24;
        if (view) {
            i32 count = view->m_planes.GetSize();
            // (count==4 ? count-1 : count) - 1: best-scoring spelling (96.7%); it
            // recovers retail's ecx/edx view/count regs + the `cmp;jne;dec` shape.
            // The lone residual is MSVC folding the true-arm `count-1` to `mov 3`
            // (+ the je/jne polarity) where retail kept `dec eax`; the literal
            // `if(idx==4)idx--;idx--;` form regresses to 88% (view in edx). The
            // fold is the constant-CSE tiebreak, not source-steerable.
            i32 idx = (count == 4 ? count - 1 : count) - 1;
            CViewport* layer = (idx < 0 || idx >= count) ? 0 : (CViewport*)view->m_planes[idx];
            if (layer && !(layer->m_flags & 1)) {
                layer->m_flags ^= 2;
                return 1;
            }
        }
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ToggleHeightLayer (0x08f060; ret). Visibility toggle for the world
// view's distinguished sub-layer ((CViewport*)m_world->m_24->m_mainPlane) - flipped unconditionally
// (no lock check). Active-gated + world/view guarded like ToggleObjectLayer.
RVA(0x0008f060, 0x35)
i32 CGruntzMgr::ToggleHeightLayer() {
    if (Wap32GameMgrVfunc3() && m_world) {
        CGameLevel* view = m_world->m_24;
        if (view) {
            CViewport* layer = (CViewport*)view->m_mainPlane;
            if (layer) {
                layer->m_flags ^= 2;
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
        CGameLevel* view = m_world->m_24;
        if (view) {
            CViewport* layer = (view->m_planes.GetSize() > 0) ? (CViewport*)view->m_planes[0] : 0;
            if (layer && !(layer->m_flags & 1)) {
                layer->m_flags ^= 2;
                return 1;
            }
        }
    }
    return 0;
}

// -------------------------------------------------------------------------
// LaunchWebBrowser (0x08f120; free __stdcall(url)) - read the HKCR http\shell\open
// \command browser command line, strip any leading flags / IEXPLORE.EXE prefix and
// trailing -/ switches, then CreateProcess it with the supplied URL appended. A free
// helper CGruntzMgr::HandleCommand (0x862f0) calls; RVA-contiguous with the manager
// methods (in the gruntzmgr .obj). Re-homed from src/Stub/ApiWrappers.cpp.
// The real callee at 0x118ce0 (reloc-masked): ?FindProcessByName@@YAHPBDHPAPAX@Z.
i32 FindProcessByName(const char* name, i32 flag, void** out);
RVA(0x0008f120, 0x264)
i32 __stdcall LaunchWebBrowser(char* url) {
    LONG len = 0x104;
    char cmd[0x104];
    if (RegQueryValueA((HKEY)0x80000000, "http\\shell\\open\\command", cmd, &len)) {
        return 0;
    }
    if (strlen(cmd) < 3) {
        return 0;
    }
    i32 quoted = 0;
    // 0x18d330 = __strupr (LIBCMT): the command is UPPER-cased before the
    // "IEXPLORE.EXE" (uppercase) substring test below.
    _strupr(cmd);
    if (strstr(cmd, "IEXPLORE.EXE")) {
        FindProcessByName("IEXPLORE.EXE", 1, (void**)&quoted);
    }
    char* dash = strchr(cmd, '-');
    i32 dn = dash - cmd + 1;
    if (dash) {
        if (dn <= 2) {
            return 0;
        }
    }
    if (dash) {
        cmd[dn - 2] = 0;
    }
    char* slash = strchr(cmd, '/');
    i32 sn = slash - cmd + 1;
    if (slash) {
        if (sn <= 2) {
            return 0;
        }
    }
    if (slash) {
        cmd[sn - 2] = 0;
    }
    char cmdline[0x104];
    sprintf(cmdline, "%s %s", cmd, url);
    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    PROCESS_INFORMATION pi;
    si.cb = sizeof(si);
    return CreateProcessA(0, cmdline, 0, 0, FALSE, 0, 0, 0, &si, &pi);
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
    CString name = RunCustomWorldDialog((i32)m_gameWnd->m_hwnd, 0);
    if (name.GetLength() == 0) {
        return 0;
    }
    m_strWorldFile = name;
    m_12c = 0;
    m_128 = 0;
    g_pPostMessageA((i32)m_gameWnd->m_hwnd, 0x111, 0x8005, 0);
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::PerFrameTick  (__thiscall; `ret`)
// The per-frame draw-clock tick. If the active state's Update() reports the
// "paused/hold" id (0x11) the tick is skipped; otherwise it refreshes the engine
// clock (CGameMgr::InitializeTimeGlobal), optionally re-stamps the
// draw clock (g_6bf3c0 = timeGetTime(), g_6bf3bc = 0) when the draw gate m_world is
// set, and finally mirrors the freshly-refreshed engine clock into the game-side
// pair (g_645580/g_645584).
RVA(0x0008f620, 0x51)
void CGruntzMgr::PerFrameTick() {
    if (m_curState && m_curState->Update() == 0x11) {
        return;
    }

    InitializeTimeGlobal();

    if (m_world) {
        g_killCueClock = timeGetTime();
        g_6bf3bc = 0;
    }

    g_645580 = g_wap32Now;
    g_645584 = g_wap32FrameDelta;
}

// -------------------------------------------------------------------------
// CGruntzMgr::AdvanceFrame  (__thiscall; `ret 8`)
// Retail Ghidra labels this ?VirtualMethod06@CGruntzMgr@@QAEXXZ (a
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
        if (m_frameGate != 0) {
            return;
        }
        if (m_musicEnabled == 0) {
            return;
        }
        if (CheckPlayState() == 0 && (m_curState == 0 || m_curState->Update() != 8)) {
            return;
        }
        m_sound->StopBank(1);
        return;
    }

    if (m_musicEnabled == 0) {
        return;
    }
    if ((m_sound->m_pCurrent ? m_sound->m_pCurrent->IsBusy() : 0) == 0) {
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
// unresolved id). The two CString temps + the szPath buffer give the /GX frame.
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
        return name; // unresolved id
    }

    CString path;
    char szDir[256];

    // First try the working directory ("<cwd>\<name>").
    if (GetCurrentDirectoryA(0xff, szDir)) {
        Format(&path, "%s\\%s", szDir, (const char*)name);
        if (!FileExists((char*)(const char*)path)) {
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

    if (!FileExists((char*)(const char*)path)) {
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
// 0x08f480 (spatially re-homed from src/Stub/ApiCallers.cpp). If the mode is
// 2/3/5, reset and post a 0x8005 command; returns 1 (else 0).
// @orphan: no direct rel32 caller (reached only as a data/command-table slot) -
// owning class of the +0x2c mode object unrecovered.
struct ModeObj_08f480 { // slot 4 (+0x10) returns the current mode
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual i32 GetMode(); // slot 4 == vtable +0x10
};
struct WndChain_08f480 {
    char m_pad0[4];
    HWND m_4; // +0x04
};
struct Sub_08f480 {
    void Reset(); // thiscall, RVA 0x1b9c69 (on this+0xc8)
};
struct ModeHost_08f480 {
    char m_pad0[4];
    WndChain_08f480* m_4; // +0x04
    char m_pad8[0x2c - 8];
    ModeObj_08f480* m_2c; // +0x2c
    char m_pad30[0xc8 - 0x30];
    Sub_08f480 m_c8; // +0xc8
    i32 Notify();
};
RVA(0x0008f480, 0x49)
i32 ModeHost_08f480::Notify() {
    i32 mode = m_2c->GetMode();
    if (mode == 5 || mode == 2 || mode == 3) {
        m_c8.Reset();
        PostMessageA(m_4->m_4, 0x111, 0x8005, 0);
        return 1;
    }
    return 0;
}
SIZE_UNKNOWN(ModeObj_08f480);
SIZE_UNKNOWN(WndChain_08f480);
SIZE_UNKNOWN(Sub_08f480);
SIZE_UNKNOWN(ModeHost_08f480);

// -------------------------------------------------------------------------
// CGruntzMgr::ResetClockGlobals (0x08f4f0). Zeroes the seven game-clock / scroll
// state globals (reloc-masked DATA refs).
RVA(0x0008f4f0, 0x26)
void CGruntzMgr::ResetClockGlobals() {
    g_645600 = 0;
    g_traitorMode = 0;
    g_gruntDestruction = 0;
    g_gruntCreation = 0;
    g_gooPuddlez = 0;
    g_explosionz = 0;
    *(u32*)&g_debugFlags = 0; // dword-zero the u8 debug-overlay byte + 3 trailing bytes
}

// -------------------------------------------------------------------------
// CGruntzMgr::SetGameClock (0x08f7b0; ret 0xc). Mirrors the three clock args into
// the game-side now/delta/abs globals plus the draw-clock pair (reloc-masked).
RVA(0x0008f7b0, 0x2b)
void CGruntzMgr::SetGameClock(i32 now, i32 delta, i32 abs) {
    g_645580 = now;
    g_645584 = delta;
    g_645588 = abs;
    g_killCueClock = now;
    g_6bf3bc = delta;
}

// RunFromState (0x090200): forward to ChangeState_8fab0(1). Out-of-line (retail
// emits it standalone; the inline member folded away and never emitted).
RVA(0x00090200, 0x8)
i32 CGruntzMgr::RunFromState() {
    return ChangeState_8fab0(1);
}

// -------------------------------------------------------------------------
// CGruntzMgr::TopState (0x090980). Returns the last pushed state (or 0 when the
// stack is empty).
RVA(0x00090980, 0x18)
CState* CGruntzMgr::TopState() {
    CPtrArray* st = &m_stateStack;
    if (st->GetSize() <= 0) {
        return 0;
    }
    return (CState*)st->GetAt(st->GetSize() - 1);
}

// -------------------------------------------------------------------------
// CGruntzMgr::PushState (0x0909b0; ret 4). Appends s to the state stack via
// SetAtGrow(GetSize(), s); ignores a null push.
RVA(0x000909b0, 0x1b)
void CGruntzMgr::PushState(CState* s) {
    if (!s) {
        return;
    }
    CPtrArray* st = &m_stateStack;
    st->SetAtGrow(st->GetSize(), (void*)s);
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
    i32 n = m_stateStack.GetSize();
    if (n <= 0) {
        return 0;
    }
    CState* top = (CState*)m_stateStack.GetAt(n - 1);
    m_stateStack.RemoveAt(n - 1, 1);
    return top == s;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ClearStateStack (0x090a50; ret). Deletes every pushed state then
// clears the array via SetSize(0, -1). The loop reads count/data off `this`
// (+0xe0/+0xdc) so the array base is not hoisted into a register.
RVA(0x00090a50, 0x40)
void CGruntzMgr::ClearStateStack() {
    for (i32 i = 0; i < m_stateStack.GetSize(); i++) {
        CState* s = (CState*)m_stateStack.GetAt(i);
        if (s) {
            delete s;
        }
    }
    m_stateStack.SetSize(0, -1);
}

// -------------------------------------------------------------------------
// CGruntzMgr::CheckMovieFileExists (0x090aa0; cdecl ret). Probes whether the
// resolved movie path (m_strMoviePath) exists on disk.
RVA(0x00090aa0, 0x10)
i32 CGruntzMgr::CheckMovieFileExists() {
    return FileExists((char*)(const char*)m_strMoviePath);
}

// -------------------------------------------------------------------------
// CGruntzMgr::IsMoviePathValid (0x0901d0; cdecl ret). The bool-normalized twin of
// CheckMovieFileExists: FileExists(m_strMoviePath) run through the neg/sbb/neg
// 0/1 canonicalizer (the result is consumed as a boolean here).
RVA(0x000901d0, 0x16)
i32 CGruntzMgr::IsMoviePathValid() {
    return FileExists((char*)(const char*)m_strMoviePath) != 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::Post (0x090220; thiscall(code), ret 4). Clamp `code` into (0,0x29]
// and PostMessageA WM_COMMAND 0x807f to the game window (wParam = code, or 1 when
// code == 0x29). Re-homed from the ApiCaller stubs (was CmdHost_090220::Post): the
// receiver is the CGruntzMgr singleton, reached by CPlay::Dispatch (0xcfbd0) as
// m_4->Post via the CState owner back-ptr.
RVA(0x00090220, 0x2f)
void CGruntzMgr::Post(i32 code) {
    if (code > 0 && code <= 0x29) {
        i32 v = (code == 0x29) ? 1 : code;
        g_pPostMessageA((i32)m_gameWnd->m_hwnd, 0x111, 0x807f, v);
    }
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
    u32 status = m_world->m_38;
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
// existing sprite TOGGLES its visible bit (m_8 & 2) + the g_monologoShown shown-flag; a
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
// MFC CMapStringToPtr (Lookup @0x1b8008); the calls were folded with the CColorLookup vein.
struct CMonoConfigMap {};
struct CMonoConfigHolder {
    char m_pad0[0x10];
    CMonoConfigMap m_10; // +0x10  embedded string map
};
struct CMonoSprite;
// FindSprite @0x15dde0 = CGameLevel::FindPlaneByName, CreateSprite @0x15d9a0 =
// CGameLevelPlanes::ReadObjectPlane (transitively-visible real classes); cast at each call.
struct CMonoView {};
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
    CObArray m_9c; // +0x9c  frame CObArray (SetAtGrow @0x1b5822)
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
    ((CMapStringToPtr*)&((CMonoWorld*)m_world)->m_10->m_10)->Lookup("GAME_MONOLITH", (void*&)out);
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
    CMonoSprite* found =
        (CMonoSprite*)((CGameLevel*)((CMonoWorld*)m_world)->m_24)->FindPlaneByName("MONOLITH");
    if (found == 0) {
        CMonoSprite* spr =
            (CMonoSprite*)((CGameLevelPlanes*)((CMonoWorld*)m_world)->m_24)
                ->ReadObjectPlane(0x20, 0x20, geoA, geoB, -0x19, -0x19, (i32) "MONOLITH");
        if (spr == 0) {
            return 0;
        }
        spr->m_9c.SetAtGrow(0, (CObject*)rec);
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
        g_monologoShown = 1;
        return 1;
    }
    if (found->m_8 & 2) {
        found->m_8 &= ~2;
        g_monologoShown = 1;
    } else {
        found->m_8 |= 2;
        g_monologoShown = 0;
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
        ((CMapStringToPtr*)&m_world->m_10->m_10)->Lookup((const char*)key, (void*&)out);
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
// CGruntzMgr::WarpCheat (0x08eaf0; __thiscall; ret). Reads the per-level warp
// target (registry keys "Level %i Warp X"/"Level %i Warp Y" for the current level)
// through m_settings. If either is unset (-1) it bails. When the live state is PLAY
// (id 3) it just records the current level as "Last Warp Level" (arming the return
// warp) and returns 1. Otherwise it reads back "Last Warp Level" and warps there via
// PassClickToPlayState, posting WM_COMMAND 0x80ca to the game window on success; a
// failed warp surfaces (0x8005, 0x43b) and returns 0. The two format sprintf reads
// use the g_gameReg singleton view (ds:g_gameReg) exactly as retail does.
RVA(0x0008eaf0, 0x10b)
i32 CGruntzMgr::WarpCheat() {
    char key[64];
    sprintf(key, "Level %i Warp X", g_gameReg->m_curState->m_levelIndex);
    i32 wx = ((Utils::RegistryHelper*)m_settings)->GetValueDword(key, -1);
    sprintf(key, "Level %i Warp Y", g_gameReg->m_curState->m_levelIndex);
    i32 wy = ((Utils::RegistryHelper*)m_settings)->GetValueDword(key, -1);
    if (wx != -1 && wy != -1) {
        if (m_curState->Update() != 3) {
            i32 last = ((Utils::RegistryHelper*)m_settings)->GetValueDword("Last Warp Level", -1);
            if (last != -1) {
                if (!PassClickToPlayState(last, 0, 1)) {
                    ReportError(0x8005, 0x43b);
                    return 0;
                }
                g_pPostMessageA((i32)m_gameWnd->m_hwnd, 0x111, 0x80ca, 0);
                return 1;
            }
        } else {
            ((Utils::RegistryHelper*)m_settings)
                ->SetValueDword("Last Warp Level", m_curState->m_levelIndex);
            return 1;
        }
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::CheatRevealTreasures (0x090f10; __thiscall; ret). PLAY-state only
// (m_curState->Update() == 3, world loaded): looks the "GAME_DEVHEADS" row up in the
// world's +0x10 color-lookup table and, when found, recolors every treasure/
// collectible sink through SetGruntColor - the geckos/scepters group 0, crosses
// group 1, chalices group 2. Returns 1 on a hit, 0 on any missing precondition.
RVA(0x00090f10, 0x151)
i32 CGruntzMgr::CheatRevealTreasures() {
    if (m_curState == 0) {
        return 0;
    }
    if (m_curState->Update() != 3) {
        return 0;
    }
    if (m_world == 0) {
        return 0;
    }
    void* found = 0;
    ((CMapStringToPtr*)&m_world->m_10->m_10)->Lookup("GAME_DEVHEADS", found);
    void* out = found;
    if (out == 0) {
        return 0;
    }
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_GECKOS_RED", 0);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_GECKOS_GREEN", 0);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_GECKOS_BLUE", 0);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_GECKOS_PURPLE", 0);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_SCEPTERS_RED", 0);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_SCEPTERS_GREEN", 0);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_SCEPTERS_BLUE", 0);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_SCEPTERS_PURPLE", 0);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_CROSSES_RED", 1);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_CROSSES_GREEN", 1);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_CROSSES_BLUE", 1);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_CROSSES_PURPLE", 1);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_CHALICES_RED", 2);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_CHALICES_GREEN", 2);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_CHALICES_BLUE", 2);
    SetGruntColor((i32)out, (i32) "GAME_TREASURE_CHALICES_PURPLE", 2);
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::CheatSkeletonToggle (0x091250; __thiscall). PLAY-state only. Looks the
// "Gruntz" image set up in the world's +0x10 lookup table and, from its lowest-index
// frame's format state (m_format->m_14), toggles all grunt frames between the normal
// (SetAllTypes(2), "You're scaring me...") and skeleton (SetAllTypes(1), "Back from
// the dead?") image type, logs the taunt, then - if the sound gate is open - plays
// the throttled "GAME_MINORCHEAT" cue. Every missing precondition falls straight
// through to the shared exit (retail sets no return value there -> void).
// @early-stop
// codegen-tie wall (~98%): flow (nested guards, void fall-through exit, per-branch
// SetAllTypes + shared AppendChatMessage cross-jump, cooldown + cue) is byte-exact. The
// lone residual is the `fmt->m_14 == 2` test - retail loads it (`mov eax,[eax+0x14];
// cmp eax,2`) where MSVC5 folds to `cmp [mem],2` (2 B shorter). Retail itself picks the
// OTHER form for the identical test in 0x091390, so it is a pure instruction-selection
// tie with no source spelling that flips it.
RVA(0x00091250, 0x100)
void CGruntzMgr::CheatSkeletonToggle() {
    if (m_curState && m_curState->Update() == 3 && m_world) {
        CObject* found = 0;
        ((CMapStringToPtr*)&m_world->m_10->m_10)->Lookup("Gruntz", (void*&)found);
        CImageSet* set = (CImageSet*)found;
        if (set) {
            CImageFrame* fr = set->m_frames[set->m_minIndex];
            if (fr) {
                CImageFormat* fmt = fr->m_format;
                if (fmt) {
                    i32 st = fmt->m_14;
                    if (st != 2) {
                        set->SetAllTypes(2);
                        AppendChatMessage((char*)"You're scaring me...");
                    } else {
                        set->SetAllTypes(1);
                        AppendChatMessage((char*)"Back from the dead?");
                    }
                    CSndHost* host = m_world->m_28;
                    if (host->m_emitGate == 0) {
                        found = 0;
                        host->m_10.Lookup("GAME_MINORCHEAT", found);
                        LeafCue* cue = (LeafCue*)found;
                        if (cue) {
                            i32 tag = g_sndCueTag;
                            if (g_sndEnabled) {
                                if ((u32)(g_killCueClock - cue->m_14) >= (u32)cue->m_18) {
                                    cue->m_14 = g_killCueClock;
                                    cue->m_10->ConfigureItem(tag, 0, 0, 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // Deliberate fall-through: retail's single shared exit (0x9134c) leaves eax unset
    // on every non-emit path (only the ConfigureItem emit path defines the result); an
    // explicit `return` would add a divergent eax-setting tail. MSVC5 emits the bare
    // `pop; ret` here; the clang label step's -Wreturn-type is a warning, not an error.
}

// -------------------------------------------------------------------------
// CGruntzMgr::CheatEclipseToggle (0x091390; __thiscall). The dusk/eclipse twin of
// CheatSkeletonToggle: keyed on m_format->m_14 == 3, it either restores the normal
// image type (SetAllTypes(1), "Where did the sun go?") or applies a random shade
// (SetAllTypes(3) + SetAllField18(rand() % 256), "Me and my...") before logging the
// taunt and playing the same "GAME_MINORCHEAT" cue.
// @early-stop
// regalloc-swap wall (~94%): the flow (nested guards, rand()%256 signed-mod idiom, the
// per-branch SetAllTypes/SetAllField18 + shared AppendChatMessage, cooldown + cue) is
// byte-exact. The residual is a two-callee-saved-register tiebreak: retail assigns
// this->edi / set->esi where MSVC5 here picks this->esi / set->edi (the extra
// SetAllField18+rand live range shifts the pick), recolouring every this/set modrm
// downstream. No source spelling forces MSVC's esi/edi assignment (regalloc family).
RVA(0x00091390, 0x11d)
void CGruntzMgr::CheatEclipseToggle() {
    if (m_curState && m_curState->Update() == 3 && m_world) {
        CObject* found = 0;
        ((CMapStringToPtr*)&m_world->m_10->m_10)->Lookup("Gruntz", (void*&)found);
        CImageSet* set = (CImageSet*)found;
        if (set) {
            CImageFrame* fr = set->m_frames[set->m_minIndex];
            if (fr) {
                CImageFormat* fmt = fr->m_format;
                if (fmt) {
                    i32 st = fmt->m_14;
                    if (st != 3) {
                        set->SetAllTypes(3);
                        set->SetAllField18(rand() % 256);
                        AppendChatMessage((char*)"Me and my...");
                    } else {
                        set->SetAllTypes(1);
                        AppendChatMessage((char*)"Where did the sun go?");
                    }
                    CSndHost* host = m_world->m_28;
                    if (host->m_emitGate == 0) {
                        found = 0;
                        host->m_10.Lookup("GAME_MINORCHEAT", found);
                        LeafCue* cue = (LeafCue*)found;
                        if (cue) {
                            i32 tag = g_sndCueTag;
                            if (g_sndEnabled) {
                                if ((u32)(g_killCueClock - cue->m_14) >= (u32)cue->m_18) {
                                    cue->m_14 = g_killCueClock;
                                    cue->m_10->ConfigureItem(tag, 0, 0, 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // Deliberate fall-through: retail's single shared exit (0x9134c) leaves eax unset
    // on every non-emit path (only the ConfigureItem emit path defines the result); an
    // explicit `return` would add a divergent eax-setting tail. MSVC5 emits the bare
    // `pop; ret` here; the clang label step's -Wreturn-type is a warning, not an error.
}

// The live game objects are dispatched to a __cdecl callback (obj, user).
typedef i32(__cdecl* ScanCb)(CGameObject* obj, i32 user);

// -------------------------------------------------------------------------
// CGruntzMgr::ScanObjectsInRadius (0x092180; __thiscall; ret 0x18). Walks the live
// game-object list (m_world->m_8->m_liveObjects) and, for each object whose
// collision-category bits (m_collCategory) intersect `mask`, tests a game-space reach
// metric (|dx|^2 + 2*|dy|) against radius^2. Every in-range object is counted and
// passed to cb(obj, user); the walk stops early if cb returns 0. Returns the hit
// count. A null cb returns 0.
RVA(0x00092180, 0x98)
i32 CGruntzMgr::ScanObjectsInRadius(i32 x, i32 y, i32 radius, i32 mask, i32 cb, i32 user) {
    if (cb == 0) {
        return 0;
    }
    i32 r2 = radius * radius;
    i32 count = 0;
    CSpriteListNode* node = m_world->m_8->m_liveObjects;
    while (node) {
        CSpriteListNode* cur = node;
        node = node->next;
        CGameObject* obj = cur->m_sprite;
        if (obj->m_collCategory & mask) {
            i32 adx = abs(obj->m_screenX - x);
            i32 ady = abs(obj->m_screenY - y);
            if (adx * adx + ady + ady < r2) {
                count++;
                if (((ScanCb)cb)(obj, user) == 0) {
                    return count;
                }
            }
        }
    }
    return count;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ScanObjectsInRect (0x092250; __thiscall; ret 0x18). The bounding-box
// twin of ScanObjectsInRadius: `rect` (left/top/right/bottom) is offset by (offX,
// offY), then every live game object matching `mask` whose screen position falls in
// the offset box is counted and passed to cb(obj, user). Stops early if cb returns 0.
// @early-stop
// regalloc/frame wall (~84.7%): the object-list walk, the four offset-rect bounds,
// the bounding-box test, the callback and the count are byte-exact. Retail keeps all
// four bounds in registers (edi/ebp/ebx/edx) and reserves a dedicated `sub esp,0x10`
// spill frame; MSVC 5.0 here proves offX dead after the X bounds and reuses its
// incoming arg slot ([esp+0x14]) to spill hiY, comparing against memory (`cmp ecx,
// [mem]`) instead of a register - so no frame is reserved. No source spelling flips
// MSVC's dead-arg-slot reuse back to a fresh frame (regalloc family).
RVA(0x00092250, 0xba)
i32 CGruntzMgr::ScanObjectsInRect(i32 offX, i32 offY, i32 rect, i32 mask, i32 cb, i32 user) {
    if (cb == 0) {
        return 0;
    }
    RECT* r = (RECT*)rect;
    if (r == 0) {
        return 0;
    }
    i32 loX = r->left + offX;
    i32 hiX = r->right + offX;
    i32 loY = r->top + offY;
    i32 hiY = r->bottom + offY;
    i32 count = 0;
    CSpriteListNode* node = m_world->m_8->m_liveObjects;
    while (node) {
        CSpriteListNode* cur = node;
        node = node->next;
        CGameObject* obj = cur->m_sprite;
        if (obj->m_collCategory & mask) {
            i32 ox = obj->m_screenX;
            if (ox >= loX && ox <= hiX) {
                i32 oy = obj->m_screenY;
                if (oy >= loY && oy <= hiY) {
                    count++;
                    if (((ScanCb)cb)(obj, user) == 0) {
                        return count;
                    }
                }
            }
        }
    }
    return count;
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
// m_54/m_recolorSurface two-stage teardowns) match. The low % is a big-SEH scoring desync:
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

    CWorldSoundSet* in = m_inputState;
    if (in) {
        in->Deactivate();
        ((CObList*)((char*)in + 8))->CObList::~CObList();
        RezFree(in);
    }
    m_inputState = 0;

    CRezSurface94* surf = m_recolorSurface;
    if (surf) {
        surf->Teardown();
        RezFree(surf);
    }
    m_recolorSurface = 0;

    m_colorDepth = mode;
    g_6455e0 = 0;
    g_6455dc = 0;
    if (m_colorDepth == 0x10) {
        g_6455dc = 1;
    }

    ((CWorldModeIface*)m_world)->Notify();
    i32 kind = (g_6455b4 == 0) ? 1 : 5;
    if (((CWorldModeIface*)m_world)
            ->SetVideoMode((i32)m_gameWnd->m_hwnd, 0x280, 0x1e0, m_colorDepth, kind)
        == 0) {
        ReportWorldStatus(0x43f);
        return 0;
    }

    ((CDDrawSurfaceMgr*)m_world)->SetHwnd((void*)ModeResetCallback);
    CGameLevel* view = m_world->m_24;
    view->m_maxStepX = 0xe;
    view->m_maxStepY = 0xe;
    CreateWorldObjects(m_world);
    if (MakeRezPath() == 0) {
        return 0;
    }

    CRezSurface94* old = m_recolorSurface;
    if (old) {
        old->Teardown();
        RezFree(old);
        m_recolorSurface = 0;
    }
    CRezSurface94* obj = (CRezSurface94*)RezAlloc(0x94);
    if (obj) {
        obj->Build();
    } else {
        obj = 0;
    }
    m_recolorSurface = obj;

    CString path;
    i32* row = ResolveRezRow(&path);
    if (m_recolorSurface->Apply(*row, 1, 0)) {
        ReportError(0x800b, 0x441);
        return 0;
    }

    SetColorDepth(m_colorDepth);

    CWorldSoundSet* in2 = m_inputState;
    if (in2) {
        in2->Deactivate();
        ((CObList*)((char*)in2 + 8))->CObList::~CObList();
        RezFree(in2);
    }
    m_inputState = 0;

    void* no = RezAlloc(0x30);
    CWorldSoundSet* ni;
    if (no) {
        new ((char*)no + 8) CObList(0xa);
        *(i32*)no = 0;
        *(i32*)((char*)no + 4) = 0x64;
        ni = (CWorldSoundSet*)no;
    } else {
        ni = 0;
    }
    m_inputState = ni;
    if (ni->Init(m_world->m_28, m_inputFlag) == 0) {
        ReportError(0x800a, 0x442);
        return 0;
    }

    CWorldSoundSet* cur = m_inputState;
    if (m_isAmbientEnabled != 0) {
        if (cur->m_active == 0) {
            cur->m_active = 1;
            cur->Resume();
        }
    } else {
        if (cur->m_active != 0) {
            cur->m_active = 0;
            cur->Stop();
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
    TransitionState(stateId, 1, 0, 0);
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
    if (m_sound && m_musicEnabled) {
        m_sound->StopAll();
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::StopBank0IfActive (0x092030; ret). When a sound object is bound and
// a level is loaded, stops the bank with flag 0.
RVA(0x00092030, 0x18)
void CGruntzMgr::StopBank0IfActive() {
    if (m_sound && m_musicEnabled) {
        m_sound->StopBank(0);
    }
}

// The global asset-root path CString (0x64e25c; SplashState.cpp's g_assetRoot binds
// it, NetMgrMisc's g_netE25c is the same datum). SetAssetRoot assigns it and pokes
// the game window.
extern CString g_assetRoot;

// -------------------------------------------------------------------------
// CGruntzMgr::SetAssetRoot (0x092060; ret 4). Copy `path` into the global asset-root
// CString and post WM_COMMAND 0x80ab to the game window; ret 1 (0 when path is null).
// @early-stop
// reloc-masked PostMessageA IAT-absolute operand only (the 0x6c44c8 slot bakes no
// symbol, same scoring artifact as the sibling PostSlot* posters); code bytes are
// byte-exact (CString::operator= + the WM_COMMAND push chain).
RVA(0x00092060, 0x3c)
i32 CGruntzMgr::SetAssetRoot(char* path) {
    if (path == 0) {
        return 0;
    }
    g_assetRoot = path;
    g_pPostMessageA((i32)m_gameWnd->m_hwnd, 0x111, 0x80ab, 0);
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::PostSlotCommandB1 (0x0920e0; ret 4). For an in-range slot (0..3),
// post WM_COMMAND 0x80b1 to the game window with the slot index as lParam. Returns
// 1 (0 when the slot is out of range).
RVA(0x000920e0, 0x32)
i32 CGruntzMgr::PostSlotCommandB1(i32 slot) {
    if (slot < 0 || slot >= 4) {
        return 0;
    }
    g_pPostMessageA((i32)m_gameWnd->m_hwnd, 0x111, 0x80b1, slot);
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::PostSlotCommandB6 (0x092130; ret 4). Twin of PostSlotCommandB1 with
// command code 0x80b6.
RVA(0x00092130, 0x32)
i32 CGruntzMgr::PostSlotCommandB6(i32 slot) {
    if (slot < 0 || slot >= 4) {
        return 0;
    }
    g_pPostMessageA((i32)m_gameWnd->m_hwnd, 0x111, 0x80b6, slot);
    return 1;
}

// -------------------------------------------------------------------------
// winapi_092a30_EndDialog (0x092a30): a bare OK/Cancel modal DialogProc (address-
// taken via the 0x2649 thunk). WM_INITDIALOG returns TRUE; on WM_COMMAND IDOK (1)
// / IDCANCEL (2) it ends the dialog with the command id / 0 respectively.
RVA(0x00092a30, 0x52)
INT_PTR CALLBACK winapi_092a30_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            return 1;
        case WM_COMMAND:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::IsLobbyHostReady (0x091500). Null-chain predicate: returns
// m_curState->ReleaseResources-slot result (slot 7, +0x1c) only when m_curState, the
// CGameApp (m_8), its m_appActive flag (+0x240) and !m_modalBusy all hold; else 0.
RVA(0x00091500, 0x42)
i32 CGruntzMgr::IsLobbyHostReady() {
    if (m_curState == 0) {
        return 0;
    }
    CGameApp* app = m_owner;
    if (app == 0) {
        return 0;
    }
    if (app->m_appActive == 0) {
        return 0;
    }
    if (m_modalBusy != 0) {
        return 0;
    }
    return m_curState->Vslot07() != 0;
}

// ---------------------------------------------------------------------------
// 0x08e880 - when in the PLAY state, register the DEBUG_SETSKILL cheat command.
// (re-homed from src/Stub/BoundaryLowerMethods.cpp; dissolves the C8e880 view.)
// The DEBUG_SETSKILL dialog proc, address-taken through its ILT thunk (0x1947);
// bound to the thunk rva (the reference is a reloc-masked DIR32 push).
// @data-symbol: ?Lab401947@@YAXXZ 0x00001947
extern void Lab401947(); // thunk 0x1947 (code address passed as a ptr; reloc-masked)
RVA(0x0008e880, 0x27)
i32 CGruntzMgr::RegisterSetSkillDebugCmd() {
    if (m_curState->Update() == GAMESTATE_PLAY) {
        RunModalDialog("DEBUG_SETSKILL", (void*)&Lab401947, 1);
    }
    return 0;
}

// 0x0915d0 - fade the current music track to silence over `ms` when it is playing.
RVA(0x000915d0, 0x3f)
void CGruntzMgr::MuteMusicIfActive(i32 ms) {
    if (m_sound == 0) {
        return;
    }
    if (m_musicEnabled == 0) {
        return;
    }
    i32 ok;
    if (m_sound->m_pCurrent != 0) {
        ok = m_sound->m_pCurrent->IsBusy();
    } else {
        ok = 0;
    }
    if (ok == 0) {
        return;
    }
    if (m_sound->m_pCurrent == 0) {
        return;
    }
    m_sound->m_pCurrent->SetVolume(0, ms);
}

// 0x091620 - restore the current music track to full volume over `ms`.
RVA(0x00091620, 0x3f)
void CGruntzMgr::RestoreMusicVolumeIfActive(i32 ms) {
    if (m_sound == 0) {
        return;
    }
    if (m_musicEnabled == 0) {
        return;
    }
    i32 ok;
    if (m_sound->m_pCurrent != 0) {
        ok = m_sound->m_pCurrent->IsBusy();
    } else {
        ok = 0;
    }
    if (ok == 0) {
        return;
    }
    if (m_sound->m_pCurrent == 0) {
        return;
    }
    m_sound->m_pCurrent->SetVolume(0x64, ms);
}

// -------------------------------------------------------------------------
// CGruntzMgr::StoreInputState (0x091a10; ret 4). Stores v at +0x120, and when the
// +0x60 sub-object is present mirrors it into that object's +0x2c.
RVA(0x00091a10, 0x17)
i32 CGruntzMgr::StoreInputState(i32 v) {
    m_inputStateVal = v;
    CGruntSpawnConfig* timer = m_timer;
    if (timer) {
        timer->m_2c = v;
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
// (m_world->m_28->m_2c, guarded), mirror the new state into the g_sndEnabled gate,
// then flush the +0x54 input object - Arm (thunk 0x18e8) when entering the run
// state (m_10 != 0), Disarm (thunk 0x29b9) when leaving it. A no-op when the value is unchanged, and
// the whole side-effect chain is skipped when no world is loaded.
// @early-stop
// 99.62% global-store regalloc tiebreak: logic byte-exact. The lone residual is
// the g_sndEnabled store - retail re-reads m_10 into eax and uses the `a3` accumulator
// store (mov ds:g_sndEnabled,eax), MSVC here loads it into ecx (mov [g_sndEnabled],ecx,
// 89 0d). A 1-instruction eax<->ecx pick on the global store; no source spelling
// flips it (see docs/patterns/select-zero-mask-dest-register.md, regalloc family).
RVA(0x00092340, 0x49)
void CGruntzMgr::SetRunState(i32 v) {
    if (v == m_soundEnabled) {
        return;
    }
    m_soundEnabled = v;
    if (m_world == 0) {
        return;
    }
    SoundStream* sub = m_world->m_28->m_2c;
    if (sub) {
        sub->Stop();
    }
    g_sndEnabled = m_soundEnabled;
    if (m_soundEnabled) {
        m_inputState->Resume();
    } else {
        m_inputState->Stop();
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
    CPtrArray* st = &m_stateStack;
    for (i32 i = 0; i < st->GetSize(); i++) {
        CState* s = (CState*)st->GetAt(i);
        if (s && s->Update() == id) {
            return s;
        }
    }
    return 0;
}

// PickPlayOrPausedState (0x092990): the concrete PLAY state, FindStateById(3).
// Out-of-line (retail emits it standalone; the inline member folded away).
RVA(0x00092990, 0x8)
CPlay* CGruntzMgr::PickPlayOrPausedState() {
    return (CPlay*)FindStateById(3);
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
    if (loaded == m_musicEnabled) {
        return;
    }
    m_musicEnabled = loaded;
    CGruntzSoundZ* snd = m_sound;
    if (snd == 0) {
        return;
    }
    if (loaded != 0) {
        CGruntzSoundInnerZ* cur = snd->m_pCurrent;
        if (cur == 0) {
            return;
        }
        if (cur->m_playMode != 0) {
            snd->Restart(1);
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
// When the first-frame guard (m_hudGuard->m_124) is already set, it pops a localized
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
    if (m_hudGuard->m_124 != 0) {
        CString name;
        name.LoadStringA(0x81aa);
        EnterModalUI(name);
        return 1;
    }
    if (m_saveInfoRec == 0 || !(m_saveInfoRec->m_flags & 1)) {
        return LoadSaveMessageSprite();
    }
    if ((char*)m_curState + 0x1d0 == 0) { // inlined GetSaveSource() non-null guard
        return 0;
    }
    if (m_timer) {
        m_timer->Stop();
    }
    FillSaveInfo(m_saveInfoRec, 0);
    if (((CSaveGame*)g_gameReg->m_saveSink)->Save((i32)m_saveInfoRec->m_serial, 0x81a7)) {
        m_chatLog->AddItem("Game Quicksaved successfully.", 0, 0x11);
        return 1;
    }
    EnterModalUI("ERROR - Cannot Save Game.");
    return 1;
}

// CGruntzMgr::Quickload (0x092710) - the quickload counterpart of Quicksave (above).
// Bails (0) with no save sink; otherwise flushes the timer, and on a valid quicksave
// record (m_saveInfoRec bit 0) loads it via the sink (SaveSink58::Check @0xe52c0),
// notifies the window (WM_COMMAND 0x807e via g_pPostMessageA) and logs "Game
// Quickloaded successfully." into the chat log; if no valid record, falls back to
// RunLoadGameDialog (0x092500). Re-homed from the ApiCaller stubs at 100%.
RVA(0x00092710, 0x77)
i32 CGruntzMgr::Quickload() {
    if (m_saveSink == 0) {
        return 0;
    }
    if (m_timer) {
        m_timer->DtorBody();
    }
    if (m_saveInfoRec && (m_saveInfoRec->m_flags & 1)) {
        // The +0x58 sink IS CSaveGame; Check == CSaveGame::VerifySlot (0xe52c0) and
        // the record IS a SaveSlot (elsewhere this file casts g_gameReg->m_saveSink
        // to CSaveGame). Bind to the real callee so the reloc is faithful.
        if (((CSaveGame*)m_saveSink)->VerifySlot((SaveSlot*)m_saveInfoRec) == 0) {
            return 1;
        }
        g_pPostMessageA((i32)m_gameWnd->m_hwnd, 0x111, 0x807e, 0);
        m_chatLog->AddItem("Game Quickloaded successfully.", 0, 0x11);
        return 1;
    }
    return RunLoadGameDialog();
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
// CGruntzMgr::DebugJumpLevel (0x08e780; ret). Shows the "DEBUG_JUMPLEVEL" modal
// dialog (flag 1); on a positive level number, jumps the play state to it via
// PassClickToPlayState(level, 0, 1). Returns 0 when the dialog is cancelled.
RVA(0x0008e780, 0x2a)
i32 CGruntzMgr::DebugJumpLevel() {
    i32 level = RunModalDialog("DEBUG_JUMPLEVEL", (void*)LevelNumberDialogProcThunk, 1);
    if (level > 0) {
        return PassClickToPlayState(level, 0, 1);
    }
    return 0;
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
// CGruntzMgr::ResetOptionsSlot (0x092da0; ret 4). Reset the indexed options slot
// when it is in range and loaded (m_20 != 0); returns Reset()'s result, else 0.
RVA(0x00092da0, 0x3a)
i32 CGruntzMgr::ResetOptionsSlot(i32 idx) {
    if ((u32)idx >= 4) {
        return 0;
    }
    OptionsSlot* s = (OptionsSlot*)&m_options[idx];
    if (s == 0) {
        return 0;
    }
    if (s->m_20 == 0) {
        return 0;
    }
    // The options slot IS a GruntzPlayer; Reset == GruntzPlayer::Reset (0xda9e0).
    return ((GruntzPlayer*)s)->Reset();
}

// -------------------------------------------------------------------------
// CGruntzMgr::ResetAllOptionsSlots (0x092df0). Reset all four options slots
// unconditionally (each guarded by a non-null check).
RVA(0x00092df0, 0x24)
void CGruntzMgr::ResetAllOptionsSlots() {
    OptionsSlot* s = (OptionsSlot*)&m_options[0];
    for (i32 d = 4; d != 0; d--) {
        if (s != 0) {
            ((GruntzPlayer*)s)->Reset(); // options slot IS GruntzPlayer (Reset 0xda9e0)
        }
        s = (OptionsSlot*)((char*)s + 0x238);
    }
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

// CGruntzMgr::Close (@0x0855e0) is the large member-teardown method the
// dtor calls; its body is still the stub (src/Stub/CGruntzMgr.cpp). It is only
// DECLARED here (in the header) - the dtor's call to it is an external ref whose
// reloc objdiff masks, so no definition is needed in this TU.

// -------------------------------------------------------------------------
// CGruntzMgr::~CGruntzMgr  (virtual; vtable slot 0; own vftable @0x5e9b64)
// The own body just runs Close(); the compiler then destructs the five
// destructible members (m_options, m_strMoviePath, m_strEC, m_stateStack, m_strWorldFile, in
// reverse-construction order) and chains the base ~CGameMgr - all under the /GX
// C++ EH frame (per-member unwind states 4..0).
RVA(0x00083360, 0xb2)
CGruntzMgr::~CGruntzMgr() {
    Close();
}

// WAP32::CGameMgr::Wap32GameMgrVfunc3 (0x85560, base vtable 0x5e9b8c slot 3): the
// "active?" gate - nonzero iff a game window is bound (m_gameWnd != 0).
RVA(0x00085560, 0xb)
i32 WAP32::CGameMgr::Wap32GameMgrVfunc3() {
    return m_gameWnd != 0;
}

// WAP32::CGameMgr::HandleCommand (0x85580, base vtable 0x5e9b8c slot 5): the base
// no-op command sink (CGruntzMgr overrides it at 0x862f0). Returns 0 (unhandled).
RVA(0x00085580, 0x5)
i32 WAP32::CGameMgr::HandleCommand(i32, i32, i32) {
    return 0;
}

// The WAP32::CGameMgr scalar-deleting destructor (??_G, vtable 0x5e9b8c slot 0, 0x855a0):
// cl auto-emits it from CGameMgr's virtual dtor; @rva-symbol names the auto-emitted thunk
// at this RVA (homed by matcher-5, unmatched sweep).
// @rva-symbol: ??_GCGameMgr@WAP32@@UAEPAXI@Z 0x000855a0 0x24

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
    CGameLevel* view = m_world->m_24;
    float fw = (float)(view->m_planeCtx.maxX - view->m_planeCtx.minX + 1);
    float fh = (float)(view->m_planeCtx.maxY - view->m_planeCtx.minY + 1);

    view->m_c8 = (i32)(fw * 1.4f);
    view->m_cc = (i32)(fh * 1.4f);
    view->MainPlaneNotify();

    view = m_world->m_24;
    view->m_d0 = (i32)(fw * 5.3f);
    view->m_d4 = (i32)(fh * 5.3f);
    view->MainPlaneNotify();

    view = m_world->m_24;
    view->m_d8 = (i32)(fw * 1.12f);
    view->m_dc = (i32)(fh * 1.12f);
    view->MainPlaneNotify();

    CGameLevel* v = m_world->m_24;
    if (v->m_mainPlane == 0) {
        return;
    }
    m_viewOriginL = ((CViewport*)v->m_mainPlane)->m_edgeL - 0x60;
    m_viewOriginT = ((CViewport*)m_world->m_24->m_mainPlane)->m_edgeT - 0x60;
    m_viewOriginR = ((CViewport*)m_world->m_24->m_mainPlane)->m_edgeR + 0x60;
    m_viewOriginB = ((CViewport*)m_world->m_24->m_mainPlane)->m_edgeB + 0x60;
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
// (options loop, m_cmdGrid/source/m_cmdSubMgr/m_tileGrid/hook/m_scoreHud) match byte for byte. Retail
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
            m_timer->Tick();
            break;
        case 7:
            if (PrepCmd7(a0) == 0) {
                return 0;
            }
            m_timer->Tick();
            break;
    }

    OptionsSlot* slot = (OptionsSlot*)m_options;
    for (i32 i = 0; i < 4; i++) {
        if (slot == 0 || ((CTriggerMgr*)slot)->RebuildOverlay((void*)a0, cmd, a2, a3) == 0) {
            return 0;
        }
        slot = (OptionsSlot*)((char*)slot + 0x238);
    }

    if (m_cmdGrid->RebuildOverlay((void*)a0, cmd, a2, a3) == 0) {
        return 0;
    }
    if (((CTriggerMgr*)PickPlayOrPausedState())->RebuildOverlay((void*)a0, cmd, a2, a3) == 0) {
        return 0;
    }
    if (((CTriggerMgr*)m_cmdSubMgr)->RebuildOverlay((void*)a0, cmd, a2, a3) == 0) {
        return 0;
    }
    if (m_tileGrid->MapCommand(a0, cmd, a2, a3) == 0) {
        return 0;
    }
    if (CmdHook(a0, cmd, a2, a3) == 0) {
        return 0;
    }
    return m_scoreHud->Command(a0, cmd, a2, a3) != 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::UpdateScoreHud (0x0860b0; ret). Per-frame score/HUD accumulator,
// active only while the registry's gate (g_gameReg->m_134) is 1. Folds this
// world's score/time deltas (m_cmdGrid's +0x20c/+0x21c tables, indexed by the active
// world g_curPlayer) into the +0x7c HUD accumulators. If a level name is loaded
// (m_strWorldFile non-empty) it just refreshes the HUD with 1 and marks it dirty;
// otherwise, on the first frame (m_hudGuard->m_124 == 0) it seeds the HUD from the
// registry's cumulative score and fires the score-bump / tick / notify chain,
// then refreshes the HUD with the live score and clears the dirty flag.
// @early-stop
// inline-budget ripple (was 100%): folding the m_scoreHud ScoreHud view onto the
// real CBattlezData member (correct model - methods + fields match) removed the
// per-call (CBattlezData*) casts, lightening this fn's IR enough that MSVC now
// inlines the (unchanged) CSaveGame::SetCurLevel/SetMaxLevel calls retail keeps
// out-of-line. Diffuse codegen artifact, not a wrong shape; deferred to the sweep.
RVA(0x000860b0, 0xe8)
void CGruntzMgr::UpdateScoreHud() {
    if (g_gameReg->m_134 != 1) {
        return;
    }
    ScoreSub2c* sub = (ScoreSub2c*)g_gameReg->m_curState;

    m_scoreHud->m_1c += m_cmdGrid->m_rowStateB[g_curPlayer];
    m_scoreHud->m_20 += m_cmdGrid->m_rowStateC[g_curPlayer];

    if (m_strWorldFile.GetLength() != 0) {
        m_scoreHud->SetCount(1);
        m_scoreHud->m_08 = 1;
        return;
    }

    if (m_hudGuard->m_124 == 0) {
        m_scoreHud->FillRecord(sub->m_1c, 0);
        ((CSaveGame*)g_gameReg->m_saveSink)->SetCurLevel(sub->m_1c);
        ((CSaveGame*)g_gameReg->m_saveSink)->SetMaxLevel((sub->m_1c % 0x28) + 1);
        ((CSaveGame*)g_gameReg->m_saveSink)->Save(0, 0x81a6);
    }
    m_scoreHud->SetCount(sub->m_1c);
    m_scoreHud->m_08 = 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::SaveState (0x093620; ret 4; returns 1/0). Streams the whole
// game-clock / scroll / warp snapshot through the supplied archive. Bails (0)
// when no archive or no loaded world. Bumps the global save serial, writes the
// world file name (m_strWorldFile, copied into a fresh 0x80 buffer), then the per-frame
// state block (m_114..m_13c) and the clock/scroll/warp globals - each via the
// archive's Serialize(&field, size) slot. Returns 1.
RVA(0x00093620, 0x254)
i32 CGruntzMgr::SaveState(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    if (m_world == 0) {
        return 0;
    }
    g_serialCounter++;

    char buf[0x80];
    memset(buf, 0, 0x80);
    strcpy(buf, m_strWorldFile);
    ar->Write(buf, 0x80);

    ar->Write(&m_114, 4);
    ar->Write(&m_inputFlag, 4);
    ar->Write(&m_128, 4);
    ar->Write(&m_12c, 4);
    ar->Write(&m_130, 4);
    ar->Write(&m_134, 4);
    ar->Write(&m_optionsCount, 4);
    ar->Write(&m_viewOriginL, 0x10); // the 0x10-byte view-origin block (+0x13c..+0x148)
    ar->Write(&g_645580, 4);
    ar->Write(&g_645584, 4);
    ar->Write(&g_645588, 4);
    ar->Write(&g_64558c, 4);
    ar->Write(&g_645590, 4);
    ar->Write(&g_645594, 4);
    ar->Write(&g_645598, 4);
    ar->Write(&g_64559c, 4);
    ar->Write(&g_6455a0, 4);
    ar->Write(&g_traitorMode, 4);
    ar->Write(&g_gruntCreation, 4);
    ar->Write(&g_gruntDestruction, 4);
    ar->Write(&g_gooPuddlez, 4);
    ar->Write(&g_explosionz, 4);
    ar->Write(&m_isEasyMode, 4);
    ar->Write(&g_monologoShown, 4);
    ar->Write(&g_jitterX, 4);
    ar->Write(&g_jitterY, 4);
    ar->Write(&g_panMinX, 4);
    ar->Write(&g_panMaxX, 4);
    ar->Write(&g_warpX, 4);
    ar->Write(&g_warpY, 4);
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
i32 CGruntzMgr::LoadState(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    g_serialCounter++;

    char buf[0x80];
    ar->Read(buf, 0x80);
    m_strWorldFile = buf;

    ar->Read(&m_114, 4);
    ar->Read(&m_inputFlag, 4);
    ar->Read(&m_128, 4);
    ar->Read(&m_12c, 4);
    ar->Read(&m_130, 4);
    ar->Read(&m_134, 4);
    ar->Read(&m_optionsCount, 4);
    ar->Read(&m_viewOriginL, 0x10); // view-origin block (+0x13c..+0x148)
    ar->Read(&g_645580, 4);
    ar->Read(&g_645584, 4);
    ar->Read(&g_645588, 4);
    ar->Read(&g_64558c, 4);
    ar->Read(&g_645590, 4);
    ar->Read(&g_645594, 4);
    ar->Read(&g_645598, 4);
    ar->Read(&g_64559c, 4);
    ar->Read(&g_6455a0, 4);
    ar->Read(&g_traitorMode, 4);
    ar->Read(&g_gruntCreation, 4);
    ar->Read(&g_gruntDestruction, 4);
    ar->Read(&g_gooPuddlez, 4);
    ar->Read(&g_explosionz, 4);
    ar->Read(&m_isEasyMode, 4);
    ar->Read(&g_monologoShown, 4);
    ar->Read(&g_jitterX, 4);
    ar->Read(&g_jitterY, 4);
    ar->Read(&g_panMinX, 4);
    ar->Read(&g_panMaxX, 4);
    ar->Read(&g_warpX, 4);
    ar->Read(&g_warpY, 4);
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
    char* src = (char*)PickPlayOrPausedState();
    if (src == 0) {
        return 0;
    }
    // Consume GetWorldFileName()'s returned CString temp inline (no named local):
    // MSVC reads m_pszData straight off the live return pointer in eax (`mov
    // edi,[eax]`) instead of re-addressing a stack slot, and - with no throwing
    // call live during the temp's range (the strcpy is inlined rep-movs) - /GX
    // still elides the EH frame, matching retail's frameless body. (0x928c0 returns
    // CString by value -> MSVC emits it out-of-line + CALLs it here, as at every
    // other site; the old separate GetLevelName alias for that call is dissolved.)
    strcpy(dst->m_levelName, GetWorldFileName());
    dst->m_isWon = (m_134 == 3);
    dst->m_f8 = m_130;
    // The +0x58 sink IS CSaveGame; Store == CSaveGame::CopySlot (0xe51d0) copying the
    // source state's SaveSlot block (+0x1d0) into the record. Bind the real callee.
    ((CSaveGame*)m_saveSink)->CopySlot((SaveSlot*)dst, (const SaveSlot*)(src + 0x1d0));
    m_saveInfoRec = dst;
    if (snapshot) {
        strncpy((char*)dst->m_snapshot, (char*)snapshot, 0x20);
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
            m_frameGate = 1;
            RunWinHook();
            m_frameGate = 0;
            return 1;
        }
    }

    if (full) {
        if (m_inputState) {
            m_inputState->Stop();
        }
        if (m_world) {
            CSndHost* sub = m_world->m_28;
            if (sub && sub->m_2c) {
                sub->m_2c->Stop();
            }
        }
        if ((m_sound->m_pCurrent ? m_sound->m_pCurrent->IsBusy() : 0) && stopBank) {
            m_sound->StopAll();
        }
        m_curState->Vslot18();
        if (full) {
            return 1;
        }
    }

    if (m_musicEnabled) {
        if (CheckLevelActive()) {
            m_sound->StopBank(1);
        }
    }
    if (m_soundEnabled) {
        m_inputState->Resume();
        if (m_cmdGrid && m_soundEnabled) {
            m_cmdGrid->DestroyAllAnims();
        }
    }
    m_curState->Vslot19();
    g_645570->Flush();
    CGruntzMgr::PerFrameTick();
    return 1;
}

// -------------------------------------------------------------------------
// The game-manager singleton (*0x64556c) - the CGruntzMgr view of the 0x24556c datum
// used by the modal reporters below. (SerializeSyncMarker, the free 0x13610 serialize
// validator that also used this, is split out to SerializeSyncMarker.cpp.)
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg; // 0x64556c

// -------------------------------------------------------------------------
// CGruntzMgr::EnterModalUI (0x08ef10; ret 4). Suspends the in-game world for a
// modal screen: stops the +0x60 timer if any, forces a map redraw + ticks the
// world's dispatch object, then brings the hardware cursor visible
// (while (ShowCursor(TRUE) < 0)), runs the modal handler on the app
// (m_8->RunModal(msg, hwnd)) with the cursor-busy gate raised, clears the gate,
// and - if the cursor was already shown on entry - hides it again
// (while (ShowCursor(FALSE) >= 0)). No-op when there is no app bound.
// @early-stop
// regalloc tiebreak (~93%): logic + the cached ShowCursor fn-ptr are exact; MSVC
// here keeps `this` in edi and the fn-ptr in esi, retail does the reverse (esi
// for `this`, edi for the ptr) - a pure esi<->edi naming swap (call ff d6 vs
// ff d7). Not steerable from source; see docs/patterns/zero-register-pinning.md.
RVA(0x0008ef10, 0x9e)
void CGruntzMgr::EnterModalUI(const char* msg) {
    CGameApp* app = m_owner;
    if (app == 0) {
        return;
    }
    if (m_timer) {
        m_timer->Stop();
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
    app->RunModal(msg, m_gameWnd->m_hwnd);
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
i32 CGruntzMgr::ExitModalUI(CDialog* dlg, i32 notify) {
    if (m_timer) {
        m_timer->Stop();
    }
    if (m_cmdGrid && m_soundEnabled) {
        m_cmdGrid->DestroyAllAnims();
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
    i32 result = dlg->DoModal(); // vtable slot 48 (+0xc0) - see the proof in GruntzMgr.h
    g_64557c = 0;
    m_modalBusy = 0;
    if (m_curState && notify) {
        m_curState->Vslot06();
    }

    if (shown <= 0) {
        while (show(0) >= 0) {
        }
    }

    CGruntzMgr::PerFrameTick();
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
    CState* next = TopState();
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
    PopTopIfMatches(next);
    if (m_curState->Vslot09(oldId) == 0 && m_curState->Vslot06() == 0) {
        return 0;
    }
    m_owner->m_running = 1;
    CGruntzMgr::PerFrameTick();
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
        if (((CPlay*)m_curState)->Vslot1e(a0, a2) == 0) {
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
        CSndHost* sub = m_world->m_28;
        if (sub) {
            SoundStream* obj = sub->m_2c;
            if (obj) {
                obj->Stop(); // direct call 0x137a80 (SoundStream::Stop)
            }
        }
    }
    CGruntzSoundZ* snd = m_sound;
    if (snd && (snd->m_pCurrent ? snd->m_pCurrent->IsBusy() : 0)) {
        m_sound->IsPlaying();
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::StoreInputFlag (0x0919d0; ret 4). Records the input flag at +0x11c;
// when the loaded world's +0x28 sub-object is present it also mirrors the flag
// into the g_sndCueTag global; finally forwards the flag to the +0x54 input object.
RVA(0x000919d0, 0x30)
void CGruntzMgr::StoreInputFlag(i32 v) {
    m_inputFlag = v;
    if (m_world && m_world->m_28) {
        g_sndCueTag = v;
    }
    CWorldSoundSet* in = m_inputState;
    if (in) {
        in->Restart((void*)v); // "StoreFlag" IS CWorldSoundSet::Restart @0xbc30 (stores v at +0x04)
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

// CGruntzMgr::GetWorldFileName (0x000928c0) is now an inline member in the header.

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
            slot->m_38.Method_025d90();
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
// the slot whose index hits the active world g_curPlayer is the "current" one - it
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
        eq = (strcmp(s, m_strWorldFile) == 0);
        if (eq) {
            matched = 1;
        }
    }
    srand((u32)time(0));
    g_6455fc = 0;

    i32 idx = 0;
    CBattlezMapConfig* tick = &((OptionsSlot*)m_options)->m_38;
    i32* cfgp = &((OptionsSlot*)m_options)->m_10;
    i32* arm = &((OptionsSlot*)m_options)->m_14;
    for (i32 i = 0; i < m_optionsCount; i++) {
        i32 cfg;
        if (idx == g_curPlayer) {
            *arm = 1;
            cfg = *cfgp;
            if (matched) {
                cfg = 0;
            }
            if (!tick->LoadConfig((CLevelInfo*)this, idx, cfg)) {
                return 0;
            }
            tick->Clear_02ade0();
            arm = (i32*)((char*)arm + 0x238);
            cfgp = (i32*)((char*)cfgp + 0x238);
            idx++;
            tick = (CBattlezMapConfig*)((char*)tick + 0x238);
            *arm = 0;
            cfg = *cfgp;
            if (matched) {
                cfg = 0;
            }
            if (!tick->LoadConfig((CLevelInfo*)this, idx, cfg)) {
                return 0;
            }
        } else {
            *arm = 0;
            cfg = *cfgp;
            if (matched) {
                cfg = 0;
            }
            if (!tick->LoadConfig((CLevelInfo*)this, idx, cfg)) {
                return 0;
            }
        }
        idx++;
        arm = (i32*)((char*)arm + 0x238);
        cfgp = (i32*)((char*)cfgp + 0x238);
        tick = (CBattlezMapConfig*)((char*)tick + 0x238);
    }
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::SetCellHeight (0x111ec0) lives in GruntzMgr2.cpp (a separate retail
// object far out at 0x111ec0 among the tile-trigger-switch-logic .text block).

// -------------------------------------------------------------------------
// CGruntzMgr::Close (0x0855e0; vtbl slot 2). The full manager teardown the
// dtor runs: deregister the world's mode-reset callback, flush the live config block
// (each WriteInt names one setting), clear the state stack + delete the live state,
// then tear down + delete every owned sub-object (most a parameterless thiscall +
// operator delete; m_30/m_3c through their own vtable slot 1; m_settings the writer; the
// two engine state singletons), and finally chain the base CGameMgr::Close and
// drop the registry singleton.
// @early-stop
// big mechanical teardown (~512 named-extern set): the control flow + the WriteInt
// settings block + the per-member delete ladder are reconstructed. The residual is
// the long run of reloc-masked teardown thiscalls (each owned sub-object's Teardown
// is a distinct engine address modeled as one shared EngObj symbol, so those relocs
// stay fuzzy until each is named) - the documented reloc-typing wall, not a code diff.
RVA(0x000855e0, 0x448)
void CGruntzMgr::Close() {
    if (m_world) {
        ((CDDrawSurfaceMgr*)m_world)->SetHwnd(0);
    }
    OpenSettingsStore();
    CSettingsWriter* cfg = m_settings;
    if (cfg) {
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Num_Runs", m_numRuns);
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Num_Movies", m_numMovies);
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Sound", m_soundEnabled);
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Voice", m_isVoiceEnabled);
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Ambient", m_isAmbientEnabled);
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Music", m_musicEnabled);
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Interlaced", m_isInterlaced);
        ((Utils::RegistryHelper*)cfg)->SetValueDword("High_Detail", m_isHighDetail);
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Effects", m_isEffectsEnabled);
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Disable_Joystick", g_6455c8);
        if (m_sound) {
            ((Utils::RegistryHelper*)cfg)->SetValueDword("Music_Volume", m_sound->GetXMidiVolume());
        }
        if (m_timer) {
            ((Utils::RegistryHelper*)cfg)->SetValueDword("Voice_Volume", m_timer->m_2c);
        }
        if (m_world && m_world->m_28) {
            ((Utils::RegistryHelper*)cfg)->SetValueDword("Sound_Volume", g_sndCueTag);
        }
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Scroll_Speed", m_scrollSpeed);
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Easy_Mode", m_isEasyMode);
        i32 res = RES_640x480;
        if (m_savedModeW == 0x400 && m_savedModeH == 0x300) {
            res = RES_1024x768;
        } else if (m_savedModeW == 0x320 && m_savedModeH == 0x258) {
            res = RES_800x600;
        }
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Resolution", res);
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Checkpoint_Prompts", m_isCheckpointPrompts);
        ((Utils::RegistryHelper*)cfg)
            ->SetValueDword("Enable_HiColor", m_colorDepth == 0x10 ? 1 : 0);
        ((Utils::RegistryHelper*)cfg)->SetValueDword("Enable_TrueColor", 0);
    }
    ClearStateStack();
    if (m_curState) {
        delete m_curState;
        m_curState = 0;
    }
    if (m_spriteFactory) {
        m_spriteFactory->Reset(); // CSpriteRefTable::Reset @0xe2290 (free both buckets)
        operator delete(m_spriteFactory);
        m_spriteFactory = 0;
    }
    if (m_cmdGrid) {
        m_cmdGrid->~CTriggerMgr(); // real dtor (thunk 0x3b1b; body in the trigger eh TU)
        operator delete(m_cmdGrid);
        m_cmdGrid = 0;
    }
    if (m_tileGrid) {
        // MISBOUND FIX: this used to call ~CTriggerMgr (thunk 0x3b1b) on the +0x70 object -
        // a real but WRONG rva. Retail's +0x70 teardown leg calls thunk 0x35b7 ->
        // ~CGruntzMapMgr @0x85d10. +0x70 IS a CGruntzMapMgr, so the cast is gone too.
        m_tileGrid->~CGruntzMapMgr();
        operator delete(m_tileGrid);
        m_tileGrid = 0;
    }
    if (m_scoreHud) {
        m_scoreHud->Teardown();
        operator delete(m_scoreHud);
        m_scoreHud = 0;
    }
    if (m_cmdSubMgr) {
        // MISBOUND FIX (same bug as the +0x70 leg): this called ~CTriggerMgr (thunk 0x3b1b),
        // a real but WRONG rva. Retail's +0x6c teardown leg calls thunk 0x4066 ->
        // ~CGruntzCmdMgr @0x85bd0 (already 100% EXACT in src). +0x6c IS a CGruntzCmdMgr.
        m_cmdSubMgr->~CGruntzCmdMgr();
        operator delete(m_cmdSubMgr);
        m_cmdSubMgr = 0;
    }
    if (g_645578) {
        StateMgrBZ* v = g_645578;
        v->m_0 = 0;
        v->m_4 = 0;
        v->m_8 = 0;
        v->m_10 = 0;
        v->m_14 = 0;
        operator delete(v);
        g_645578 = 0;
    }
    if (g_645570) {
        g_645570->Teardown();
        operator delete(g_645570);
        g_645570 = 0;
    }
    if (m_hudGuard) {
        m_hudGuard->Teardown();
        operator delete(m_hudGuard);
        m_hudGuard = 0;
    }
    if (m_sound) {
        m_sound->Shutdown();
        operator delete(m_sound);
        m_sound = 0;
    }
    if (m_inputState) {
        m_inputState->Teardown();
        operator delete(m_inputState);
        m_inputState = 0;
    }
    if (m_40) {
        ((CTriggerMgr*)m_40)->~CTriggerMgr();
        operator delete(m_40);
        m_40 = 0;
    }
    if (m_chatLog) {
        m_chatLog->~CFontConfig();
        operator delete(m_chatLog);
        m_chatLog = 0;
    }
    if (m_timer) {
        m_timer->Teardown();
        operator delete(m_timer);
        m_timer = 0;
    }
    if (m_world) {
        ((CWorldDelete*)m_world)->Slot1(1);
        m_world = 0;
    }
    if (m_recolorSurface) {
        m_recolorSurface->Teardown();
        operator delete(m_recolorSurface);
        m_recolorSurface = 0;
    }
    if (m_settings) {
        ((CTriggerMgr*)m_settings)->~CTriggerMgr();
        operator delete(m_settings);
        m_settings = 0;
    }
    if (m_3c) {
        m_3c->Slot1(1);
        m_3c = 0;
    }
    if (m_50) {
        ((CTriggerMgr*)m_50)->~CTriggerMgr();
        operator delete(m_50);
        m_50 = 0;
    }
    if (m_saveSink) {
        ((CTriggerMgr*)m_saveSink)->~CTriggerMgr();
        operator delete(m_saveSink);
        m_saveSink = 0;
    }
    if (m_logicPump) {
        m_logicPump->Reset(); // CLightFxMgr::Reset @0x9dc80 (zero the bound ptrs + tables)
        operator delete(m_logicPump);
        m_logicPump = 0;
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
    this->CGameMgr::Close();
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
        if (m_cmdGrid->m_288 == 1) {
            UpdateScoreHud();
        }
        TransitionState(0xa, 1, 0, 0);
        return;
    }
    ((GameRegHudView*)g_gameReg)->m_7c->SetCount(((StateScoreView*)st)->m_1c);
    if (m_134 == 3) {
        LevelClock* clk = ((StateScoreView*)st)->m_3f4;
        i64 d = (i64)g_645588 - clk->m_38;
        ((GameRegHudView*)g_gameReg)->m_7c->m_score += (d < 0) ? 0 : (i32)d;
        TransitionState(0x12, 1, 0, 0);
        return;
    }
    CBattlezData* hud = ((GameRegHudView*)g_gameReg)->m_7c;
    u32 now = g_pTimeGetTime();
    hud->m_score += (now - g_648ce8);
    TransitionState(0x12, 1, 0, 0);
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
    if (ExitModalUI(&dlg, 0) == 1) {
        g_pSendMessageA((i32)m_gameWnd->m_hwnd, 0x111, 0x80cf, 0);
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::DelayedQuit (0x08f530; the PostMessageA-wrapping delayed shutdown).
// One-shot (guarded by m_a4): resolve the world's "MENU_ACTIVATE" menu node to a base
// timestamp (its +0x10->+0x28 plus 0x1f4), busy-wait via timeGetTime until that
// deadline passes, clear the app's resume flag (m_running), then PostMessageA WM_CLOSE to
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
    LeafCue* out = 0;
    ((CMapStringToPtr*)&m_world->m_28->m_10)->Lookup("MENU_ACTIVATE", (void*&)out);
    i32 base;
    if (out != 0) {
        ((CMapStringToPtr*)&m_world->m_28->m_10)->Lookup("MENU_ACTIVATE", (void*&)out);
        base = out->m_10->m_28 + 0x1f4; // cue duration + 500ms: wait out the cue
    } else {
        base = 0;
    }
    u32(WINAPI * tgt)() = g_pTimeGetTime;
    u32 deadline = base + tgt();
    while (tgt() < deadline) {
    }
    if (m_owner) {
        m_owner->m_running = 0;
    }
    if (m_gameWnd) {
        g_pPostMessageA((i32)m_gameWnd->m_hwnd, 0x10, 0, 0);
    }
}

// The Portal companion path resolver + process spawner (PortalPath.cpp, both
// __stdcall free functions; reloc-masked here).
i32 __stdcall LaunchPortalExe(char* outPath);
i32 __stdcall LaunchProcessInDir(char* exe, char* dir);

// CGruntzMgr::LaunchPortal (0x0907c0, ret 4). Resolve the installed Portal
// companion's exe path into a local buffer; if it resolves and the buffer is
// non-empty, spawn the process in place; on a successful spawn, optionally
// schedule the delayed shutdown (`quitAfter`). Returns 1 on launch, 0 otherwise.
// @early-stop
// byte-proven (llvm-objdump -dr base vs target): the ONLY difference is a single
// dead `mov ecx,esi` retail emits BEFORE the LaunchProcessInDir call site. The
// retail source invoked LaunchProcessInDir through a `this`-qualified / member
// expression (a __thiscall member that ignores `this`, which Ghidra mislabels
// __stdcall since it never reads ecx), so the caller materializes this=esi->ecx;
// our free-function call omits it. All three reloc operands (LaunchPortalExe /
// LaunchProcessInDir / DelayedQuit) already match; that one 2-byte instruction
// shifts the tail and caps this at 97.37%. Keeping the free-fn model (exact callee
// symbol resolution) over a fake member-view that would only reloc-mask.
RVA(0x000907c0, 0x77)
i32 CGruntzMgr::LaunchPortal(i32 quitAfter) {
    char path[256];
    path[0] = 0;
    if (!LaunchPortalExe(path)) {
        return 0;
    }
    if (path[0] == 0) {
        return 0;
    }
    if (!LaunchProcessInDir(path, 0)) {
        return 0;
    }
    if (quitAfter) {
        DelayedQuit();
    }
    return 1;
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
        m_timer->Stop();
    }
    if (m_cmdGrid && m_soundEnabled) {
        m_cmdGrid->DestroyAllAnims();
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
    i32 result =
        g_pDialogBoxParamA((i32)m_owner->m_hInstance, tmpl, (i32)m_gameWnd->m_hwnd, dlgProc, 0);
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
// path Quicksave falls into: when the first-frame guard (m_hudGuard->m_124) is set it pops a
// localized message (resource 0x81aa) through a modal; otherwise it runs the GAME_SAVE
// dialog and, on success, the GAME_SAVEMSG dialog. Returns 1.
RVA(0x00092420, 0xa4)
i32 CGruntzMgr::LoadSaveMessageSprite() {
    if (m_hudGuard->m_124 != 0) {
        CString name;
        name.LoadStringA(0x81aa);
        EnterModalUI(name);
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
    CBattlezDlg dlg((i32)this, 0); // ctor 0x14b30 (a0 = this, pParent = null)
    i32 st = m_curState->Update();
    if (st != 5 && st != 2 && st != 3 && st != 7) {
        return 0;
    }
    ChannelSlots_InitAll();
    if (ExitModalUI(&dlg, 1) != 1) {
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
    g_pPostMessageA((i32)m_gameWnd->m_hwnd, 0x111, 0x80e3, 0);
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
    m_recolorSurface = 0;
    m_settings = 0;
    m_scoreHud = 0;
    m_3c = 0;
    m_40 = 0;
    m_hudGuard = 0;
    m_sound = 0;
    m_4c = 0;
    m_50 = 0;
    m_64 = 0;
    m_lobby = 0;
    m_inputState = 0;
    m_saveSink = 0;
    m_chatLog = 0;
    m_timer = 0;
    m_cmdGrid = 0;
    m_cmdSubMgr = 0;
    m_tileGrid = 0;
    m_spriteFactory = 0;
    m_logicPump = 0;
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
    m_musicEnabled = 1;
    m_soundEnabled = 1;
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
    ((CDirectDrawMgr*)m_world->m_1c)->FindFwd((CDdModePair*)&pt, m_modeW, m_modeH, m_colorDepth);
    CPointXY* p = &pt;
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
    ((CDirectDrawMgr*)m_world->m_1c)->FindBack((CDdModePair*)&pt, m_modeW, m_modeH, m_colorDepth);
    CPointXY* p = &pt;
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
// 0x8e3a0 (RVA-homed from src/Stub/ApiCallers.cpp) - __thiscall(out): default to the
// full 0x280x0x1e0 screen rect, or the active viewport's rect (m_30->m_24 + 0x10) when
// one is set; write it to *out.
// @orphan: a shared viewport-rect helper called on divergent `this` from many owners
// (CSingleFrameMessage ctor, CPlay::Render/DrawDebugStats/Profile*) - no single class
// owns it; RVA-adjacent .obj not pinned. Kept as a placeholder view.
struct ViewObj_08e3a0 {
    char m_pad0[0x24];
    char* m_24; // +0x24 (its +0x10 is a RECT)
};
struct RectQuery_08e3a0 {
    char m_pad0[0x30];
    ViewObj_08e3a0* m_30; // +0x30
    void GetRect(RECT* out);
};
SIZE_UNKNOWN(ViewObj_08e3a0);
SIZE_UNKNOWN(RectQuery_08e3a0);
RVA(0x0008e3a0, 0x94)
void RectQuery_08e3a0::GetRect(RECT* out) {
    RECT local;
    SetRect(&local, 0, 0, 0x27f, 0x1df);
    if (!m_30) {
        *out = local;
        return;
    }
    local = *(RECT*)(m_30->m_24 + 0x10);
    *out = local;
}

// ---------------------------------------------------------------------------
// The developer option-dialog Win32 DialogProc callbacks (ex AppDialogs.cpp;
// merged wave3-J - the FLAGS table's 0x08b8c0-0x093ce7 group gruntzmgr+playdtor+
// appdialogs is ONE obj, GruntzMgr.cpp; the ex-"appdialogs" base profile unifies
// to this TU's /GX). Free __stdcall INT_PTR CALLBACK(HWND, UINT, WPARAM, LPARAM)
// procs the engine hands to the USER32 dialog manager.
//
// WarpDialogProc backs the developer "warp" cheat dialog: on WM_INITDIALOG it
// seeds two edit controls (0x40e / 0x40f) with the current warp X/Y from the
// game registry; on WM_COMMAND it ends the dialog (IDCANCEL=2) or, on IDOK=1,
// reads the two edit fields back, stores them, and - if checkbox 0x410 is set -
// persists them to the registry as the warp target.
//
// The procs reach three registry slots through AUTHENTIC per-site downcasts
// (their concrete class lives in other clusters, so CGameRegistry keeps the
// base/void* type and the consumer casts):
//   ((ScoreSub2c*)g_gameReg->m_curState)->m_1c  read here as the CURRENT LEVEL
//                        NUMBER (the "Level %i Warp X/Y" key + the go-to-level
//                        seed) - the same +0x1c state slot UpdateScoreHud reads
//                        as the score accumulator (name reconciliation TODO).
//   ((Utils::RegistryHelper*)g_gameReg->m_settings)->SetValueDword(...)
//   ((CGameRegWarp*)g_gameReg->m_world->m_24->m_5c)->m_84 / ->m_88  seed X/Y
struct CGameRegWarp { // reinterpret view of the m_5c viewport (+0x84/+0x88 seed X/Y)
    char m_pad00[0x84];
    i32 m_84;
    i32 m_88;
};

// WarpDialogProc - the warp-cheat dialog callback.
RVA(0x0008e4e0, 0x172)
INT_PTR CALLBACK WarpDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    char szValue[64];

    switch (msg) {
        case WM_INITDIALOG: {
            CGameRegWarp* warp = (CGameRegWarp*)g_gameReg->m_world->m_24->m_5c;
            i32 seedX = warp->m_84;
            i32 seedY = warp->m_88;
            SetDlgItemInt(hDlg, 0x40e, seedX, 0);
            SetDlgItemInt(hDlg, 0x40f, seedY, 0);
            return 1;
        }

        case WM_COMMAND:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                i32 valX = GetDlgItemInt(hDlg, 0x40e, 0, 0);
                i32 valY = GetDlgItemInt(hDlg, 0x40f, 0, 0);
                g_warpX = valX;
                g_warpY = valY;
                if (IsDlgButtonChecked(hDlg, 0x410)) {
                    sprintf(szValue, "Level %i Warp X", ((ScoreSub2c*)g_gameReg->m_curState)->m_1c);
                    ((Utils::RegistryHelper*)g_gameReg->m_settings)->SetValueDword(szValue, valX);
                    sprintf(szValue, "Level %i Warp Y", ((ScoreSub2c*)g_gameReg->m_curState)->m_1c);
                    ((Utils::RegistryHelper*)g_gameReg->m_settings)->SetValueDword(szValue, valY);
                    ((Utils::RegistryHelper*)g_gameReg->m_settings)
                        ->SetValueDword(
                            "Last Warp Level",
                            ((ScoreSub2c*)g_gameReg->m_curState)->m_1c
                        );
                }
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

// LevelNumberDialogProc (0x0008e7c0 / 0x0008e8c0) - two byte-identical developer
// "go to level" dialog callbacks. On WM_INITDIALOG the current level number
// (g_gameReg->m_curState->m_1c) seeds edit control 0x40c; on WM_COMMAND IDCANCEL
// (2) ends the dialog with 0, IDOK (1) ends it with the entered level number
// (GetDlgItemInt of 0x40c). The two procs back two separate dialog resources with
// the same layout, so the compiler emits them identically.
RVA(0x0008e7c0, 0x86)
INT_PTR CALLBACK LevelNumberDialogProc8e7c0(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            SetDlgItemInt(hDlg, 0x40c, ((ScoreSub2c*)g_gameReg->m_curState)->m_1c, 0);
            return 1;
        case WM_COMMAND:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                EndDialog(hDlg, GetDlgItemInt(hDlg, 0x40c, 0, 0));
                return 1;
            }
            break;
    }
    return 0;
}

RVA(0x0008e8c0, 0x86)
INT_PTR CALLBACK LevelNumberDialogProc8e8c0(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            SetDlgItemInt(hDlg, 0x40c, ((ScoreSub2c*)g_gameReg->m_curState)->m_1c, 0);
            return 1;
        case WM_COMMAND:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                EndDialog(hDlg, GetDlgItemInt(hDlg, 0x40c, 0, 0));
                return 1;
            }
            break;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CGruntzMgr::SetVideoMode (0x08df00; ret 0xc). Switch the display to (w,h) at
// the current bit depth (m_colorDepth). No-op if already at (w,h). When the live state is
// playable (Update() in {3,0x11}) and the new size exceeds the loaded map's
// playable field (((CViewport*)m_world->m_24->m_mainPlane)->{m_30,m_34}), it refuses: pokes the HUD
// guts subsystem (m_2dc) and surfaces the "map too small" modal, returning 0.
// Otherwise it applies the mode through the engine, re-hides the cursor, stamps
// m_modeW/m_modeH (+ the saved pair when arg3 is set), re-pokes the guts, runs the
// two post-mode resync siblings, and (once) logs "Resolution is now %ix%ix%i".
//
// The SetVideoMode symbol pairs the @early-stop CheckDisplayBoundsA/B and the
// RestoreVideoMode/CheckSavedMode call sites (previously the Boundary_08df00 stub).
// The loaded map's playable extent ((CViewport*)m_world->m_24->m_mainPlane) is the shared CViewport;
// SetVideoMode reads its +0x30/+0x34 field width/height limits.
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
            CViewport* f = (CViewport*)m_world->m_24->m_mainPlane;
            if (f != 0) {
                if (w > f->m_worldWidth || h > f->m_worldHeight) {
                    CPlay* st = (CPlay*)m_curState;
                    st->ResetViewport();
                    if (st->m_guts != 0) {
                        st->m_guts->m_614 = m_modeH;
                        if (st->m_guts->m_state == 0) {
                            st->m_guts->StepBracketL();
                            st->m_guts->StepBracketR();
                            ReportMapTooSmall(
                                "This map is too small to be displayed under your "
                                "desired video resolution. Default resolution will "
                                "be used."
                            );
                            return 0;
                        }
                        if (st->m_guts->m_state == 1) {
                            st->m_guts->StepBracketR();
                            st->m_guts->StepBracketL();
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
        CPlay* st = (CPlay*)m_curState;
        st->ResetViewport();
        if (st->m_guts != 0) {
            st->m_guts->m_614 = h;
            if (st->m_guts->m_state == 0) {
                st->m_guts->StepBracketL();
                st->m_guts->StepBracketR();
            } else if (st->m_guts->m_state == 1) {
                st->m_guts->StepBracketR();
                st->m_guts->StepBracketL();
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

// ---------------------------------------------------------------------------
// The screen/video-region manager (Open @0x0fe460 / Reset @0x0fe600) lives in
// ScreenRegionMgr.cpp - a separate retail object far from this block, interleaved
// with the CSBI_RectOnly .text at 0x0fe4xx.

SIZE_UNKNOWN(CActiveObj);
SIZE_UNKNOWN(CActiveSub2dc);
SIZE_UNKNOWN(CColorLookup);
SIZE_UNKNOWN(CColorRow);
SIZE_UNKNOWN(CMonoConfigHolder);
SIZE_UNKNOWN(CMonoConfigMap);
SIZE_UNKNOWN(CMonoConfigRec);
SIZE_UNKNOWN(CMonoEntry);
SIZE_UNKNOWN(CMonoSprite);
SIZE_UNKNOWN(CMonoView);
SIZE_UNKNOWN(CMonoWorld);
SIZE_UNKNOWN(CPlayStateView);
SIZE_UNKNOWN(CPointXY);
SIZE_UNKNOWN(CRezSurface94);
SIZE_UNKNOWN(CBattlezDlg);
SIZE_UNKNOWN(CSettingsWriter);
SIZE_UNKNOWN(CWorldCoordResolver);
SIZE_UNKNOWN(CWorldDelete);
SIZE_UNKNOWN(CWorldLookupHolder);
SIZE_UNKNOWN(CWorldModeIface);
SIZE_UNKNOWN(CmdSink);
SIZE_UNKNOWN(DirectInputMgr2);
SIZE_UNKNOWN(EngObj);
SIZE_UNKNOWN(GameRegHudView);
SIZE_UNKNOWN(LevelClock);
SIZE_UNKNOWN(OptionsSlot);
SIZE_UNKNOWN(PlayStatusSlot);
SIZE_UNKNOWN(RegScoreHud);
SIZE_UNKNOWN(ScoreNotifier);
SIZE_UNKNOWN(ScoreSub2c);
SIZE_UNKNOWN(StateMgrBZ);
SIZE_UNKNOWN(StateScoreView);
SIZE_UNKNOWN(SvmGuts);
SIZE_UNKNOWN(SvmStateView);
SIZE_UNKNOWN(TimerObj);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CGameRegWarp);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
VTBL(CDemo, 0x001e9f0c); // vtable_names -> code (RTTI game class; dtor 0x8d0d0 lives here)
