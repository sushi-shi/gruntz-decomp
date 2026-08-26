#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CustomMapSelection.h>
#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <MsgParam.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>
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
    {WM_MEASUREITEM, 0, 0, 0, AfxSig_vOWNER, GZ_MFC_PMSG(&CBattlezDlgColors::OnMeasureItem)},
    {WM_DRAWITEM, 0, 0, 0, AfxSig_vOWNER, GZ_MFC_PMSG(&CBattlezDlgColors::OnDrawItem)},
    {WM_COMMAND,
     CBN_DBLCLK,
     CTRL_COLOR_LIST,
     CTRL_COLOR_LIST,
     AfxSig_vv,
     GZ_MFC_PMSG(&CBattlezDlg::OnOkCommand)},
    {0, 0, 0, 0, AfxSig_end, 0},
};

RVA(0x00017930, 0x3a)
CBattlezDlgColors::CBattlezDlgColors(
    CGruntzMgr* gameManager,
    i32 slotIndex,
    i32 networked,
    CWnd* pParent
)
    : CDialog(0xc2, pParent) {
    m_gameManager = gameManager;
    m_slotIndex = slotIndex;
    m_pickedColor = TINT_ORANGE;
    m_networked = networked;
}
RVA_COMPGEN(0x00017980, 0x1e, ??_GCBattlezDlgColors@@UAEPAXI@Z)

RVA(0x000179b0, 0xcb)
void CBattlezDlgColors::DoDataExchange(CDataExchange* pDX) {
    LRESULT(WINAPI * sendMessage)(HWND, UINT, WPARAM, LPARAM);
    if (pDX->m_bSaveAndValidate) {
        CWnd* colorList = GetDlgItem(CTRL_COLOR_LIST);
        sendMessage = ::SendMessageA;
        long selection = sendMessage(colorList->m_hWnd, LB_GETCURSEL, 0, 0);
        long color = sendMessage(colorList->m_hWnd, LB_GETITEMDATA, selection, 0);
        m_pickedColor = static_cast<ColorTint>(color);
        if (color >= TINT_COUNT) {
            m_pickedColor = TINT_WHITE;
        }
    } else {
        CWnd* colorList = GetDlgItem(CTRL_COLOR_LIST);
        sendMessage = ::SendMessageA;
        for (i32 i = 0; i < 0x11; i++) {
            b32 available = true;
            GruntzPlayer* player = m_gameManager->m_players;
            for (i32 j = 0; j < 4; j++) {
                if (player->m_active != false && IDX(player->m_color) == i) {
                    available = false;
                }
                player++;
            }
            if (available) {

                MsgParam name;
                name.m_str = "Color";
                long itemIndex = sendMessage(colorList->m_hWnd, LB_ADDSTRING, 0, name.m_lparam);
                sendMessage(colorList->m_hWnd, LB_SETITEMDATA, itemIndex, i);
            }
        }
        sendMessage(colorList->m_hWnd, LB_SETCURSEL, 0, 0);
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
RVA(0x00017b10, 0x1b8)
void CBattlezDlgColors::OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis) {
    CWnd* colorList = GetDlgItem(CTRL_COLOR_LIST);
    if (nIDCtl == CTRL_COLOR_LIST) {
        CDC dc;
        dc.Attach(lpdis->hDC);
        COLORREF color;
        switch (static_cast<ColorTint>(
            ::SendMessageA(colorList->m_hWnd, LB_GETITEMDATA, lpdis->itemID, 0)
        )) {
            case TINT_DKBLUE:
                color = 0x800000;
                break;
            case TINT_DKGREEN:
                color = 0x008000;
                break;
            case TINT_TURQ:
                color = 0x808000;
                break;
            case TINT_DKRED:
                color = 0x000080;
                break;
            case TINT_PURPLE:
                color = 0x800080;
                break;
            case TINT_DKYELLOW:
                color = 0x008080;
                break;
            case TINT_GREY:
                color = 0x808080;
                break;
            case TINT_BLUE:
                color = 0xff0000;
                break;
            case TINT_GREEN:
                color = 0x00ff00;
                break;
            case TINT_CYAN:
                color = 0xffff00;
                break;
            case TINT_RED:
                color = 0x0000ff;
                break;
            case TINT_PINK:
                color = 0xff00ff;
                break;
            case TINT_YELLOW:
                color = 0x00ffff;
                break;
            case TINT_WHITE:
                color = 0xffffff;
                break;
            case TINT_ORANGE:
                color = 0x0080ff;
                break;
            case TINT_HOTPINK:
                color = 0x8000ff;
                break;
            case TINT_BLACK:
            default:
                color = 0;
                break;
        }
        CBrush brush(color);
        FillRect(dc.m_hDC, &lpdis->rcItem, brush);
        dc.Detach();
    }
    CWnd::OnDrawItem(nIDCtl, lpdis);
}

RVA(0x00017d40, 0x8)
void CBattlezDlg::OnOkCommand() {
    OnOK();
}

RVA(0x00018030, 0x56)
CBattlezDlgCustom::CBattlezDlgCustom(CWnd* pParent) : CDialog(0xc3, pParent) {}
RVA_COMPGEN(0x000180b0, 0x1e, ??_GCBattlezDlgCustom@@UAEPAXI@Z)
