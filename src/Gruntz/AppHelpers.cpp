// AppHelpers.cpp - three small app/UI helpers (anonymous owners).
//
//  0xf9880  - hide the cursor (spin ShowCursor until the count goes negative),
//             kick off the title sequence, arm a 60s (0xea60) timer, return 1.
//  0xb4cb0  - forward a 4-arg message to a sub-handler; on success, when arg1==8,
//             stamp three fields of the +0x10 child from the global settings.
//  0xbe030  - enable/disable a pair of dialog controls (0x4cc/0x4cd) from a flag.
#include <rva.h>
#include <Mfc.h> // afx-first (TU pulls MFC via unified CObject; superset of Win32.h)

#include <Gruntz/GameRegistry.h> // canonical *0x64556c game-manager singleton
#include <Gruntz/LightFxMgr.h>   // CLightFxMgr (m_logicPump @+0x78; m_tables[])

extern int(WINAPI* g_ShowCursor)(int); // ?g_ShowCursor@@3P6GHH@ZA (RVA 0x2c44c4)
// The title-sequence's const-char* arg source at RVA 0x24e25c (the CString/asset-root
// whose data ptr RunTitleSeq consumes). reloc-fidelity BLOCKED: 0x24e25c is a
// name-CONFLATION - netmgrmisc (F2-forbidden) binds it ?g_netE25c and splashstate
// binds ?g_assetRoot at the VA-typo 0x64e25c; the per-rva keep-last dedup drops any
// name we add here in favour of the higher-sorting ?g_netE25c, so this stays UNBOUND
// until the owning (forbidden) units unify on one name at 0x24e25c.
extern void* g_64e25c; // 0x24e25c (conflated; see note)

// @identity-TODO (RECOVERED, model deferred): OnStart calls RunTitleSeq on `this`, and
// retail_rva 0xfa350 = ?RunTitleSeq@CAttract@@QAEHPBDHHHH@Z [attract] - so this is a
// CAttract method (or a CAttract-embedding host). Tying it needs Attract.h (CAttract :
// CState, heavy) + confirming +0x1b8 (a timer here vs m_host in Attract.h - a layout
// conflict to resolve first); left reloc-masked until that dedicated pass.
struct CTitleApp {
    char pad[0x1b8];
    int m_1b8;                                            // +0x1b8 timer
    int RunTitleSeq(void* a, int b, int c, int d, int e); // 0xfa350 (= CAttract::RunTitleSeq)
    int OnStart(int unused);                              // 0xf9880
};

// 0xf9880
RVA(0x000f9880, 0x43)
int CTitleApp::OnStart(int) {
    int(WINAPI * sc)(int) = g_ShowCursor;
    while (sc(0) >= 0) {
    }
    RunTitleSeq(g_64e25c, 1, 1, 1, 0);
    m_1b8 = 0xea60;
    return 1;
}

// ---------------------------------------------------------------------------
// The game-manager singleton is the canonical CGameRegistry (*0x64556c); this
// routine reaches its +0x78 light-FX pump (CLightFxMgr) shade-table array
// (m_tables[5], the +0x28 slot).
extern "C" CGameRegistry* g_gameReg; // 0x64556c

struct CSub10 {
    char pad[0x4c];
    void* m_4c; // +0x4c
    int m_50;   // +0x50
    char pad2[4];
    int m_58; // +0x58
};
// @identity-TODO (RECOVERED, model deferred): Handle calls Method_b4d30 on `this`, and
// retail_rva 0xb4d30 = ?Serialize@CUFO@@QAEHPAXHHH@Z [ufo] - so this is a CUFO method
// (Handle IS CUFO::Handle). Tying needs Ufo.h (CUFO : CPathHazard) + confirming the
// +0x10 (CSub10*) member is CUFO's; left reloc-masked until that dedicated pass.
struct CHandlerB4 {
    char pad[0x10];
    CSub10* m_10;                         // +0x10
    int Method_b4d30(int, int, int, int); // 0xb4d30 (= CUFO::Serialize)
    int Handle(int, int, int, int);       // 0xb4cb0
};

// 0xb4cb0
RVA(0x000b4cb0, 0x56)
int CHandlerB4::Handle(int a0, int a1, int a2, int a3) {
    if (!Method_b4d30(a0, a1, a2, a3)) {
        return 0;
    }
    if (a1 == 8) {
        void* x = g_gameReg->m_logicPump->m_tables[5];
        CSub10* p = m_10;
        p->m_58 = 1;
        p->m_50 = 7;
        p->m_4c = x;
    }
    return 1;
}

// ---------------------------------------------------------------------------
struct DlgData {
    char pad[0x528];
    int m_528; // +0x528 enable flag
};

// 0xbe030
RVA(0x000be030, 0x49)
void Unmatched_be030(HWND hDlg, DlgData* p) {
    if (hDlg && p) {
        EnableWindow(GetDlgItem(hDlg, 0x4cc), p->m_528);
        EnableWindow(GetDlgItem(hDlg, 0x4cd), p->m_528);
    }
}

SIZE_UNKNOWN(CHandlerB4);
SIZE_UNKNOWN(CSub10);
SIZE_UNKNOWN(CTitleApp);
SIZE_UNKNOWN(DlgData);
