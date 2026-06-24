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
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

namespace Utils {
    namespace WinAPI {
        char GetGruntzDriveLetter();
        int FileExists(char* szPath);
    } // namespace WinAPI
} // namespace Utils

// External DirectPlay surface used by InitializeLobbyConnectionSettings. The
// DPLAYX export DirectPlayLobbyCreate (ordinal 4) and CNetMgr::ReportError are
// reached as `call rel32`/[IAT] - the displacement reloc-masks, so only the
// arg shapes are load-bearing. `m_c4` (the connection-settings buffer) is freed
// through a small NAFXCW-style operator-delete helper (out-of-line, reloc-
// masked); modelled as a plain free function.
extern "C" __declspec(dllimport) long __stdcall DirectPlayLobbyCreate(
    void* lpguidDSP,
    IDirectPlayLobbyZ** lplpDPL,
    void* pUnk,
    void* lpData,
    unsigned long dwDataSize
);

class CNetMgr {
public:
    static void ReportError(char* file, int line, long hr, void* hWnd);
};

void FreeConnectionSettings(void* p); // FUN_005b9b82 (operator delete wrapper)

void* operator new(unsigned int);

// -------------------------------------------------------------------------
// Engine objects reached through CGruntzMgr's member pointers. Each engine-side
// method is a reloc-masked __thiscall (`mov ecx,obj; call rel32`), so only the
// layout offsets + call shapes are load-bearing; the displacements reloc-mask.
extern "C" {
    extern int g_61ab24; // DAT_0061ab24  (last-stored input flag mirror)
    extern int g_64557c; // DAT_0064557c  (modal/cursor-busy gate)
    // The clock/scroll/warp globals SaveState streams through the archive.
    extern int g_629ad0; // DAT_00629ad0  (save serial counter)
    extern int g_64558c; // DAT_0064558c
    extern int g_645590; // DAT_00645590
    extern int g_645594; // DAT_00645594
    extern int g_645598; // DAT_00645598
    extern int g_64559c; // DAT_0064559c
    extern int g_6455a0; // DAT_006455a0
    extern int g_6455e8; // DAT_006455e8
    extern int g_6452a4; // DAT_006452a4
    extern int g_6452cc; // DAT_006452cc
    extern int g_645508; // DAT_00645508
    extern int g_64550c; // DAT_0064550c
}
extern int g_warpX; // ?g_warpX@@3HA
extern int g_warpY; // ?g_warpY@@3HA

extern "C" {
    extern int g_644c54; // DAT_00644c54  (active player/world index)
}

// The game registry singleton (?g_gameReg@@3PAUWwdGameReg@@A), modeled here with
// the offsets UpdateScoreHud touches (a per-TU view; the DATA pin reloc-masks the
// `mov eax,ds:g_gameReg` load against the already-named symbol).
struct ScoreNotifier {          // g_gameReg->m_58
    void Bump(int wp);          // FUN @ 0x4408 thunk (this, wp)
    void Tick(int wp);          // FUN @ 0x1c53 thunk (this, wp)
    void Notify(int a, int wp); // FUN @ 0x2d97 thunk (this, a, wp)
};
struct ScoreSub2c { // g_gameReg->m_2c
    char m_pad0[0x1c];
    int m_1c; // +0x1c  cumulative score
};
struct WwdGameRegZ {
    char m_pad0[0x2c];
    ScoreSub2c* m_2c; // +0x2c
    char m_pad30[0x58 - 0x30];
    ScoreNotifier* m_58; // +0x58
    char m_pad5c[0x134 - 0x5c];
    int m_134; // +0x134  active gate
};
DATA(0x0024556c)
extern WwdGameRegZ* g_gameReg;

// The +0x68 per-world delta tables: two parallel int arrays at +0x20c / +0x21c
// indexed by g_644c54.
struct WorldDeltaTables {
    char m_pad0[0x20c];
    int m_arr20c[4]; // +0x20c..+0x21c
    int m_arr21c[4]; // +0x21c
};

// The +0x7c HUD/score accumulator object.
struct ScoreHud {
    char m_pad0[0x8];
    int m_8; // +0x08  refresh flag
    char m_padc[0x1c - 0xc];
    int m_1c;                // +0x1c  accumulator A
    int m_20;                // +0x20  accumulator B
    void Refresh(int score); // FUN @ 0x1884 thunk (this, score)
    void Seed(int v, int z); // FUN @ 0x1c8f thunk (this, v, z)
};

// The +0x44 object carrying a one-shot guard at +0x124.
struct HudGuard44 {
    char m_pad0[0x124];
    int m_124; // +0x124
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
    virtual void s0b();
    virtual void Serialize(void* data, int size); // slot 12 (+0x30)
};

// The engine reaches the USER32 cursor through a stored function pointer at
// 0x6c44c4 (`mov edi,ds:...; call edi`), not a direct IAT call; model it as a
// typed global fn-ptr so the indirect call shape (`call reg`) falls out.
extern "C" int(__stdcall* g_pShowCursor)(int); // PTR_ShowCursor_006c44c4

// A free helper that flushes/forces a redraw of one map index (reloc-masked).
void RedrawMapIndex(int idx); // FUN_00558c70

// The trailing __cdecl command hook in BroadcastCmd's fan-out (reloc-masked;
// `push a3..a0; call; add esp,0x10`).
int CmdHook(int a, int b, int c, int d); // FUN @ 0x17da thunk

