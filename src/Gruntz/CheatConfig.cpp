// CheatConfig.cpp - cheat config loaders + booty state + Monolith dance.
// Built /O2 /MT /GX for CString dtors on EH unwind.
#include "CheatConfig.h"

extern int g_bootyCheatEnabled;   // @data: 0x262af10

// ============================================================================
// LoadBootyCheatState  @0x18830  (ret 0xc)
// @address: 0x18830
// @size:    0x380
// ============================================================================
int CCheatOwner::LoadBootyCheatState(int a1, int a2, int a3)
{
    return 0;
}

// ============================================================================
// LoadCheatConfigEx  @0x205c0  (ret 8)
// @address: 0x205c0
// @size:    0x741
// ============================================================================
int CCheatConfigMgr::LoadCheatConfigEx(int a1, int a2)
{
    return 1;
}

// ============================================================================
// LoadCheatConfig  @0x22e60  (void)
// @address: 0x22e60
// @size:    0x1BE
// ============================================================================
int CCheatConfigMgr::LoadCheatConfig(int a1)
{
    CString def("");
    CString sink;

    SYSTEMTIME st;
    GetLocalTime(&st);

    int n = g_buteMgr.GetIntDef("Cheatz", "NumCheatz", 0);
    if (n >= 1) {
        for (int i = 1; i <= n; i++) {
            char buf[32];
            sprintf(buf, "Cheat%i", i);

            int em = g_buteMgr.GetIntDef("Cheatz", buf, 0);
            int ey = g_buteMgr.GetIntDef("Cheatz", buf, 0);

            if (em != 0 && ey != 0) {
                unsigned short cy = st.wYear;
                unsigned short cm = st.wMonth;
                if ((int)cy > ey || ((int)cy == ey && (int)cm > em))
                    continue;
            }

            char *v = g_buteMgr.GetString("Cheatz", buf);
            if (v == 0) continue;

            int nc = g_buteMgr.GetIntDef("Cheatz", v, 0);
            if (nc == 1) {
                char *name = g_buteMgr.GetStringDef("Cheatz", v, (ButeString *)&sink)->m_pchData;
                int vi = g_buteMgr.GetIntDef("Cheatz", v, 0x807b);
                ((CheatRegCall *)this)->Call(name, vi, 1);
            } else {
                char *name = g_buteMgr.GetStringDef("Cheatz", v, (ButeString *)&sink)->m_pchData;
                int vi = g_buteMgr.GetIntDef("Cheatz", v, 0x807b);
                ((CheatRegCall *)this)->Call(name, vi, 0);
            }
        }
    }
    return 1;
}

// ============================================================================
// ShowMonolithDanceMessage  @0x396f0
// @address: 0x396f0
// @size:    0x2B8
// ============================================================================
int CMonolithDancer::ShowMonolithDanceMessage()
{
    return 1;
}
