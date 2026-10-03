#include <StdAfx.h>

#include <Ints.h>

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

void CBattlezDlgColors::DoDataExchange(CDataExchange* pDX) {
    if (pDX->m_bSaveAndValidate) {
        CListBox* colorList = static_cast<CListBox*>(GetDlgItem(CTRL_COLOR_LIST));
        long selection = colorList->GetCurSel();
        long color = colorList->GetItemData(selection);
        m_pickedColor = static_cast<ColorTint>(color);
        if (color >= TINT_COUNT) {
            m_pickedColor = TINT_WHITE;
        }
    } else {
        CListBox* colorList = static_cast<CListBox*>(GetDlgItem(CTRL_COLOR_LIST));
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

                long itemIndex = colorList->AddString("Color");
                colorList->SetItemData(itemIndex, i);
            }
        }
        colorList->SetCurSel(0);
    }
}

BEGIN_MESSAGE_MAP(CBattlezDlgColors, CDialog)
    ON_WM_MEASUREITEM()
    ON_WM_DRAWITEM()
    ON_LBN_DBLCLK(CTRL_COLOR_LIST, CBattlezDlgColors::OnOkCommand)
END_MESSAGE_MAP()

void CBattlezDlgColors::OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis) {
    lpmis->itemWidth = 0xc8;
    lpmis->itemHeight = 0x1e;
    CWnd::OnMeasureItem(nIDCtl, lpmis);
}

void CBattlezDlgColors::OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis) {
    CListBox* colorList = static_cast<CListBox*>(GetDlgItem(CTRL_COLOR_LIST));
    if (nIDCtl == CTRL_COLOR_LIST) {
        CDC dc;
        dc.Attach(lpdis->hDC);
        COLORREF color;
        color = TintColorRef(static_cast<ColorTint>(colorList->GetItemData(lpdis->itemID)));
        CBrush brush(color);
        dc.FillRect(&lpdis->rcItem, &brush);
        dc.Detach();
    }
    CWnd::OnDrawItem(nIDCtl, lpdis);
}

void CBattlezDlgColors::OnOkCommand() {
    OnOK();
}

CBattlezDlgCustom::CBattlezDlgCustom(CWnd* pParent) : CDialog(0xc3, pParent) {}