// The +0x60 timer/poll object (reloc-masked thiscall).
struct TimerObj {
    void Stop(); // (this) reloc-masked
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

// One player-status slot in the PLAY state's 4-entry status array (m_2c->+0x520),
// each 0x64 bytes; the status id is at +0x20 (3 == "won/done").
struct PlayStatusSlot {
    char m_pad0[0x20];
    int m_status; // +0x20
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
    int m_f8; // +0xf8
    int m_fc; // +0xfc
};

// The +0x58 manager FillSaveInfo forwards the record + source-state ptr to
// (reloc-masked thiscall).
struct SaveSink58 {
    void Store(SaveInfo* dst, char* src); // (this, dst, src) reloc-masked
};

// The engine's out-of-line block copy (FUN_00520340). Retail calls it here (not
// the inlined rep-movs memcpy intrinsic); modeled as a plain __cdecl free fn so
// the `push n; push src; push dst; call; add esp,0xc` shape falls out.
void EngineCopy(void* dst, void* src, int n); // FUN_00520340

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
    virtual int Run(); // slot 48 (+0xc0)
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
    extern int g_6455fc; // DAT_006455fc  (round-robin options cursor)
}

// The per-frame input/state object at CGruntzMgr +0x54 (reloc-masked thiscall).
struct InputStateObj {
    void StoreFlag(int v); // FUN_004385e0-family (this, v)
};

// One element of the 4-entry options array embedded at CGruntzMgr +0x150 (each
// 0x238 bytes). AdvanceOptionsCycle reads its arm flag (+0x14) + loaded flag
// (+0x20) and ticks the +0x38 sub-object (reloc-masked thiscall); BroadcastCmd
// forwards the 4-arg command into the slot itself.
struct OptionsTickSub {
    void Tick(); // (this) reloc-masked
};
struct OptionsSlot {
    char m_pad0[0x14];
    int m_14; // +0x14  arm flag
    char m_pad18[0x20 - 0x18];
    int m_20; // +0x20  loaded flag
    char m_pad24[0x38 - 0x24];
    OptionsTickSub m_38;                     // +0x38
    int Command(int a, int b, int c, int d); // (this, a..d) reloc-masked
};

// The downstream command sinks BroadcastCmd fans the 4-arg command out to: the
// +0x68 grid, the live source object (via GetSaveSource), the +0x6c sub-mgr, the
// +0x70 polymorphic object (vtbl slot 1), and the +0x7c HUD. Each returns
// nonzero to keep broadcasting. All reloc-masked.
struct CmdSink {
    int Command(int a, int b, int c, int d); // (this, a..d) reloc-masked
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
    virtual int Command(int a, int b, int c, int d); // slot 1 (+0x04)
};

// The loaded world's height/influence grid (reached via m_30->m_24->m_5c). m_20
// is the value grid, m_24 the per-column base offset table.
struct CHeightGrid {
    char m_pad0[0x20];
    int* m_20; // +0x20  value grid
    int* m_24; // +0x24  per-column base table
};

// The active world view at m_30->m_24; its +0x5c holds the height grid.
struct CWorldViewZ {
    char m_pad0[0x5c];
    CHeightGrid* m_5c; // +0x5c
};

// The scroll/camera view at m_30->m_24 (a CWorldView): a tile rect at
// +0x10..+0x1c and the three scaled-extent output pairs at +0xc8..+0xdc that
// RecomputeViewScale fills; +0x5c is a CWorldEdges sub-object (origin fields at
// +0x40..+0x4c). The view's rescale notify is a reloc-masked thiscall.
struct CWorldEdges {
    char m_pad0[0x40];
    int m_40, m_44, m_48, m_4c; // +0x40..+0x4c
};
struct CScrollView {
    char m_pad0[0x10];
    int m_10, m_14, m_18, m_1c; // +0x10..+0x1c  tile rect
    char m_pad20[0x5c - 0x20];
    CWorldEdges* m_5c; // +0x5c
    char m_pad60[0xc8 - 0x60];
    int m_c8, m_cc; // +0xc8/+0xcc  scale-1 extents
    int m_d0, m_d4; // +0xd0/+0xd4  scale-2 extents
    int m_d8, m_dc; // +0xd8/+0xdc  scale-3 extents
    void Notify();  // FUN @ 0x160ee0 (this) reloc-masked
};

// The +0x70 notify object (reloc-masked 3-arg thiscall setter).
struct CNotify70 {
    void Set(int row, int col, int value); // (this, row, col, value) reloc-masked
};

// The engine's __cdecl CString-formatting helper (sprintf-style into a CString
// destination; reloc-masked - only the call shape is load-bearing).
extern "C" void Format(CString* dst, const char* fmt, ...);

// The per-frame draw-clock globals PerFrameTick stamps each tick. g_wap32Now /
// g_wap32FrameDelta are the engine's just-refreshed clock (mangled C++ globals,
// stored into the game-side mirror g_645580/g_645584); g_6bf3c0/g_6bf3bc are the
// draw-clock pair (extern "C" -> the _g_* C symbols). All reloc-masked DATA refs.
extern int g_wap32Now;        // ?g_wap32Now@@3HA
extern int g_wap32FrameDelta; // ?g_wap32FrameDelta@@3HA
extern "C" {
    extern unsigned int g_645580; // game-side now mirror (DAT_00645580)
    extern unsigned int g_645584; // game-side delta mirror (DAT_00645584)
    extern unsigned int g_645588; // game-side abs clock (DAT_00645588)
    extern unsigned int g_6bf3c0; // draw-clock (timeGetTime stamp)
    extern unsigned int g_6bf3bc; // draw-clock delta (cleared)
    // The clock/scroll-state globals ResetClockGlobals zeroes (reloc-masked).
    extern unsigned int g_645600; // DAT_00645600
    extern unsigned int g_6455b0; // DAT_006455b0
    extern unsigned int g_6455a4; // DAT_006455a4
    extern unsigned int g_6455a8; // DAT_006455a8
    extern unsigned int g_6455ac; // DAT_006455ac
    extern unsigned int g_6455f8; // DAT_006455f8
    extern unsigned int g_6455f4; // DAT_006455f4
}

