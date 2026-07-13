// TitleAppStart.cpp - OnStart (0xf9880, __thiscall ret 4): hide the cursor (spin
// ShowCursor until the count goes negative), kick off the title sequence, arm a 60s
// (0xea60) timer at +0x1b8, return 1. Split out of the AppHelpers.cpp holding TU.
//
// @identity-TODO (owner deferred): OnStart calls RunTitleSeq on `this`, and retail
// 0xfa350 = ?RunTitleSeq@CAttract@@QAEHPBDHHHH@Z [attract] - so `this` is a CAttract*.
// But (1) OnStart @0xf9880 sits far from the CAttract obj block (which starts at
// FadeInTitle @0xfa1f0, amid the splashstate/gamemode objs) and (2) it writes an int
// timer (0xea60) to +0x1b8, which Attract.h models as a CAttractVoice* sound/host
// sub-object - a real layout conflict. So the CAttract binding is NOT forced here: the
// CTitleApp placeholder host keeps the RVA (RunTitleSeq declared-only, reloc-masked to
// the real CAttract::RunTitleSeq at 0xfa350). Resolve the +0x1b8 conflict + the obj
// placement in a dedicated CAttract pass before binding.
#include <rva.h>
#include <Mfc.h>          // afx-first (unified CObject; superset of Win32.h) - ::ShowCursor fn-ptr
#include <Gruntz/State.h> // the CState base this title state derives (RunTitleSeq @0xfa350)

// The title-sequence's const-char* arg source @0x24e25c IS the global asset-root CString
// `g_assetRoot` (same datum SplashState/GruntzMgr bind); RunTitleSeq consumes its data
// ptr (CString::operator LPCTSTR -> the +0 m_pszData load). Single-sourced onto the
// canonical `g_assetRoot` name (was the fake `g_64e25c` void* that lost the per-rva
// keep-last dedup to netmgrmisc's ex-`g_netE25c` view - now dissolved -> UNBOUND).
DATA(0x0024e25c)
extern CString g_assetRoot; // 0x24e25c (the asset-root CString)

// CTitleApp is a CState leaf (OnStart calls RunTitleSeq @0xfa350 - a CState base method -
// on its own `this`; retail passes `this` as the CState). Its exact leaf identity is
// still @identity-TODO, but deriving CState is proven + sufficient to bind the base
// symbol. The +0x1b8 timer sits past the CState base (which ends at +0x1a8), exactly as
// CPreviewState's own m_1b8 does.
class CTitleApp : public CState {
public:
    int OnStart(int unused); // 0xf9880
    char m_pad1a8[0x1b8 - 0x1a8];
    int m_1b8; // +0x1b8 timer
};

RVA(0x000f9880, 0x43)
int CTitleApp::OnStart(int) {
int(WINAPI * sc)(BOOL) = ::ShowCursor;
    while (sc(0) >= 0) {
    }
    RunTitleSeq((const char*)g_assetRoot, 1, 1, 1, 0); // CState::RunTitleSeq @0xfa350
    m_1b8 = 0xea60;
    return 1;
}

SIZE_UNKNOWN(CTitleApp);
