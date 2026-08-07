#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <EmptyString.h>
#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CustomMapSelection.h>
#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/ParseSource.h>
#include <MsgParam.h>
#include <Utils/RegistryHelper.h>

#include <stdio.h>
#include <string.h>

DATA(0x001e8d10)
const AFX_MSGMAP CBattlezDlgColors::messageMap = {
    &CDialog::messageMap,
    &CBattlezDlgColors::_messageEntries[0],
};

DATA(0x001e8d18)
const AFX_MSGMAP_ENTRY CBattlezDlgColors::_messageEntries[] = {
    {WM_MEASUREITEM,
     0,
     0,
     0,
     AfxSig_vOWNER,
     reinterpret_cast<AFX_PMSG>(&CBattlezDlgColors::OnMeasureItem)}, // API-forced MFC seam.
    {WM_DRAWITEM,
     0,
     0,
     0,
     AfxSig_vOWNER,
     reinterpret_cast<AFX_PMSG>(&CBattlezDlgColors::OnDrawItem)}, // API-forced MFC seam.
    {WM_COMMAND,
     CBN_DBLCLK,
     0x515,
     0x515,
     AfxSig_vv,
     reinterpret_cast<AFX_PMSG>(&CBattlezDlg::OnOkCommand)}, // API-forced MFC seam.
    {0, 0, 0, 0, AfxSig_end, 0},
};

RVA(0x00017930, 0x3a)
CBattlezDlgColors::CBattlezDlgColors(CGruntzMgr* mgr, i32 slotIndex, i32 networked, CWnd* pParent)
    : CDialog(0xc2, pParent) {
    m_slots = mgr;
    m_slotIndex = slotIndex;
    m_pickedColor = TINT_ORANGE;
    m_networked = networked;
}
RVA_COMPGEN(0x00017980, 0x1e, ??_GCBattlezDlgColors@@UAEPAXI@Z)

RVA(0x000179b0, 0xcb)
void CBattlezDlgColors::DoDataExchange(CDataExchange* pDX) {
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM);
    if (pDX->m_bSaveAndValidate) {
        CWnd* lb = GetDlgItem(0x515);
        pSend = ::SendMessageA;
        long sel = pSend(lb->m_hWnd, LB_GETCURSEL, 0, 0);
        long data = pSend(lb->m_hWnd, LB_GETITEMDATA, sel, 0);
        m_pickedColor = static_cast<ColorTint>(data);
        if (data >= TINT_COUNT) {
            m_pickedColor = TINT_WHITE;
        }
    } else {
        CWnd* lb = GetDlgItem(0x515);
        pSend = ::SendMessageA;
        for (i32 i = 0; i < 0x11; i++) {
            i32 avail = 1;
            GruntzPlayer* rec = m_slots->m_options;
            for (i32 j = 0; j < 4; j++) {
                if (rec->m_liveGate != 0 && IDX(rec->m_colorIndex) == i) {
                    avail = 0;
                }
                rec++;
            }
            if (avail) {

                MsgParam name;
                name.m_str = "Color";
                long idx = pSend(lb->m_hWnd, LB_ADDSTRING, 0, name.m_lparam);
                pSend(lb->m_hWnd, LB_SETITEMDATA, idx, i);
            }
        }
        pSend(lb->m_hWnd, LB_SETCURSEL, 0, 0);
    }
}

RVA(0x00017ac0, 0x6)
const AFX_MSGMAP* CBattlezDlgColors::GetMessageMap() const {
    return &messageMap;
}

RVA(0x00017ae0, 0x20)
void CBattlezDlgColors::OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis) {
    lpmis->itemWidth = 0xc8;
    lpmis->itemHeight = 0x1e;
    CWnd::OnMeasureItem(nIDCtl, lpmis);
}
RVA(0x00017b10, 0x0)
// @confidence: high
// @source: msgmap-pfn (the AFX_MSGMAP_ENTRY pfn slot names this handler)
// @stub
void CBattlezDlgColors::OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis) {}

RVA(0x00017d40, 0x8)
void CBattlezDlg::OnOkCommand() {
    OnOK();
}

RVA(0x00018030, 0x56)
CBattlezDlgCustom::CBattlezDlgCustom(CWnd* pParent) : CDialog(0xc3, pParent) {}
RVA_COMPGEN(0x000180b0, 0x1e, ??_GCBattlezDlgCustom@@UAEPAXI@Z)