// The two engine input/state singletons TickStateMgrs drives once per call
// (DAT_00645570/DAT_00645578; reloc-masked DATA refs). g_645570 is the
// DirectInputMgr2 (its PollAll @0x533080 is the per-frame device poll); g_645578
// is a second mgr (Flush @0x4385e0). Each call is a single reloc-masked
// __thiscall, so only the one-method shape on a tiny helper is load-bearing.
struct DirectInputMgr2 {
    int PollAll(); // FUN_00533080
    int Flush();   // FUN_00533110 (a second per-frame entrypoint)
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
// Re-asserts the standard 640x480 display mode. If the live mode (m_8c x m_90)
// is already 640x480 it is a no-op success - and when `save` is set it also
// records the mode into the saved/last-good pair (m_94/m_98). Otherwise it
// drives SetVideoMode(640, 480, save); on failure it surfaces a (0x8008, 0x438)
// error and returns 0.
// @early-stop
// constant-CSE/copy-prop wall: retail uses immediate cmp operands + re-loads the
// fields fresh in the save block (no constant propagation); MSVC 5.0 here either
// hoists 0x280/0x1e0 into entry registers (direct-member compare) or copy-props
// the known value into the m_94/m_98 store (local-copy compare). Logic is exact;
// see docs/patterns/constant-cse-immediate-vs-hoist.md.
RVA(0x0008ddd0, 0x7e)
int CGruntzMgr::RestoreVideoMode(int save) {
    int w = m_8c;
    int h = m_90;
    if (w == 0x280 && h == 0x1e0) {
        if (save) {
            m_94 = w;
            m_98 = h;
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
// CGruntzMgr::GetGruntzDriveLetter  (__thiscall)
// Returns the CD-ROM drive letter holding the Gruntz disc, memoised in the
// pair (m_d0 = letter, m_d4 = probed-flag): once probed, return the cached
// letter; otherwise call Utils::WinAPI::GetGruntzDriveLetter(), cache + set the
// flag. (The result is discarded by the engine on the first/uncached path - the
// store IS the return, the cached path returns the byte.)
RVA(0x0008fa70, 0x2c)
char CGruntzMgr::GetGruntzDriveLetter() {
    if (m_d4) {
        return m_d0;
    }
    m_d0 = Utils::WinAPI::GetGruntzDriveLetter();
    m_d4 = 1;
    return m_d0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::CheckPlayState  (__thiscall; retail FUN_0048ec50)
// A small game-state predicate over the active state's Update() id: 0 if there
// is no state, 1 when it reports PLAY (3), else (id == 0x11 ? 1 : 0) - i.e. true
// for the in-game PLAY state or the paused/hold state. AdvanceFrame uses it to
// decide whether the sound bank should keep running. (Update() is re-evaluated
// on the second compare, matching the retail two-call codegen.)
RVA(0x0008ec50, 0x33)
int CGruntzMgr::CheckPlayState() {
    if (m_2c == 0) {
        return 0;
    }
    if (m_2c->Update() == 3) {
        return 1;
    }
    return m_2c->Update() == 0x11;
}

// -------------------------------------------------------------------------
// CGruntzMgr::InitializeLobbyConnectionSettings  (__thiscall)
// One-shot (guarded by m_a0) acquisition of the DirectPlay lobby connection
// settings the game was launched with: create an IDirectPlayLobby (m_c0), then
// pull the launch-time connection-settings blob into a freshly allocated buffer
// (m_c4) using the classic size-probe / fill-buffer two-call idiom. Every COM /
// DPLAYX failure routes a CNetMgr::ReportError diagnostic (with this TU's file +
// the call-site line) through the game window. m_9c records success (1) / failure
// (0); the result also lands in eax for the call site.
RVA(0x0008eca0, 0x164)
int CGruntzMgr::InitializeLobbyConnectionSettings() {
    if (m_a0) {
        return m_9c;
    }

    m_a0 = 1;
    m_9c = 0;

    if (m_c0) {
        m_c0->vtbl->Release(m_c0);
        m_c0 = 0;
    }

    long hr = DirectPlayLobbyCreate(0, &m_c0, 0, 0, 0);
    if (hr) {
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x120d, hr, ((CGameWnd*)m_4)->m_4);
        return 0;
    }
    if (!m_c0) {
        return 0;
    }

    if (m_c4) {
        FreeConnectionSettings(m_c4);
        m_c4 = 0;
    }

    unsigned long dwSize = 0;
    hr = m_c0->vtbl->GetConnectionSettings(m_c0, 0, 0, &dwSize);
    if (hr != 0 && hr != (long)0x8877001e) { // !DPERR_BUFFERTOOSMALL
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x1221, hr, ((CGameWnd*)m_4)->m_4);
        m_c0->vtbl->Release(m_c0);
        m_c0 = 0;
        return 0;
    }

    m_c4 = operator new(dwSize);
    if (!m_c4) {
        m_c0->vtbl->Release(m_c0);
        m_c0 = 0;
        return 0;
    }

    hr = m_c0->vtbl->GetConnectionSettings(m_c0, 0, m_c4, &dwSize);
    if (hr) {
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x1232, hr, ((CGameWnd*)m_4)->m_4);
        m_c0->vtbl->Release(m_c0);
        m_c0 = 0;
        return 0;
    }

    m_9c = 1;
    return m_9c;
}

// -------------------------------------------------------------------------
// CGruntzMgr::PerFrameTick  (__thiscall; `ret`)
// The per-frame draw-clock tick. If the active state's Update() reports the
// "paused/hold" id (0x11) the tick is skipped; otherwise it refreshes the engine
// clock (CGameMgr::UnknownMethodInitializeTimeGlobal), optionally re-stamps the
// draw clock (g_6bf3c0 = timeGetTime(), g_6bf3bc = 0) when the draw gate m_30 is
// set, and finally mirrors the freshly-refreshed engine clock into the game-side
// pair (g_645580/g_645584).
RVA(0x0008f620, 0x51)
void CGruntzMgr::PerFrameTick() {
    if (m_2c && m_2c->Update() == 0x11) {
        return;
    }

    UnknownMethodInitializeTimeGlobal();

    if (m_30) {
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
void CGruntzMgr::AdvanceFrame(int doDraw, int /*unused*/) {
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
        if (CheckPlayState() == 0 && (m_2c == 0 || m_2c->Update() != 8)) {
            return;
        }
        m_48->StopBank(1);
        return;
    }

    if (m_14 == 0) {
        return;
    }
    if ((m_48->m_1c ? m_48->m_1c->IsBusy() : 0) == 0) {
        return;
    }
    m_48->StopAll();
}

// -------------------------------------------------------------------------
// CGruntzMgr::BuildMoviePath  (__thiscall; returns CString by value; /GX EH)
// Resolves the on-disk path of a numbered intro/cutscene .vob: maps the movie
// id to its file name, then probes first the working directory ("<cwd>\<name>")
// and, failing that, the Movies\ folder on the Gruntz CD ("<letter>:\Movies\
// <name>"), returning the first that exists (empty CString if neither, or for an
// unknown id). The two CString temps + the szPath buffer give the /GX frame.
RVA(0x0008ff30, 0x1ca)
CString CGruntzMgr::BuildMoviePath(int movie) {
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
int CGruntzMgr::NotifyState0b(int a, int b) {
    if (m_2c) {
        return m_2c->Vslot0b(a, b);
    }
    return 0;
}
RVA(0x0008da00, 0x1e)
int CGruntzMgr::NotifyState0c(int a, int b) {
    if (m_2c) {
        return m_2c->Vslot0c(a, b);
    }
    return 0;
}
RVA(0x0008da30, 0x1e)
int CGruntzMgr::NotifyState0d(int a, int b) {
    if (m_2c) {
        return m_2c->Vslot0d(a, b);
    }
    return 0;
}
RVA(0x0008da60, 0x23)
int CGruntzMgr::NotifyState0e(int a, int b, int c) {
    if (m_2c) {
        return m_2c->Vslot0e(a, b, c);
    }
    return 0;
}
RVA(0x0008daa0, 0x23)
int CGruntzMgr::NotifyState0f(int a, int b, int c) {
    if (m_2c) {
        return m_2c->Vslot0f(a, b, c);
    }
    return 0;
}
RVA(0x0008dae0, 0x23)
int CGruntzMgr::NotifyState10(int a, int b, int c) {
    if (m_2c) {
        return m_2c->Vslot10(a, b, c);
    }
    return 0;
}
RVA(0x0008db20, 0x23)
int CGruntzMgr::NotifyState11(int a, int b, int c) {
    if (m_2c) {
        return m_2c->Vslot11(a, b, c);
    }
    return 0;
}
RVA(0x0008db60, 0x23)
int CGruntzMgr::NotifyState12(int a, int b, int c) {
    if (m_2c) {
        return m_2c->Vslot12(a, b, c);
    }
    return 0;
}
RVA(0x0008dba0, 0x23)
int CGruntzMgr::NotifyState13(int a, int b, int c) {
    if (m_2c) {
        return m_2c->Vslot13(a, b, c);
    }
    return 0;
}
RVA(0x0008dbe0, 0x23)
int CGruntzMgr::NotifyState14(int a, int b, int c) {
    if (m_2c) {
        return m_2c->Vslot14(a, b, c);
    }
    return 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::CheckSavedMode (0x08de70; ret). If the live mode (m_8c x m_90)
// already equals the saved/last-good mode (m_94 x m_98) it succeeds; otherwise
// it drives SetVideoMode(saved-w, saved-h, save=1), falling back to
// RestoreVideoMode(1), and on total failure surfaces a (0x8008, 0x45e) error.
RVA(0x0008de70, 0x61)
int CGruntzMgr::CheckSavedMode() {
    // All success paths short-circuit to one trailing `return 1` (retail tail-
    // merges the mov eax,1; pop; ret epilogue).
    if ((m_8c == m_94 && m_90 == m_98) || SetVideoMode(m_94, m_98, 1) || RestoreVideoMode(1)) {
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
void CGruntzMgr::SetGameClock(int now, int delta, int abs) {
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
int CGruntzMgr::RunFromState() {
    return ChangeState_8fab0(1);
}

// -------------------------------------------------------------------------
// CGruntzMgr::TopState (0x090980). Returns the last pushed state (or 0 when the
// stack is empty).
RVA(0x00090980, 0x18)
CState* CGruntzMgr::TopState() {
    CStateStackZ* st = (CStateStackZ*)&m_arrD8;
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
    CStateStackZ* st = (CStateStackZ*)&m_arrD8;
    st->SetAtGrow(st->m_nSize, s);
}

// -------------------------------------------------------------------------
// CGruntzMgr::PopTopIfMatches (0x0909e0; ret 4). Saves the last stack slot,
// RemoveAt()s it, and reports whether the removed top was s. The count/data are
// read fresh off `this` (+0xe0/+0xdc); only the helper call forms &m_arrD8.
RVA(0x000909e0, 0x46)
int CGruntzMgr::PopTopIfMatches(CState* s) {
    if (!s) {
        return 0;
    }
    int n = *(int*)((char*)this + 0xe0);
    if (n <= 0) {
        return 0;
    }
    CState* top = (*(CState***)((char*)this + 0xdc))[n - 1];
    ((CStateStackZ*)&m_arrD8)->RemoveAt(n - 1, 1);
    return top == s;
}

// -------------------------------------------------------------------------
// CGruntzMgr::ClearStateStack (0x090a50; ret). Deletes every pushed state then
// clears the array via SetSize(0, -1). The loop reads count/data off `this`
// (+0xe0/+0xdc) so the array base is not hoisted into a register.
RVA(0x00090a50, 0x40)
void CGruntzMgr::ClearStateStack() {
    for (int i = 0; i < *(int*)((char*)this + 0xe0); i++) {
        CState* s = (*(CState***)((char*)this + 0xdc))[i];
        if (s) {
            delete s;
        }
    }
    ((CStateStackZ*)&m_arrD8)->SetSize(0, -1);
}

// -------------------------------------------------------------------------
// CGruntzMgr::CheckMovieFileExists (0x090aa0; cdecl ret). Probes whether the
// resolved movie path (m_strF0) exists on disk.
RVA(0x00090aa0, 0x10)
int CGruntzMgr::CheckMovieFileExists() {
    return Utils::WinAPI::FileExists((char*)(const char*)m_strF0);
}

// -------------------------------------------------------------------------
// CGruntzMgr::IsLobbyHostReady (0x091500). Null-chain predicate: returns
// m_2c->ReleaseResources-slot result (slot 7, +0x1c) only when m_2c, the
// CGameApp (m_8), its +0x240 sub-object and !m_ac all hold; else 0.
RVA(0x00091500, 0x42)
int CGruntzMgr::IsLobbyHostReady() {
    if (m_2c == 0) {
        return 0;
    }
    int* app = (int*)m_8;
    if (app == 0) {
        return 0;
    }
    if (*(int*)((char*)app + 0x240) == 0) {
        return 0;
    }
    if (m_ac != 0) {
        return 0;
    }
    return m_2c->Vslot07() != 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::StoreInputState (0x091a10; ret 4). Stores v at +0x120, and when the
// +0x60 sub-object is present mirrors it into that object's +0x2c.
RVA(0x00091a10, 0x17)
int CGruntzMgr::StoreInputState(int v) {
    m_120 = v;
    int* p = (int*)m_60;
    if (p) {
        p[0x2c / 4] = v;
    }
    return v;
}

// -------------------------------------------------------------------------
// CGruntzMgr::TickStateMgrs (0x0920b0). Drives the two engine state singletons
// (g_645570/g_645578) once and reports success.
RVA(0x000920b0, 0x1c)
int CGruntzMgr::TickStateMgrs() {
    g_645570->PollAll();
    g_645578->Flush();
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::FindStateById (0x092900; ret 4). Returns the live state when its
// Update() id matches `id`; otherwise the first stack entry whose Update() id
// matches (or 0).
RVA(0x00092900, 0x6e)
CState* CGruntzMgr::FindStateById(int id) {
    if (m_2c && m_2c->Update() == id) {
        return m_2c;
    }
    CStateStackZ* st = (CStateStackZ*)&m_arrD8;
    for (int i = 0; i < st->m_nSize; i++) {
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
// CGruntzMgr::ChangeState_8fab0 (0x08fab0) - deferred big method; the thin
// RunFromState wrapper calls it (reloc-masked). Migrated from Discovered.cpp.
// @confidence: high
// @source: call-xref
// @stub
RVA(0x0008fab0, 0x318)
int CGruntzMgr::ChangeState_8fab0(int /*arg*/) {
    return 0;
}

// CGruntzMgr::UnknownClose (@0x0855e0) is the large member-teardown method the
// dtor calls; its body is still the stub (src/Stub/CGruntzMgr.cpp). It is only
// DECLARED here (in the header) - the dtor's call to it is an external ref whose
// reloc objdiff masks, so no definition is needed in this TU.

// -------------------------------------------------------------------------
// CGruntzMgr::~CGruntzMgr  (virtual; vtable slot 0; own vftable @0x5e9b64)
// The own body just runs UnknownClose(); the compiler then destructs the five
// destructible members (m_options150, m_strF0, m_strEC, m_arrD8, m_strC8, in
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
// the view's edge origins (m_5c +0x40..+0x4c, biased by 0x60) into m_13c..m_148.
// No-op when no world is loaded. The int extents are converted to float once and
// reused across the three scale passes (fild/fst then fld).
// @early-stop
// ~83% x87-scheduling wall: logic is exact (same 1.4/5.3/1.12 scales, same view
// writes + per-pass Notify + the m_13c..m_148 edge snapshot). Retail keeps the
// freshly-converted extent in st0 with `fst` (store-and-keep) and multiplies it
// in place; MSVC here emits `fstp` (store-and-pop) + a reload `fld`, a 1-2 instr
// FPU-stack scheduling drift per pass. No source spelling flips fst<->fstp;
// interleaving the conversions makes it worse (78%).
RVA(0x0008f7f0, 0x131)
void CGruntzMgr::RecomputeViewScale() {
    if (m_30 == 0) {
        return;
    }
    CScrollView* view = (CScrollView*)m_30->m_24;
    float fw = (float)(view->m_18 - view->m_10 + 1);
    float fh = (float)(view->m_1c - view->m_14 + 1);

    view->m_c8 = (int)(fw * 1.4f);
    view->m_cc = (int)(fh * 1.4f);
    view->Notify();

    view = (CScrollView*)m_30->m_24;
    view->m_d0 = (int)(fw * 5.3f);
    view->m_d4 = (int)(fh * 5.3f);
    view->Notify();

    view = (CScrollView*)m_30->m_24;
    view->m_d8 = (int)(fw * 1.12f);
    view->m_dc = (int)(fh * 1.12f);
    view->Notify();

    CScrollView* v = (CScrollView*)m_30->m_24;
    if (v->m_5c == 0) {
        return;
    }
    m_13c = v->m_5c->m_40 - 0x60;
    m_140 = ((CScrollView*)m_30->m_24)->m_5c->m_44 - 0x60;
    m_144 = ((CScrollView*)m_30->m_24)->m_5c->m_48 + 0x60;
    m_148 = ((CScrollView*)m_30->m_24)->m_5c->m_4c + 0x60;
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
// (options loop, m_68/source/m_6c/m_70/hook/m_7c) match byte for byte. Retail
// emits the cmd==4 arm OUT-OF-LINE at the function tail (`cmp 4; je <end>`; the
// 4-block jmps back into the shared m_60->Tick), but MSVC here keeps it inline
// between the gate and the cmd==7 test, shifting that one block. Pure basic-block
// placement; case reorder doesn't move it (see docs/patterns/switch-cases-source-order.md).
// NOTE: the trace size was 0x124 but the real function runs to 0x15c (the cmd==4
// tail + the bool-normalizing HUD epilogue past the under-counted Ghidra bound).
RVA(0x00093460, 0x15c)
int CGruntzMgr::BroadcastCmd(int a0, int cmd, int a2, int a3) {
    if (a0 == 0) {
        return 0;
    }
    switch (cmd) {
        case 4:
            if (PrepCmd4(a0) == 0) {
                return 0;
            }
            ((CmdTimer60*)m_60)->Tick();
            break;
        case 7:
            if (PrepCmd7(a0) == 0) {
                return 0;
            }
            ((CmdTimer60*)m_60)->Tick();
            break;
    }

    OptionsSlot* slot = (OptionsSlot*)((char*)this + 0x150);
    for (int i = 0; i < 4; i++) {
        if (slot == 0 || slot->Command(a0, cmd, a2, a3) == 0) {
            return 0;
        }
        slot = (OptionsSlot*)((char*)slot + 0x238);
    }

    if (((CmdSink*)m_68)->Command(a0, cmd, a2, a3) == 0) {
        return 0;
    }
    if (((CmdSink*)GetSaveSource())->Command(a0, cmd, a2, a3) == 0) {
        return 0;
    }
    if (((CmdSink*)m_6c)->Command(a0, cmd, a2, a3) == 0) {
        return 0;
    }
    if (((CmdSinkV*)m_70)->Command(a0, cmd, a2, a3) == 0) {
        return 0;
    }
    if (CmdHook(a0, cmd, a2, a3) == 0) {
        return 0;
    }
    return ((CmdSink*)m_7c)->Command(a0, cmd, a2, a3) != 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::UpdateScoreHud (0x0860b0; ret). Per-frame score/HUD accumulator,
// active only while the registry's gate (g_gameReg->m_134) is 1. Folds this
// world's score/time deltas (m_68's +0x20c/+0x21c tables, indexed by the active
// world g_644c54) into the +0x7c HUD accumulators. If a level name is loaded
// (m_strC8 non-empty) it just refreshes the HUD with 1 and marks it dirty;
// otherwise, on the first frame (m_44->m_124 == 0) it seeds the HUD from the
// registry's cumulative score and fires the score-bump / tick / notify chain,
// then refreshes the HUD with the live score and clears the dirty flag.
RVA(0x000860b0, 0xe8)
void CGruntzMgr::UpdateScoreHud() {
    if (g_gameReg->m_134 != 1) {
        return;
    }
    ScoreSub2c* sub = g_gameReg->m_2c;

    ((ScoreHud*)m_7c)->m_1c += ((WorldDeltaTables*)m_68)->m_arr20c[g_644c54];
    ((ScoreHud*)m_7c)->m_20 += ((WorldDeltaTables*)m_68)->m_arr21c[g_644c54];

    if (m_strC8.GetLength() != 0) {
        ((ScoreHud*)m_7c)->Refresh(1);
        ((ScoreHud*)m_7c)->m_8 = 1;
        return;
    }

    if (((HudGuard44*)m_44)->m_124 == 0) {
        ((ScoreHud*)m_7c)->Seed(sub->m_1c, 0);
        g_gameReg->m_58->Bump(sub->m_1c);
        g_gameReg->m_58->Tick((sub->m_1c % 0x28) + 1);
        g_gameReg->m_58->Notify(0, 0x81a6);
    }
    ((ScoreHud*)m_7c)->Refresh(sub->m_1c);
    ((ScoreHud*)m_7c)->m_8 = 0;
}

// -------------------------------------------------------------------------
// CGruntzMgr::SaveState (0x093620; ret 4; returns 1/0). Streams the whole
// game-clock / scroll / warp snapshot through the supplied archive. Bails (0)
// when no archive or no loaded world. Bumps the global save serial, writes the
// world file name (m_strC8, copied into a fresh 0x80 buffer), then the per-frame
// state block (m_114..m_13c) and the clock/scroll/warp globals - each via the
// archive's Serialize(&field, size) slot. Returns 1.
RVA(0x00093620, 0x254)
int CGruntzMgr::SaveState(CSerializerZ* ar) {
    if (ar == 0) {
        return 0;
    }
    if (m_30 == 0) {
        return 0;
    }
    g_629ad0++;

    char buf[0x80];
    memset(buf, 0, 0x80);
    strcpy(buf, (const char*)m_strC8);
    ar->Serialize(buf, 0x80);

    ar->Serialize(&m_114, 4);
    ar->Serialize(&m_11c, 4);
    ar->Serialize(&m_128, 4);
    ar->Serialize(&m_12c, 4);
    ar->Serialize(&m_130, 4);
    ar->Serialize(&m_134, 4);
    ar->Serialize(&m_138, 4);
    ar->Serialize((char*)this + 0x13c, 0x10);
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
    ar->Serialize(&m_118, 4);
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
// CGruntzMgr::FillSaveInfo (0x0927b0; ret 8). Populates a save-slot record from
// the live game: bails if there is no record or no live source state, copies the
// level name into the record (inline strcpy), stamps the win/level state flags
// (m_134==3 -> m_fc, m_130 -> m_f8), hands the record + the source state's data
// block (+0x1d0) to the +0x58 sink, remembers the record at m_bc, and - when a
// snapshot block is supplied - copies its 0x20 bytes into the record (+0x14).
// @early-stop
// ~99.2% regalloc tail tiebreak: logic + the EH-frame elision (the name temp is
// scoped so no throwing call is live during it) + the out-of-line EngineCopy all
// match; the residual is the m_134 compare landing in esi here vs retail's edi
// (freed by the inline-strcpy rep-movs) - a pure esi<->edi naming swap.
RVA(0x000927b0, 0xc4)
int CGruntzMgr::FillSaveInfo(SaveInfo* dst, void* snapshot) {
    if (dst == 0) {
        return 0;
    }
    char* src = (char*)GetSaveSource();
    if (src == 0) {
        return 0;
    }
    {
        // Scope the name temp so it is destructed before the (potentially
        // throwing) sink/memcpy calls - keeping its live range free of any such
        // call lets MSVC /GX elide the EH frame, matching retail's frameless body.
        CString name = GetLevelName();
        strcpy(dst->m_75, (const char*)name);
    }
    dst->m_fc = (m_134 == 3);
    dst->m_f8 = m_130;
    ((SaveSink58*)m_58)->Store(dst, src + 0x1d0);
    m_bc = (int)dst;
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
int CGruntzMgr::FinishLevel(int full, int stopBank) {
    if (m_2c && m_2c->Update() == 0x11) {
        PlayStatusSlot* base = ((CPlayStateView*)m_2c)->m_520;
        char* p = (char*)base + 0x20;
        int done = 0;
        for (int d = 4; d != 0; d--) {
            if (p && *(int*)p == 3) {
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
        if (m_54) {
            ((InputState54*)m_54)->Method0();
        }
        if (m_30) {
            CWorldSub28* sub = m_30->m_28;
            if (sub && sub->m_2c) {
                sub->m_2c->Teardown();
            }
        }
        if ((m_48->m_1c ? m_48->m_1c->IsBusy() : 0) && stopBank) {
            m_48->StopAll();
        }
        m_2c->Vslot18();
        if (full) {
            return 1;
        }
    }

    if (m_14) {
        if (CheckLevelActive()) {
            m_48->StopBank(1);
        }
    }
    if (m_10) {
        ((InputState54*)m_54)->Method1();
        if (m_68 && m_10) {
            ((Ctrl68*)m_68)->Reset();
        }
    }
    m_2c->Vslot19();
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
void CGruntzMgr::EnterModalUI(int arg) {
    CGameApp* app = (CGameApp*)m_8;
    if (app == 0) {
        return;
    }
    if (m_60) {
        ((TimerObj*)m_60)->Stop();
    }
    if (m_30) {
        RedrawMapIndex(m_30->m_4->m_14);
        CWorldDispatch* d = *m_30->m_1c;
        d->vtbl->Slot0a(d);
    }

    int(__stdcall * show)(int) = g_pShowCursor;
    int shown = show(1);
    while (show(1) < 0) {
    }

    m_ac = 1;
    app->RunModal(arg, ((CGameWnd*)m_4)->m_4);
    g_64557c = 0;
    m_ac = 0;
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
int CGruntzMgr::ExitModalUI(CModalDialog* dlg, int notify) {
    if (m_60) {
        ((TimerObj*)m_60)->Stop();
    }
    if (m_68 && m_10) {
        ((Ctrl68*)m_68)->Flush();
    }
    if (m_30) {
        if (notify && m_2c && m_2c->Update() != 5) {
            m_2c->NotifyExit(0x32);
        } else {
            notify = 0;
        }
        CWorldDispatch* d = *m_30->m_1c;
        d->vtbl->Slot0a(d);
    }

    int(__stdcall * show)(int) = g_pShowCursor;
    int shown = show(1);
    while (show(1) < 0) {
    }

    m_ac = 1;
    int result = dlg->Run();
    g_64557c = 0;
    m_ac = 0;
    if (m_2c && notify) {
        m_2c->Vslot06();
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
// state's id, then delete it), installs the new state (m_2c = next; Activate),
// then notifies slot 9 (Vslot09) with the old state's id and polls slot 6
// (Vslot06); on either success it arms the app's resume flag (m_8->+0x244) and
// runs the post-switch hook, returning 1. Otherwise returns 0.
RVA(0x0008d6a0, 0xaf)
int CGruntzMgr::SwitchToNextState() {
    if (Wap32GameMgrVfunc3() == 0) {
        return 0;
    }
    CState* next = MakeNextState();
    if (next == 0) {
        return 0;
    }
    if (m_2c == next) {
        return 0;
    }
    int oldId = 0;
    if (m_2c) {
        oldId = m_2c->Update();
        m_2c->FrameSlot28(next->Update());
        if (m_2c) {
            delete m_2c;
        }
        m_2c = 0;
    }
    m_2c = next;
    ActivateState(next);
    if (m_2c->Vslot09(oldId) == 0 && m_2c->Vslot06() == 0) {
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
// regalloc wall: in the hit block retail caches m_2c's vtbl in callee-saved ebx
// across the two virtual calls (push ebx at entry, stack args shift +4); MSVC
// here keeps the vtbl in edi and re-reads it, so no ebx push. Logic is exact;
// the residual is the vtbl-CSE register choice (see docs/patterns/
// pin-local-for-callee-saved-reg.md - no clean source spelling for vtbl pinning).
int CGruntzMgr::PassClickToPlayState(int a0, int a1, int a2) {
    int inPlay = 0;
    if (m_2c->Update() == 3) {
        inPlay = 1;
    }
    if (m_2c->Update() == 0x11) {
        inPlay = 1;
    }
    if (inPlay && a1 == 0) {
        m_2c->FrameSlot28(m_2c->Update());
        if (m_2c->Vslot1e(a0, a2) == 0) {
            return 0;
        }
        m_2c->Vslot09(m_2c->Update());
        return 1;
    }
    return ChangeToPlayState(3, a0, 0, 0);
}

// -------------------------------------------------------------------------
// CGruntzMgr::UnloadSoundChain (0x08f740; ret). Tears down the loaded world's
// inner controller object (m_30->m_28->m_2c->Teardown(), each guarded) then, if
// the sound object's inner busy-poll reports busy, stops the bank via StopBank2.
RVA(0x0008f740, 0x46)
void CGruntzMgr::UnloadSoundChain() {
    if (m_30) {
        CWorldSub28* sub = m_30->m_28;
        if (sub) {
            CWorldSub2c* obj = sub->m_2c;
            if (obj) {
                obj->Teardown();
            }
        }
    }
    CGruntzSoundZ* snd = m_48;
    if (snd && (snd->m_1c ? snd->m_1c->IsBusy() : 0)) {
        m_48->StopBank2();
    }
}

// -------------------------------------------------------------------------
// CGruntzMgr::StoreInputFlag (0x0919d0; ret 4). Records the input flag at +0x11c;
// when the loaded world's +0x28 sub-object is present it also mirrors the flag
// into the g_61ab24 global; finally forwards the flag to the +0x54 input object.
RVA(0x000919d0, 0x30)
void CGruntzMgr::StoreInputFlag(int v) {
    m_11c = v;
    if (m_30 && m_30->m_28) {
        g_61ab24 = v;
    }
    InputStateObj* in = (InputStateObj*)m_54;
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
    char* p = (char*)this + 0x174; // &m_options150[0].m_24
    for (int i = 4; i != 0; i--) {
        char* elem = p - 0x24;
        if (elem) {
            *(int*)(p - 4) = 0; // elem.m_20
            *(int*)p = 0;       // elem.m_24
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
    return m_strC8;
}

// -------------------------------------------------------------------------
// CGruntzMgr::AdvanceOptionsCycle (0x0933e0; ret). Bumps the global round-robin
// cursor g_6455fc (mod 4); for the options slot whose index matches the cursor,
// if it is armed (m_164 == 0) and loaded (m_170 != 0) it ticks the slot's +0x38
// sub-object. The loop runs i in [0, m_138]; both the count and the cursor are
// re-read each iteration (the cursor after the reloc-masked tick).
RVA(0x000933e0, 0x5e)
int CGruntzMgr::AdvanceOptionsCycle() {
    int cursor = (g_6455fc + 1) & 3;
    g_6455fc = cursor;
    for (int i = 0; i < m_138 + 1; i++) {
        OptionsSlot* slot = (OptionsSlot*)((char*)this + 0x150 + i * 0x238);
        if (cursor == i && slot->m_14 == 0 && slot->m_20 != 0) {
            slot->m_38.Tick();
            cursor = g_6455fc;
        }
    }
    return 1;
}

// -------------------------------------------------------------------------
// CGruntzMgr::SetCellHeight (0x111ec0; ret 0xc). Writes value into the loaded
// world's height grid: idx = view->m_24[col] + row; view->m_20[idx] = value.
// Then forwards (row, col, value) to the +0x70 notify object (reloc-masked).
RVA(0x00111ec0, 0x37)
void CGruntzMgr::SetCellHeight(int row, int col, int value) {
    CHeightGrid* grid = ((CWorldViewZ*)m_30->m_24)->m_5c;
    int idx = grid->m_24[col] + row;
    grid->m_20[idx] = value;
    ((CNotify70*)m_70)->Set(row, col, value);
}

// size 0xa30 recovered from operator-new sites (gruntz.analysis.news)
SIZE(CGruntzMgr, 0xa30);
