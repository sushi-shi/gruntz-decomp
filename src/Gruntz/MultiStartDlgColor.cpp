// MultiStartDlgColor.cpp - the CMultiStartDlg multiplayer colour/name/chat dialog
// handler cluster (the 0xc3830-0xc4000 band), re-homed from the GapFunctions stubs.
// Every one runs on the ONE multiplayer-start dialog (CMultiStartDlg, this=ecx): the
// four per-slot colour swatches (0x501/0x503/0x505/0x507) each pop the modal
// CBattlezDlgColors picker and, on IDOK, claim the colour through the CNetSessHost
// +0x5c command-buffer facet (SelectColor @0xc4b60), re-drive the connect state, and
// invalidate the swatch. OnCustomWorld runs the CBattlezDlgCustom name dialog.
//
// Homed in its own /GX eh unit (not MultiStartDlgWorld/Roster.cpp) so it can't perturb
// those TUs' matched methods; it reuses the shared Dialogs.h dialog models, the
// canonical CMulti game-state (Multi.h @0x64bd5c) and CFocusSlot roster record
// (GameRegistry.h). Every callee/global is external (reloc-masked); field names are
// placeholders, only offsets + code bytes are load-bearing.
#include <Gruntz/Dialogs.h>
#include <Gruntz/Multi.h>        // the real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <Gruntz/GameRegistry.h> // CFocusSlot (the per-player roster record, stride 0x238)
#include <Net/NetSessHost.h>     // CNetSessHost::SelectColor (0xc4b60), the +0x5c facet
#include <rva.h>

// The multiplayer game-state singleton (a CMulti, xref-proven). DATA reloc-masks
// against ReconBatch2's home.
DATA(0x0024bd5c)
extern CMulti* g_64bd5c;
// The shared empty-string literal (0x6293f4; homed in NetMgrReportError.cpp).
extern "C" char g_emptyString[];
// USER32 entry points reached through the game's own IAT-style function pointers
// (ff 15 [ptr]); the colour/name handlers redraw / walk child windows through them.
DATA(0x002c44f0)
extern BOOL(WINAPI* g_pInvalidateRect)(HWND, const RECT*, BOOL);
DATA(0x002c44d8)
extern HWND(WINAPI* g_pGetWindow)(HWND, UINT);

