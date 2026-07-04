// AppHelpers.cpp - three small app/UI helpers (anonymous owners).
//
//  0xf9880  - hide the cursor (spin ShowCursor until the count goes negative),
//             kick off the title sequence, arm a 60s (0xea60) timer, return 1.
//  0xb4cb0  - forward a 4-arg message to a sub-handler; on success, when arg1==8,
//             stamp three fields of the +0x10 child from the global settings.
//  0xbe030  - enable/disable a pair of dialog controls (0x4cc/0x4cd) from a flag.
#include <rva.h>
#include <Win32.h>

#include <Gruntz/GameRegistry.h> // canonical *0x64556c game-manager singleton

extern int(WINAPI* g_ShowCursor)(int); // ?g_ShowCursor@@3P6GHH@ZA (0x6c44c4)
extern void* g_64e25c;

struct CTitleApp {
    char pad[0x1b8];
    int m_1b8;                                            // +0x1b8 timer
    int RunTitleSeq(void* a, int b, int c, int d, int e); // 0xfa350
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
// The +0x78 reused sub-object slot's concrete view here (authentic downcast off
// the game-registry singleton's void* m_78).
struct MgrInner {
    char pad[0x28];
    void* m_28; // +0x28
};
// The game-manager singleton is the canonical CGameRegistry (*0x64556c); this
// routine reaches its +0x78 slot's m_28.
extern "C" CGameRegistry* g_mgrSettings; // 0x64556c

struct CSub10 {
    char pad[0x4c];
    void* m_4c; // +0x4c
    int m_50;   // +0x50
    char pad2[4];
    int m_58; // +0x58
};
struct CHandlerB4 {
    char pad[0x10];
    CSub10* m_10;                         // +0x10
    int Method_b4d30(int, int, int, int); // 0xb4d30
    int Handle(int, int, int, int);       // 0xb4cb0
};

// 0xb4cb0
RVA(0x000b4cb0, 0x56)
int CHandlerB4::Handle(int a0, int a1, int a2, int a3) {
    if (!Method_b4d30(a0, a1, a2, a3)) {
        return 0;
    }
    if (a1 == 8) {
        void* x = ((MgrInner*)g_mgrSettings->m_78)->m_28;
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
SIZE_UNKNOWN(MgrInner);
