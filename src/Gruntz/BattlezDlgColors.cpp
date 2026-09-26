#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/ColorTintRef.h>
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
#include <Utils/RegMgr.h>

#include <stdio.h>
#include <string.h>

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
    if (pDX->m_bSaveAndValidate) {
        CWnd* colorList = GetDlgItem(CTRL_COLOR_LIST);
        long selection = colorList->SendMessageA(LB_GETCURSEL, 0, 0);
        long color = colorList->SendMessageA(LB_GETITEMDATA, selection, 0);
        m_pickedColor = static_cast<ColorTint>(color);
        if (color >= TINT_COUNT) {
            m_pickedColor = TINT_WHITE;
        }
    } else {
        CWnd* colorList = GetDlgItem(CTRL_COLOR_LIST);
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
                long itemIndex = colorList->SendMessageA(LB_ADDSTRING, 0, name.m_lparam);
                colorList->SendMessageA(LB_SETITEMDATA, itemIndex, i);
            }
        }
        colorList->SendMessageA(LB_SETCURSEL, 0, 0);
    }
}

RVA(0x00017ac0, 0x6)
DATA_MESSAGE_MAP(0x001e8d10, 0x001e8d18)
BEGIN_MESSAGE_MAP(CBattlezDlgColors, CDialog)
    ON_WM_MEASUREITEM()
    ON_WM_DRAWITEM()
    ON_LBN_DBLCLK(CTRL_COLOR_LIST, CBattlezDlgColors::OnOkCommand)
END_MESSAGE_MAP()

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
        color = TintColorRef(
            static_cast<ColorTint>(colorList->SendMessageA(LB_GETITEMDATA, lpdis->itemID, 0))
        );
        CBrush brush(color);
        FillRect(dc.m_hDC, &lpdis->rcItem, brush);
        dc.Detach();
    }
    CWnd::OnDrawItem(nIDCtl, lpdis);
}

RVA(0x00017d40, 0x8)
void CBattlezDlgColors::OnOkCommand() {
    OnOK();
}

RVA(0x00018030, 0x56)
CBattlezDlgCustom::CBattlezDlgCustom(CWnd* pParent) : CDialog(0xc3, pParent) {}
RVA_COMPGEN(0x000180b0, 0x1e, ??_GCBattlezDlgCustom@@UAEPAXI@Z)
