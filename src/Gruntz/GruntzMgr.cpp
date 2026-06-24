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

// size 0xa30 recovered from operator-new sites (gruntz.analysis.news)
SIZE(CGruntzMgr, 0xa30);
