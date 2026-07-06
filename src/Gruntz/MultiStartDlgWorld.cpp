// MultiStartDlgWorld.cpp - CMultiStartDlg::SetupWorldCombo (0xc1840), a /GX EH
// method of the multiplayer-start dialog. It fills the 0x4ff "world" combo from
// the GAME_MULTI registry path (each entry name uppercased through a scratch
// CString), makes the combo's edit child read-only, selects the first item, and
// subclasses the edit child's window-proc (saving the old proc in g_64bdc0).
//
// Homed in its own eh unit (not Dialogs.cpp) so it can't perturb that TU's parked
// dtors; it reuses the shared Dialogs.h class models. Every callee is a NAFXCW /
// engine / import thunk that reloc-masks; the "GAME_MULTI" $SG literal reloc-masks
// against the matched string symbol; only offsets + code bytes are load-bearing.
#include <Gruntz/Dialogs.h>
#include <Bute/SymTab.h>
#include <Gruntz/Multi.h>      // the real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <Gruntz/NetDlgHost.h> // CMultiStartDlg::m_host (its +0x34 m_registry is a CSymParser)
#include <Bute/SymParser.h>    // CSymParser::ResolvePath (0x13c030), the world name registry
#include <rva.h>
#include <Globals.h>

// The old edit-child window-proc snapshot the subclass saves (reloc-masked DATA).

// The subclass window-proc installed on the combo's edit child (0x4c1a10). Only
// its address is taken (push offset -> DIR32 reloc-masks).
extern "C" i32 CALLBACK WndProc_c1a10(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// The multiplayer game-state singleton (a CMulti, xref-proven); Sub_c3e30 commits
// the selected world/host name into it. DATA reloc-masks against ReconBatch2's home.
DATA(0x0024bd5c)
extern CMulti* g_64bd5c;
// The shared empty-string literal (0x6293f4; homed in NetMgrReportError.cpp).
extern "C" char g_emptyString[];

// The GAME_MULTI registry path -> a name registry. m_host is the canonical CNetDlgHost;
// its +0x34 m_registry is the real Bute CSymParser (ResolvePath 0x13c030 - proven), which
// returns a CSymTab symbol table. The table is iterated FirstSym/NextSym2 (first entry)
// then NextSym3 (advance) - CSymTab methods; kept as this TU's iteration view MpSymTable
// (each returned payload's +0x00 is the entry name). Folding MpSymTable onto CSymTab is
// proven but DEFERRED (its payload/item identity - CSymRec vs child scope - is unsettled).
struct MpSymItem {
    char* m_name; // +0x00  entry name (LPCSTR)
};

// ---------------------------------------------------------------------------
// BuildNamedGruntTable (0xc16b0) - seeds the 4 default named-grunt CString globals
// (the contiguous array at 0x64bdb0, read back as the multiplayer channel labels)
// with their default names via CString::operator=(LPCSTR) (FUN_001b9d4c, reloc-
// masked __thiscall). Re-homed here from the deleted BacklogStateLoaders.cpp: its
// caller is the (unmodeled) CMultiStartDlg init routine at 0xc1690, one function
// before CMultiStartDlg itself (0xc1750) - the multiplayer-start dialog cluster.
// ---------------------------------------------------------------------------
struct EngStrAssign {
    char* m_pszData;
    void operator=(const char* s); // CString::operator=, FUN_001b9d4c
};
// 4 contiguous CString globals at 0x64bdb0 (defined in the engine's data).
DATA(0x0064bdb0)
extern EngStrAssign g_gruntNames[4];

RVA(0x000c16b0, 0x3d)
void BuildNamedGruntTable() {
    g_gruntNames[0] = "Beefy";
    g_gruntNames[1] = "Zed";
    g_gruntNames[2] = "Serra";
    g_gruntNames[3] = "Jebediah";
}

RVA(0x000c1840, 0x16e)
i32 CMultiStartDlg::SetupWorldCombo() {
    CWnd* combo = GetDlgItem(0x4ff);
    if (combo == 0) {
        return 0;
    }
    CSymTab* st = (CSymTab*)((CNetDlgHost*)m_host)->m_registry->ResolvePath("GAME_MULTI");
    if (st == 0) {
        return 0;
    }
    MpSymItem* item = (MpSymItem*)st->NextSym2(st->FirstSym());
    while (item != 0) {
        CString name(item->m_name);
        name.MakeUpper();
        SendMessageA(combo->m_hWnd, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)name);
        item = (MpSymItem*)st->NextSym3(item);
    }
    CWnd* combo2 = GetDlgItem(0x4ff);
    CWnd* child = CWnd::FromHandle(GetWindow(combo2->m_hWnd, GW_CHILD));
    if (child == 0) {
        return 0;
    }
    SendMessageA(child->m_hWnd, EM_SETREADONLY, 1, 0);
    SendMessageA(combo->m_hWnd, CB_SETCURSEL, 0, 0);
    HWND__* h = child->m_hWnd;
    g_64bdc0 = GetWindowLongA(h, GWL_WNDPROC);
    SetWindowLongA(h, GWL_WNDPROC, (i32)WndProc_c1a10);
    Sub_c3e30();
    return 1;
}

// CMultiStartDlg::Sub_c3e30 (0xc3e30) - re-homed from src/Stub/ApiCallers.cpp
// (ReplayDlg_c3e30::OnReset); the caller SetupWorldCombo runs it as a self-call. When
// this is the host, read the current selection of the 0x4ff world combo, and if its
// text is non-empty commit it as the game's world/host name into the CMulti game-state
// (m_5b4 = name, m_5b8 = "", m_5b0 = 0, Commit3ada). The /GX EH frame unwinds the local
// scratch CString. GetLBText (CComboBox::GetLBText 0x1ce7db) / operator= / Commit3ada /
// SendMessageA all reloc-mask; only offsets + code bytes are load-bearing.
RVA(0x000c3e30, 0xfe)
void CMultiStartDlg::Sub_c3e30() {
    if (g_64bd5c->m_isHost != 0) {
        CWnd* item = GetDlgItem(0x4ff);
        if (item != 0) {
            i32 r = SendMessageA(item->m_hWnd, 0x147, 0, 0);
            if (r != -1) {
                CString name;
                item->GetLBText1ce7db(r, name);
                if (name.GetLength() != 0) {
                    m_6c = 0;
                }
                g_64bd5c->m_5b0 = 0;
                g_64bd5c->m_5b8 = g_emptyString;
                g_64bd5c->m_5b4 = (LPCTSTR)name;
                g_64bd5c->Commit3ada(0);
            }
        }
    }
}

SIZE_UNKNOWN(EngStrAssign);
SIZE_UNKNOWN(MpSymItem);
SIZE_UNKNOWN(MpSymTable);
