// ShowMultiDlg.cpp - CNetMgr::ShowMultiStartDlg (0xb86c0) and the ~CMultiStartDlg
// destructor COMDAT (0xb8960) the linker placed next to it. Both sit in one
// contiguous retail .text block (0xb86c0..0xb89b9), inside the CNetMgr region -
// ShowMultiStartDlg STACK-constructs a CMultiStartDlg and runs it modally, so its
// dtor COMDAT was emitted here beside it. Split out of the Dialogs.cpp aggregate
// (matcher-1 de-fragmentation).
//
// `this` is the CNetMgr, modeled minimally as CNetMgrLite over only the touched
// offsets (NetMgr.cpp's <Mfc.h>-based CNetMgr/CWnd world is header-incompatible
// with these minimal-MFC dialog models); the class name is cosmetic - retail
// carries no symbols, objdiff matches by RVA.
//
// Built /GX (eh): the modal dialog + its CObList/CString/CDialog member dtors
// unwind inline across the exits. Field names are placeholders (m_<hexoffset>).
// ---------------------------------------------------------------------------
#include <Gruntz/Dialogs.h>
#include <Gruntz/GruntzMgr.h> // CGruntzMgr / CModalDialog (ExitModalUI, FindOptionsSlot)
#include <Gruntz/Multi.h>     // <Mfc.h> -> CMapStringToOb
#include <rva.h>
#include <Globals.h> // s_GameKey, g_dlgResultSink

// ===========================================================================
// CNetMgr::ShowMultiStartDlg @0x0b86c0  (/GX EH frame)
// On modal result 1 it either nudges the host (Sub386e, m_528 set) or, when sound
// is enabled, re-fires the kill cue throttled by g_killCueClock, then Sleeps 250ms;
// otherwise it tears down the pending local player and reports removal. The dlg's
// CObList/CString/CDialog member dtors unwind inline across the three /GX exits.
// ===========================================================================
// @early-stop
// 81% - EH dtor-emission wall. Retail INLINES the three CMultiStartDlg member
// dtors (~CObList m_74 @0x1b5d78, ~CString m_70 @0x1b9cde, ~CDialog base
// @0x1ba51d) with per-site states 2/1, 4/3, 6/5 at each of the three /GX exits;
// our build instead emits ONE out-of-line `call ~CMultiStartDlg` per exit. The
// inline form only falls out when the class dtor is implicit, but CMultiStartDlg
// must keep its explicit out-of-line ~CMultiStartDlg (the matched/parked function
// @0x0b8960) - so the member-dtor inlining cannot be steered without regressing
// that. All control flow + the logic (modal run, m_528/FindRec/m_538 teardown,
// the registry GAME_KEY cue throttle, Sleep) is byte-aligned; only the three
// cleanup sites differ.
DATA(0x0021ab20)
extern i32 g_sndEnabled; // ?g_sndEnabled@@3HA
DATA(0x0021ab24)
extern i32 g_sndCueTag; // ?g_sndCueTag@@3HA
DATA(0x002bf3c0)
extern i32 g_killCueClock; // _g_killCueClock

// The cue emitter held at record+0x10; Trigger @0x1360d0 (__thiscall, 4 args).
struct CCueEmitter {
    // Trigger @0x1360d0 IS CSoundCueMgr::ConfigureItem; cast at the call.
};
// The FindRec / registry-lookup record. Only the touched fields are named.
struct CNetCueRec {
    char m_pad0[8];
    i32 m_8; // +0x08
    char m_padc[0x10 - 0xc];
    CCueEmitter* m_10; // +0x10
    i32 m_14;          // +0x14  last-fire clock
    i32 m_18;          // +0x18  min interval
    char m_pad1c[0x20 - 0x1c];
    i32 m_20; // +0x20
};
// The embedded registry/bute object at (m_c->m_28 + 0x10); Lookup @0x1b8438.
struct CRegBute {}; // MFC CMapStringToOb (Lookup @0x1b8438); cast at the call
struct CNetCfgSub { // m_c->m_28
    char m_pad0[0x10];
    CRegBute m_10;             // +0x10  embedded registry/bute (Lookup 0x1b8438)
    char m_pad11[0x30 - 0x11]; // to +0x30
    i32 m_30;                  // +0x30
};
struct CNetCfg { // m_c
    char m_pad0[0x28];
    CNetCfgSub* m_28; // +0x28
};
// cdecl ILT-thunk helpers.
void NetCueReset_3bbb(i32 a, i32 b); // 0x3bbb
void ActiveWait(i32 ms);             // 0x13dfe0 busy-wait

