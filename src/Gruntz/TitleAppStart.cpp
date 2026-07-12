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
#include <Mfc.h>          // afx-first (unified CObject; superset of Win32.h) - g_ShowCursor fn-ptr
#include <Gruntz/State.h> // the CState base this title state derives (RunTitleSeq @0xfa350)

extern int(WINAPI* g_ShowCursor)(int); // ?g_ShowCursor@@3P6GHH@ZA (RVA 0x2c44c4)
// The title-sequence's const-char* arg source at RVA 0x24e25c (the CString/asset-root
// whose data ptr RunTitleSeq consumes). reloc-fidelity BLOCKED: 0x24e25c is a
// name-CONFLATION - netmgrmisc (F2-forbidden) binds it ?g_netE25c and splashstate
// binds ?g_assetRoot at the VA-typo 0x64e25c; the per-rva keep-last dedup drops any
// name we add here in favour of the higher-sorting ?g_netE25c, so this stays UNBOUND
// until the owning (forbidden) units unify on one name at 0x24e25c.
extern void* g_64e25c; // 0x24e25c (conflated; see note)

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
    int(WINAPI * sc)(int) = g_ShowCursor;
    while (sc(0) >= 0) {
    }
    RunTitleSeq((const char*)g_64e25c, 1, 1, 1, 0); // CState::RunTitleSeq @0xfa350
    m_1b8 = 0xea60;
    return 1;
}

SIZE_UNKNOWN(CTitleApp);
