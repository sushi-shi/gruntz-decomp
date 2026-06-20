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
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

namespace Utils {
namespace WinAPI {
char GetGruntzDriveLetter();
int  FileExists(char *szPath);
}
}

// External DirectPlay surface used by InitializeLobbyConnectionSettings. The
// DPLAYX export DirectPlayLobbyCreate (ordinal 4) and CNetMgr::ReportError are
// reached as `call rel32`/[IAT] - the displacement reloc-masks, so only the
// arg shapes are load-bearing. `m_c4` (the connection-settings buffer) is freed
// through a small NAFXCW-style operator-delete helper (out-of-line, reloc-
// masked); modelled as a plain free function.
extern "C" __declspec(dllimport) long __stdcall DirectPlayLobbyCreate(
    void *lpguidDSP, IDirectPlayLobbyZ **lplpDPL, void *pUnk,
    void *lpData, unsigned long dwDataSize);

class CNetMgr {
public:
    static void ReportError(char *file, int line, long hr, void *hWnd);
};

void FreeConnectionSettings(void *p);   // FUN_005b9b82 (operator delete wrapper)

void *operator new(unsigned int);

// KERNEL32 surface (BuildMoviePath probes the working directory). Minimal
// __declspec(dllimport) __stdcall decl -> the FF15 [IAT] indirect call form.
typedef unsigned long DWORD;
extern "C" __declspec(dllimport) DWORD __stdcall GetCurrentDirectoryA(
    DWORD nBufferLength, char *lpBuffer);

// The engine's __cdecl CString-formatting helper (sprintf-style into a CString
// destination; reloc-masked - only the call shape is load-bearing).
extern "C" void Format(CString *dst, const char *fmt, ...);

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
RVA(0x08dc60, 0x19)
void CGruntzMgr::ReportError(WPARAM wParam, LPARAM lParam)
{
    CGameApp *pApp = (CGameApp *)m_8;
    if (pApp)
        pApp->ReportError(wParam, lParam);
}

// -------------------------------------------------------------------------
// CGruntzMgr::GetGruntzDriveLetter  (__thiscall)
// Returns the CD-ROM drive letter holding the Gruntz disc, memoised in the
// pair (m_d0 = letter, m_d4 = probed-flag): once probed, return the cached
// letter; otherwise call Utils::WinAPI::GetGruntzDriveLetter(), cache + set the
// flag. (The result is discarded by the engine on the first/uncached path - the
// store IS the return, the cached path returns the byte.)
RVA(0x08fa70, 0x2c)
char CGruntzMgr::GetGruntzDriveLetter()
{
    if (m_d4)
        return m_d0;
    m_d0 = Utils::WinAPI::GetGruntzDriveLetter();
    m_d4 = 1;
    return m_d0;
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
RVA(0x08eca0, 0x164)
int CGruntzMgr::InitializeLobbyConnectionSettings()
{
    if (m_a0)
        return m_9c;

    m_a0 = 1;
    m_9c = 0;

    if (m_c0) {
        m_c0->vtbl->Release(m_c0);
        m_c0 = 0;
    }

    long hr = DirectPlayLobbyCreate(0, &m_c0, 0, 0, 0);
    if (hr) {
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x120d, hr,
                             ((CGameWnd *)m_4)->m_4);
        return 0;
    }
    if (!m_c0)
        return 0;

    if (m_c4) {
        FreeConnectionSettings(m_c4);
        m_c4 = 0;
    }

    unsigned long dwSize = 0;
    hr = m_c0->vtbl->GetConnectionSettings(m_c0, 0, 0, &dwSize);
    if (hr != 0 && hr != (long)0x8877001e) {   // !DPERR_BUFFERTOOSMALL
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x1221, hr,
                             ((CGameWnd *)m_4)->m_4);
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
        CNetMgr::ReportError("C:\\Proj\\Gruntz\\GruntzMgr.cpp", 0x1232, hr,
                             ((CGameWnd *)m_4)->m_4);
        m_c0->vtbl->Release(m_c0);
        m_c0 = 0;
        return 0;
    }

    m_9c = 1;
    return m_9c;
}

// -------------------------------------------------------------------------
// CGruntzMgr::BuildMoviePath  (__thiscall; returns CString by value; /GX EH)
// Resolves the on-disk path of a numbered intro/cutscene .vob: maps the movie
// id to its file name, then probes first the working directory ("<cwd>\<name>")
// and, failing that, the Movies\ folder on the Gruntz CD ("<letter>:\Movies\
// <name>"), returning the first that exists (empty CString if neither, or for an
// unknown id). The two CString temps + the szPath buffer give the /GX frame.
RVA(0x08ff30, 0x1ca)
CString CGruntzMgr::BuildMoviePath(int movie)
{
    CString name;

    switch (movie) {
    case -1: name = "Logo.vob";    break;
    case 0:  name = "Gruntz0.vob"; break;
    case 1:  name = "Gruntz1.vob"; break;
    case 2:  name = "Gruntz2.vob"; break;
    case 3:  name = "Gruntz3.vob"; break;
    case 4:  name = "Gruntz4.vob"; break;
    case 5:  name = "Gruntz5.vob"; break;
    case 6:  name = "Gruntz6.vob"; break;
    case 7:  name = "Gruntz7.vob"; break;
    case 8:  name = "Gruntz8.vob"; break;
    }

    if (name.GetLength() == 0)
        return name;                         // unknown id

    CString path;
    char szDir[256];

    // First try the working directory ("<cwd>\<name>").
    if (GetCurrentDirectoryA(0xff, szDir)) {
        Format(&path, "%s\\%s", szDir, (const char *)name);
        if (!Utils::WinAPI::FileExists((char *)(const char *)path))
            path.Empty();
    }

    // Fall back to the Movies\ folder on the Gruntz CD.
    if (path.GetLength() == 0) {
        Format(&path, "%c:\\Movies\\%s", GetGruntzDriveLetter(), (const char *)name);
        if (path.GetLength() == 0)
            return path;
    }

    if (!Utils::WinAPI::FileExists((char *)(const char *)path))
        path.Empty();

    return path;
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
RVA(0x083360, 0xb2)
CGruntzMgr::~CGruntzMgr()
{
    UnknownClose();
}