class CNetMgrLite {
public:
    char m_pad0[4];
    CGruntzMgr* m_4; // +0x04
    char m_pad8[0xc - 8];
    CNetCfg* m_c; // +0x0c
    char m_pad10[0x528 - 0x10];
    i32 m_528; // +0x528
    char m_pad52c[0x538 - 0x52c];
    i32 m_538; // +0x538
    char m_pad53c[0x5c0 - 0x53c];
    i32 m_5c0; // +0x5c0

    i32 ShowMultiStartDlg();    // 0x0b86c0
    void Sub1d70(i32 a);        // 0x1d70   __thiscall self-call
    void Sub2e82(i32 a, i32 b); // 0x2e82   __thiscall self-call
    void Sub386e();             // 0x386e   __thiscall self-call
};

RVA(0x000b86c0, 0x206)
i32 CNetMgrLite::ShowMultiStartDlg() {
    CMultiStartDlg dlg((i32)m_4, 0);
    i32 r = m_4->ExitModalUI((CModalDialog*)&dlg, 0);
    g_dlgResultSink = 0;
    if (r != 1) {
        if (m_528 != 0) {
            CNetCueRec* rec = (CNetCueRec*)m_4->FindOptionsSlot(m_5c0);
            if (rec == 0) {
                return 0;
            }
            rec->m_20 = 0;
            NetCueReset_3bbb(rec->m_8, 1);
            Sub1d70(0);
        }
        if (m_528 == 0 && m_538 == 0) {
            Sub2e82(0x3ea, 1);
        }
        return 0;
    }
    // r == 1
    if (m_528 != 0) {
        Sub386e();
    } else {
        if (m_c->m_28->m_30 == 0) {
            CNetCueRec* rec = 0;
            ((CMapStringToOb*)&m_c->m_28->m_10)->Lookup(s_GameKey, (CObject*&)rec);
            if (rec != 0) {
                i32 snd = g_sndEnabled;
                i32 cue = g_sndCueTag;
                if (snd != 0) {
                    i32 clk = g_killCueClock;
                    if ((u32)(clk - rec->m_14) >= (u32)rec->m_18) {
                        rec->m_14 = clk;
                        ((CSoundCueMgr*)rec->m_10)->ConfigureItem(cue, 0, 0, 0);
                    }
                }
            }
        }
        ActiveWait(0xfa);
    }
    return 1;
}

// ~CMultiStartDlg @0x0b8960 - destroy the CObList member m_74 then the CString
// member m_70, then chain the NAFXCW ~CDialog base dtor (all reloc-masked). /GX
// frame unwinds the half-torn object across the member dtors.
// @early-stop
// vptr-restamp-presence wall (docs/patterns/eh-dtor-vptr-restamp-presence.md): same
// as ~CBattlezDlg - our polymorphic model emits one extra most-derived vptr re-stamp
// at entry that retail elided; the member/base teardown chain is otherwise byte-exact.
RVA(0x000b8960, 0x59)
CMultiStartDlg::~CMultiStartDlg() {}

SIZE_UNKNOWN(CCueEmitter);
SIZE_UNKNOWN(CNetCueRec);
SIZE_UNKNOWN(CRegBute);
SIZE_UNKNOWN(CNetCfgSub);
SIZE_UNKNOWN(CNetCfg);
SIZE_UNKNOWN(CNetMgrLite);