// ---------------------------------------------------------------------------
// Per-slot colour handlers (0xc3830/0xc3950/0xc3a70/0xc3b90). Slot N owns swatch
// control 0x501+2*N. The pick is allowed when the host set the slot's colour gate
// (m_164==0) or when it is unlocked (m_16c==0) and owned by us (m_168==m_hostIndex).
// All four are byte-exact code (~99.84%); residual is two reloc/regalloc artifacts:
// the /GX scope-table push addend (delinker names it Unwind@005dda10+8 vs our own
// $L scope table at +0) and a single eax-vs-ecx coin-flip on the InvalidateRect hwnd
// load (`mov ecx,[eax+0x1c]` retail vs `mov eax,...`). Neither is source-steerable.
// @early-stop
// reloc scope-table addend + InvalidateRect-hwnd eax/ecx regalloc coin-flip (~99.84%).
RVA(0x000c3830, 0xd1)
void CMultiStartDlg::OnColorSlot0() {
    CMulti* mp = g_64bd5c;
    if ((mp->m_isHost == 0 || ((CFocusSlot*)m_host)[0].m_164 != 0)
        && (((CFocusSlot*)m_host)[0].m_16c != 0
            || ((CFocusSlot*)m_host)[0].m_168 != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 0, 1, 0);
    if (dlg.DoModal() == 1) {
        if (((CNetSessHost*)this)->SelectColor(0, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x501)->m_hWnd;
            g_pInvalidateRect(h, 0, 1);
        }
    }
}

RVA(0x000c3950, 0xd1)
void CMultiStartDlg::OnColorSlot1() {
    CMulti* mp = g_64bd5c;
    if ((mp->m_isHost == 0 || ((CFocusSlot*)m_host)[1].m_164 != 0)
        && (((CFocusSlot*)m_host)[1].m_16c != 0
            || ((CFocusSlot*)m_host)[1].m_168 != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 1, 1, 0);
    if (dlg.DoModal() == 1) {
        if (((CNetSessHost*)this)->SelectColor(1, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x503)->m_hWnd;
            g_pInvalidateRect(h, 0, 1);
        }
    }
}

RVA(0x000c3a70, 0xd1)
void CMultiStartDlg::OnColorSlot2() {
    CMulti* mp = g_64bd5c;
    if ((mp->m_isHost == 0 || ((CFocusSlot*)m_host)[2].m_164 != 0)
        && (((CFocusSlot*)m_host)[2].m_16c != 0
            || ((CFocusSlot*)m_host)[2].m_168 != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 2, 1, 0);
    if (dlg.DoModal() == 1) {
        if (((CNetSessHost*)this)->SelectColor(2, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x505)->m_hWnd;
            g_pInvalidateRect(h, 0, 1);
        }
    }
}

RVA(0x000c3b90, 0xd1)
void CMultiStartDlg::OnColorSlot3() {
    CMulti* mp = g_64bd5c;
    if ((mp->m_isHost == 0 || ((CFocusSlot*)m_host)[3].m_164 != 0)
        && (((CFocusSlot*)m_host)[3].m_16c != 0
            || ((CFocusSlot*)m_host)[3].m_168 != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 3, 1, 0);
    if (dlg.DoModal() == 1) {
        if (((CNetSessHost*)this)->SelectColor(3, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x507)->m_hWnd;
            g_pInvalidateRect(h, 0, 1);
        }
    }
}

// ~CBattlezDlgCustom (0x17140, owned by Dialogs.cpp) redeclared `inline` here so /Ob1
// inlines OnCustomWorld's teardown (member ~CString m_customName + base ~CDialog as
// separate calls) exactly as retail did - the out-of-line ??1 call misses that shape.
inline CBattlezDlgCustom::~CBattlezDlgCustom() {}

// ---------------------------------------------------------------------------
// OnCustomWorld (0xc3cb0): double-click the world combo (0x4ff). Host-only: run the
// modal CBattlezDlgCustom name dialog, and on IDOK with a non-empty name uppercase it
// into the combo's edit child and commit it as the game's custom world/host name.
// ---------------------------------------------------------------------------
// @early-stop
// Same wall family as the sibling CBattlezDlg::ShowCustomDlg (Dialogs.cpp, ~92.9%):
// the inlined ~CBattlezDlgCustom teardown, /GX EH trylevel numbering (retail 0/1/2/-1
// vs 0/1/-1), the child!=0 branch polarity, and an esi-save shrink-wrap our newer
// codegen does that MSVC5 didn't - none source-steerable. Body byte-faithful. ~86.5%.
RVA(0x000c3cb0, 0x128)
void CMultiStartDlg::OnCustomWorld() {
    if (g_64bd5c->m_isHost == 0) {
        return;
    }
    CBattlezDlgCustom dlg(0);
    if (dlg.DoModal() == 1 && dlg.m_customName.GetLength() != 0) {
        CWnd* child = CWnd::FromHandle(g_pGetWindow(GetDlgItem(0x4ff)->m_hWnd, GW_CHILD));
        if (child != 0) {
            dlg.m_customName.MakeUpper();
            child->SetWindowTextA((LPCTSTR)dlg.m_customName);
            m_6c = 1;
            g_64bd5c->m_5b0 = 1;
            g_64bd5c->m_5b8 = (LPCTSTR)dlg.m_customName;
            g_64bd5c->m_5b4 = g_emptyString;
            g_64bd5c->Commit3ada(0);
        }
    }
}
