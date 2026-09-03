#include <rva.h>

#include <Gruntz/Dialogs.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CustomMapSelection.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <MsgParam.h>
#include <Net/NetLobby.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>
#include <Utils/RegMgr.h>

#include <stdio.h>
#include <string.h>

DATA(0x001e88b0)
const AFX_MSGMAP CBattlezDlg::messageMap = {
    &CDialog::messageMap,
    &CBattlezDlg::_messageEntries[0],
};

DATA(0x001e88b8)
const AFX_MSGMAP_ENTRY CBattlezDlg::_messageEntries[] = {

    ON_CBN_SELCHANGE(CTRL_PLAYER_TYPE0, CBattlezDlg::OnPlayerTypeSelection0)
        ON_CBN_SELCHANGE(CTRL_PLAYER_TYPE1, CBattlezDlg::OnPlayerTypeSelection1)
            ON_CBN_SELCHANGE(CTRL_PLAYER_TYPE2, CBattlezDlg::OnPlayerTypeSelection2)
                ON_CBN_SELCHANGE(CTRL_PLAYER_TYPE3, CBattlezDlg::OnPlayerTypeSelection3)

                    {WM_MEASUREITEM,
                     0,
                     0,
                     0,
                     AfxSig_vOWNER,

                     GZ_MFC_PMSG(&CBattlezDlg::OnMeasureItem)},
    {WM_DRAWITEM, 0, 0, 0, AfxSig_vOWNER, GZ_MFC_PMSG(&CBattlezDlg::OnDrawItem)},
    ON_BN_CLICKED(CTRL_PLAYER_COLOR0, CBattlezDlg::OnPlayerColor0) ON_BN_CLICKED(
        CTRL_PLAYER_COLOR1,
        CBattlezDlg::OnPlayerColor1
    ) ON_BN_CLICKED(CTRL_PLAYER_COLOR2, CBattlezDlg::OnPlayerColor2)
        ON_BN_CLICKED(CTRL_PLAYER_COLOR3, CBattlezDlg::OnPlayerColor3) ON_BN_CLICKED(
            0x42b,
            CBattlezDlg::ShowCustomDlg
        ) ON_CBN_SELCHANGE(0x4ff, CBattlezDlg::OnWorldSelectionChange)

            ON_CONTROL(0x200, 0x50a, CBattlezDlg::OnPlayerNameKillFocus0)
                ON_CONTROL(0x200, 0x50b, CBattlezDlg::OnPlayerNameKillFocus1)
                    ON_CONTROL(0x200, 0x50c, CBattlezDlg::OnPlayerNameKillFocus2)
                        ON_CONTROL(0x200, 0x50d, CBattlezDlg::OnPlayerNameKillFocus3)
                            ON_CONTROL(0x300, 0x50b, CBattlezDlg::OnPlayerNameChange1)
                                ON_CONTROL(0x300, 0x50a, CBattlezDlg::OnPlayerNameChange0)
                                    ON_CONTROL(0x300, 0x50c, CBattlezDlg::OnPlayerNameChange2)
                                        ON_CONTROL(0x300, 0x50d, CBattlezDlg::OnPlayerNameChange3)
    // API-forced MFC message-map representation seam.
    {WM_PAINT, 0, 0, 0, AfxSig_vv, GZ_MFC_PMSG(&CBattlezDlg::OnPaint)},
    ON_CBN_SELCHANGE(CTRL_PLAYER_MAX_GRUNTZ0, CBattlezDlg::OnMaxGruntzSelection0)
        ON_CBN_SELCHANGE(CTRL_PLAYER_MAX_GRUNTZ1, CBattlezDlg::OnMaxGruntzSelection1)
            ON_CBN_SELCHANGE(CTRL_PLAYER_MAX_GRUNTZ2, CBattlezDlg::OnMaxGruntzSelection2)
                ON_CBN_SELCHANGE(
                    CTRL_PLAYER_MAX_GRUNTZ3,
                    CBattlezDlg::OnMaxGruntzSelection3
                ){0, 0, 0, 0, AfxSig_end, 0},
};
DATA(0x00229c50)
i32 g_battlezLastColors[4];
DATA(0x00229cf0)
i32 g_battlezLastDifficulties[4];
DATA(0x00229d00)
i32 g_battlezLastMaxGruntz[4];

DATA(0x00229d10)
WNDPROC g_savedDlgWndProc;
DATA(0x00229d14)
b32 g_battlezResetOptions;

RVA(0x00014b10, 0x5)
long CBattlezDlg::OnPaint() {
    return Default();
}

RVA(0x00014b30, 0x64)
CBattlezDlg::CBattlezDlg(CGruntzMgr* gameManager, CWnd* pParent) : CDialog(0xc0, pParent) {
    m_gameManager = gameManager;
    m_customNameFlag = false;
}

RVA_COMPGEN(0x00014bc0, 0x3, ?Serialize@CObject@@UAEXAAVCArchive@@@Z)
RVA_COMPGEN(0x00014be0, 0x1, ?AssertValid@CObject@@UBEXXZ)
RVA_COMPGEN(0x00014c00, 0x3, ?Dump@CObject@@UBEXAAVCDumpContext@@@Z)
RVA_COMPGEN(0x00014c20, 0xd, ?BeginModalState@CWnd@@UAEXXZ)
RVA_COMPGEN(0x00014c40, 0xd, ?EndModalState@CWnd@@UAEXXZ)
RVA_COMPGEN(0x00014c60, 0x1e, ??_GCBattlezDlg@@UAEPAXI@Z)
RVA_COMPGEN(0x00014c90, 0x47, ??1CBattlezDlg@@UAE@XZ)

// @early-stop
RVA(0x00014d00, 0xa68)
void CBattlezDlg::DoDataExchange(CDataExchange* pDX) {
    CRegMgr* reg = g_gameReg->m_settings;
    char key[0x100];
    i32 i;

    if (!pDX->m_bSaveAndValidate) {
        i32 defaultMax = g_buteMgr.GetDword("Battlez", "DefaultMaxGruntz", 8);
        for (i = 0; i < 4; i++) {
            sprintf(key, "LastMaxGruntz%d", i);
            g_battlezLastMaxGruntz[i] = reg->Get(key, defaultMax);
            sprintf(key, "LastDiff%d", i);
            g_battlezLastDifficulties[i] = reg->Get(key, 1);
            sprintf(key, "LastColour%d", i);
            g_battlezLastColors[i] = reg->Get(key, IDX(g_gameReg->m_players[i].m_color));
            g_gameReg->m_players[i].m_color = static_cast<ColorTint>(g_battlezLastColors[i]);
        }

        CWnd* comboChild = GetDlgItem(0x4ff)->GetWindow(GW_CHILD);
        if (comboChild == NULL) {
            return;
        }
        comboChild->SendMessageA(EM_SETREADONLY, 1, 0);
        comboChild->SetWindowTextA("");

        CWnd* combo = GetDlgItem(0x4ff);
        CRezDir* worlds = m_gameManager->m_resourceArchive->GetDirFromPath("GAME_BATTLEZ");
        if (worlds == NULL) {
            return;
        }
        CRezItm* entry = static_cast<CRezItm*>(worlds->GetFirstItem(worlds->GetFirstType()));
        i32 first = 1;
        while (entry != NULL) {
            CString upper(entry->GetName());
            upper.MakeUpper();
            CString display;
            char c = 0;
            for (i = 0; i < upper.GetLength() && c != '.'; i++) {
                c = upper[i];
                if (c != '.') {
                    display += c;
                }
            }
            combo->SendMessageA(
                CB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(static_cast<const char*>(display))
            );
            if (first != 0) {
                first = 0;
                comboChild->SetWindowTextA(display);
            }
            entry = static_cast<CRezItm*>(worlds->GetNextItem(entry));
        }
        combo->SendMessageA(CB_SETCURSEL, 0, 0);
        MsgParam prev;
        prev.m_long = GetWindowLongA(comboChild->m_hWnd, GWL_WNDPROC);
        g_savedDlgWndProc = prev.m_wndproc;

        MsgParam proc;
        proc.m_intProc = BattlezMapComboEditProc;
        SetWindowLongA(comboChild->m_hWnd, GWL_WNDPROC, proc.m_long);

        GetDlgItem(0x512)->SetWindowTextA("Battlez Setup");
        NetLobby::g_curDlg = GetSafeHwnd();

        for (i = 0; i < 4; i++) {
            if (i != 0) {
                GetPlayerTypeControl(i)
                    ->SendMessageA(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("None"));
                GetPlayerTypeControl(i)
                    ->SendMessageA(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Computer (easy)"));
                GetPlayerTypeControl(i)
                    ->SendMessageA(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Computer (normal)"));
                GetPlayerTypeControl(i)->SendMessageA(
                    CB_ADDSTRING,
                    0,
                    reinterpret_cast<LPARAM>("Computer (difficult)")
                );
            } else {
                GetPlayerTypeControl(i)
                    ->SendMessageA(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Human"));
            }
        }
        SetPlayerTypeSelection(0, 0);
        SetPlayerTypeSelection(1, 2);
        SetPlayerTypeSelection(2, 2);
        SetPlayerTypeSelection(3, 2);

        if (g_battlezResetOptions != false) {
            m_gameManager->m_players[1].m_difficulty = BZDIFF_NORMAL;
            m_gameManager->m_players[2].m_difficulty = BZDIFF_NORMAL;
            m_gameManager->m_players[3].m_difficulty = BZDIFF_NORMAL;
        } else {
            for (i = 1; i < 4; i++) {
                if (g_battlezLastDifficulties[i] != -1) {
                    SetPlayerTypeSelection(i, g_battlezLastDifficulties[i] + 1);
                    m_gameManager->m_players[i].m_difficulty =
                        static_cast<BattlezDifficulty>(g_battlezLastDifficulties[i]);
                } else {
                    SetPlayerTypeSelection(i, 0);
                }
            }
        }

        SetPlayerName(0, "Player");
        SetPlayerName(1, "Zed");
        SetPlayerName(2, "Serra");
        SetPlayerName(3, "Jebediah");
        SetMaxGruntzSelection(0, defaultMax);
        SetMaxGruntzSelection(1, defaultMax);
        SetMaxGruntzSelection(2, defaultMax);
        SetMaxGruntzSelection(3, defaultMax);
        for (i = 0; i < 4; i++) {
            if (g_battlezResetOptions == false) {
                SetMaxGruntzSelection(i, g_battlezLastMaxGruntz[i]);
                m_gameManager->m_players[i].m_maxGruntz = g_battlezLastMaxGruntz[i];
            }
            GruntzPlayer* slot = &m_gameManager->m_players[i];
            if (slot != NULL) {
                slot->m_active = true;
            }
        }
        for (i = 0; i < 4; i++) {
            CWnd* edit = GetPlayerNameControl(i);
            if (edit != NULL) {
                edit->SendMessageA(EM_LIMITTEXT, 9, 0);
            }
        }

        combo->EnableWindow(true);
        GetDlgItem(0x42b)->EnableWindow(true);
        GetDlgItem(IDOK)->EnableWindow(true);
        GetDlgItem(IDCANCEL)->EnableWindow(true);
        for (i = 0; i < 4; i++) {
            CWnd* typeControl = GetPlayerTypeControl(i);
            CWnd* nameControl = GetPlayerNameControl(i);
            CWnd* colorControl = GetPlayerColorControl(i);
            CWnd* maxGruntzControl = GetMaxGruntzControl(i);
            nameControl->EnableWindow(true);
            nameControl->SendMessageA(EM_SETREADONLY, 0, 0);
            colorControl->EnableWindow(true);
            typeControl->EnableWindow(true);
            maxGruntzControl->EnableWindow(true);
            if (GetPlayerTypeSelection(i) == 0) {
                nameControl->EnableWindow(false);
                if (i != 0) {
                    colorControl->EnableWindow(false);
                }
                if (i != 0) {
                    maxGruntzControl->EnableWindow(false);
                }
            }
        }
        if (GetPlayerTypeSelection(1) || GetPlayerTypeSelection(2) || GetPlayerTypeSelection(3)) {
            GetDlgItem(IDOK)->EnableWindow(true);
        } else {
            GetDlgItem(IDOK)->EnableWindow(false);
        }

        CustomMapSelection customMap =
            static_cast<CustomMapSelection>(reg->Get("CustomMap", IDX(CUSTOM_MAP_UNINITIALIZED)));
        if (customMap != CUSTOM_MAP_UNINITIALIZED) {
            char mapName[0x100];
            DWORD size = sizeof(mapName);
            reg->Get("LastMap", mapName, size, "");
            m_customNameFlag = IDX(customMap);
            if (customMap != CUSTOM_MAP_STANDARD) {
                sprintf(key, "custom\\%s", mapName);
                FILE* file = fopen(key, "rb");
                if (file != NULL) {
                    CWnd* child = GetDlgItem(0x4ff)->GetWindow(GW_CHILD);
                    if (child == NULL) {
                        return;
                    }
                    child->SetWindowTextA(mapName);
                    fclose(file);
                }
            } else {
                CWnd* child = GetDlgItem(0x4ff)->GetWindow(GW_CHILD);
                if (child == NULL) {
                    return;
                }
                child->SetWindowTextA(mapName);
            }
        } else {
            CWnd* child = GetDlgItem(0x4ff)->GetWindow(GW_CHILD);
            CString mapName;
            if (mapName.LoadStringA(0x81ab)) {
                child->SetWindowTextA(mapName);
                SetMaxGruntzSelection(0, 15);
                SetMaxGruntzSelection(1, 1);
                SetMaxGruntzSelection(2, 1);
                SetMaxGruntzSelection(3, 1);
                m_gameManager->m_players[0].m_maxGruntz = 15;
                m_gameManager->m_players[1].m_maxGruntz = 1;
                m_gameManager->m_players[2].m_maxGruntz = 1;
                m_gameManager->m_players[3].m_maxGruntz = 1;
                for (i = 1; i < 4; i++) {
                    SetPlayerTypeSelection(i, 1);
                    GetPlayerNameControl(i)->EnableWindow(true);
                    GetPlayerTypeControl(i)->EnableWindow(true);
                    GetPlayerColorControl(i)->EnableWindow(true);
                    GetMaxGruntzControl(i)->EnableWindow(true);
                }
            }
            m_customNameFlag = false;
        }
    } else {
        CWnd* comboChild = GetDlgItem(0x4ff)->GetWindow(GW_CHILD);
        if (comboChild == NULL) {
            return;
        }
        comboChild->GetWindowTextA(m_worldName);
        reg->Set("LastMap", m_worldName);
        reg->Set("CustomMap", m_customNameFlag);

        for (i = 0; i < 4; i++) {
            CWnd* edit = GetPlayerNameControl(i);
            if (edit != NULL) {
                CString name;
                edit->GetWindowTextA(name);
                m_gameManager->m_players[i].m_name = name;
            }
        }
        for (i = 0; i < 4; i++) {
            i32 selection =
                static_cast<i32>(GetPlayerTypeControl(i)->SendMessageA(CB_GETCURSEL, 0, 0));
            if (selection != 0) {
                m_gameManager->m_players[i].m_active = true;
                m_gameManager->m_players[i].m_difficulty =
                    static_cast<BattlezDifficulty>(selection - 1);
            } else {
                m_gameManager->m_players[i].m_active = false;
                m_gameManager->m_players[i].m_difficulty = BZDIFF_NORMAL;
            }
        }
        if (g_battlezResetOptions != false) {
            g_battlezResetOptions = false;
        }
        g_buteMgr.GetDword("Battlez", "DefaultMaxGruntz", 8);
        for (i = 0; i < 4; i++) {
            sprintf(key, "LastMaxGruntz%d", i);
            reg->Set(key, GetMaxGruntzSelection(i));
            sprintf(key, "LastDiff%d", i);
            if (m_gameManager->m_players[i].m_active != false) {
                reg->Set(key, IDX(m_gameManager->m_players[i].m_difficulty));
            } else {
                reg->Set(key, -1);
            }
            sprintf(key, "LastColour%d", i);
            reg->Set(key, IDX(g_gameReg->m_players[i].m_color));
        }
        NetLobby::g_curDlg = NULL;
    }
    PaintPlayerColorControls();
}

RVA(0x00015a10, 0x70)
i32 CALLBACK BattlezMapComboEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_SETTEXT) {

        MsgParam text;
        text.m_lparam = lParam;
        if (strcmp("", text.m_str) == 0) {
            return 0;
        }
    }
    return CallWindowProcA(g_savedDlgWndProc, hWnd, msg, wParam, lParam);
}

RVA(0x00015aa0, 0x6)
const AFX_MSGMAP* CBattlezDlg::GetMessageMap() const {
    return &messageMap;
}

RVA(0x00015ac0, 0x60)
CWnd* CBattlezDlg::GetPlayerTypeControl(i32 slot) {
    CWnd* result = NULL;
    switch (static_cast<PlayerSlot>(slot)) {
        case PLAYER_SLOT_0:
            result = GetDlgItem(CTRL_PLAYER_TYPE0);
            break;
        case PLAYER_SLOT_1:
            result = GetDlgItem(CTRL_PLAYER_TYPE1);
            break;
        case PLAYER_SLOT_2:
            result = GetDlgItem(CTRL_PLAYER_TYPE2);
            break;
        case PLAYER_SLOT_3:
            result = GetDlgItem(CTRL_PLAYER_TYPE3);
            break;
    }
    return result;
}

RVA(0x00015b40, 0x60)
CWnd* CBattlezDlg::GetPlayerNameControl(i32 slot) {
    CWnd* result = NULL;
    switch (static_cast<PlayerSlot>(slot)) {
        case PLAYER_SLOT_0:
            result = GetDlgItem(CTRL_PLAYER_NAME0);
            break;
        case PLAYER_SLOT_1:
            result = GetDlgItem(CTRL_PLAYER_NAME1);
            break;
        case PLAYER_SLOT_2:
            result = GetDlgItem(CTRL_PLAYER_NAME2);
            break;
        case PLAYER_SLOT_3:
            result = GetDlgItem(CTRL_PLAYER_NAME3);
            break;
    }
    return result;
}

RVA(0x00015bc0, 0x60)
CWnd* CBattlezDlg::GetMaxGruntzControl(i32 slot) {
    CWnd* result = NULL;
    switch (static_cast<PlayerSlot>(slot)) {
        case PLAYER_SLOT_0:
            result = GetDlgItem(CTRL_PLAYER_MAX_GRUNTZ0);
            break;
        case PLAYER_SLOT_1:
            result = GetDlgItem(CTRL_PLAYER_MAX_GRUNTZ1);
            break;
        case PLAYER_SLOT_2:
            result = GetDlgItem(CTRL_PLAYER_MAX_GRUNTZ2);
            break;
        case PLAYER_SLOT_3:
            result = GetDlgItem(CTRL_PLAYER_MAX_GRUNTZ3);
            break;
    }
    return result;
}

RVA(0x00015c40, 0x60)
CWnd* CBattlezDlg::GetPlayerColorControl(i32 slot) {
    CWnd* result = NULL;
    switch (static_cast<PlayerSlot>(slot)) {
        case PLAYER_SLOT_0:
            result = GetDlgItem(CTRL_PLAYER_COLOR0);
            break;
        case PLAYER_SLOT_1:
            result = GetDlgItem(CTRL_PLAYER_COLOR1);
            break;
        case PLAYER_SLOT_2:
            result = GetDlgItem(CTRL_PLAYER_COLOR2);
            break;
        case PLAYER_SLOT_3:
            result = GetDlgItem(CTRL_PLAYER_COLOR3);
            break;
    }
    return result;
}

RVA(0x00015cc0, 0x23)
i32 CBattlezDlg::SetPlayerTypeSelection(i32 slot, i32 selection) {
    CWnd* control = GetPlayerTypeControl(slot);
    return control->SendMessageA(CB_SETCURSEL, selection, 0);
}

RVA(0x00015d00, 0x20)
i32 CBattlezDlg::GetPlayerTypeSelection(i32 slot) {
    CWnd* control = GetPlayerTypeControl(slot);
    return control->SendMessageA(CB_GETCURSEL, 0, 0);
}

RVA(0x00015d30, 0x21)
i32 CBattlezDlg::GetMaxGruntzSelection(i32 slot) {
    CWnd* control = GetMaxGruntzControl(slot);
    return control->SendMessageA(CB_GETCURSEL, 0, 0) + 1;
}

RVA(0x00015d70, 0x24)
i32 CBattlezDlg::SetMaxGruntzSelection(i32 slot, i32 count) {
    CWnd* control = GetMaxGruntzControl(slot);
    return control->SendMessageA(CB_SETCURSEL, count - 1, 0);
}

RVA(0x00015db0, 0x19)
void CBattlezDlg::SetPlayerName(i32 slot, const char* name) {
    CWnd* control = GetPlayerNameControl(slot);
    control->SetWindowTextA(name);
}
RVA(0x00015de0, 0x5f)
void CBattlezDlg::OnPlayerTypeSelection0() {
    UpdatePlayerSlotEnabled(0);
    OnPlayerOptionsChanged();
    if (GetPlayerTypeSelection(1) || GetPlayerTypeSelection(2) || GetPlayerTypeSelection(3)) {
        GetDlgItem(1)->EnableWindow(true);
    } else {
        GetDlgItem(1)->EnableWindow(false);
    }
}

RVA(0x00015e60, 0x5f)
void CBattlezDlg::OnPlayerTypeSelection1() {
    UpdatePlayerSlotEnabled(1);
    OnPlayerOptionsChanged();
    if (GetPlayerTypeSelection(1) || GetPlayerTypeSelection(2) || GetPlayerTypeSelection(3)) {
        GetDlgItem(1)->EnableWindow(true);
    } else {
        GetDlgItem(1)->EnableWindow(false);
    }
}

RVA(0x00015ee0, 0x5f)
void CBattlezDlg::OnPlayerTypeSelection2() {
    UpdatePlayerSlotEnabled(2);
    OnPlayerOptionsChanged();
    if (GetPlayerTypeSelection(1) || GetPlayerTypeSelection(2) || GetPlayerTypeSelection(3)) {
        GetDlgItem(1)->EnableWindow(true);
    } else {
        GetDlgItem(1)->EnableWindow(false);
    }
}

RVA(0x00015f60, 0x5f)
void CBattlezDlg::OnPlayerTypeSelection3() {
    UpdatePlayerSlotEnabled(3);
    OnPlayerOptionsChanged();
    if (GetPlayerTypeSelection(1) || GetPlayerTypeSelection(2) || GetPlayerTypeSelection(3)) {
        GetDlgItem(1)->EnableWindow(true);
    } else {
        GetDlgItem(1)->EnableWindow(false);
    }
}

RVA(0x00015fe0, 0xbe)
void CBattlezDlg::UpdatePlayerSlotEnabled(i32 slot) {
    CWnd* typeControl = GetPlayerTypeControl(slot);
    CWnd* nameControl = GetPlayerNameControl(slot);
    CWnd* colorControl = GetPlayerColorControl(slot);
    CWnd* maxGruntzControl = GetMaxGruntzControl(slot);
    if (slot == 0) {
        return;
    }
    GruntzPlayer* player = &m_gameManager->m_players[slot];
    if (typeControl->SendMessageA(CB_GETCURSEL, 0, 0) != 0) {
        nameControl->EnableWindow(true);
        colorControl->EnableWindow(true);
        player->m_active = true;
        maxGruntzControl->EnableWindow(true);
        return;
    }
    nameControl->EnableWindow(false);
    colorControl->EnableWindow(false);
    player->m_active = false;
    maxGruntzControl->EnableWindow(false);
}

RVA(0x000160d0, 0xb)
i32 CBattlezDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    return 1;
}

RVA(0x000160f0, 0x245)
void CBattlezDlg::PaintPlayerColorControls() {
    CPaintDC dc(this);
    for (i32 i = 0; i < 4; i++) {
        CWnd* colorControl = GetPlayerColorControl(i);
        if (colorControl == NULL) {
            continue;
        }

        CRect rect;
        colorControl->GetClientRect(&rect);
        colorControl->ClientToScreen(&rect.TopLeft());
        colorControl->ClientToScreen(&rect.BottomRight());
        ScreenToClient(&rect.TopLeft());
        ScreenToClient(&rect.BottomRight());
        CBrush brush;
        if (colorControl->IsWindowEnabled()) {
            GetRandomNumber();
            GetRandomNumber();
            i32 shade = (GetRandomNumber() % 0xff) & 0xff;
            brush.Attach(CreateSolidBrush((shade << 8 | shade) << 8 | shade));
        } else {
            brush.Attach(CreateSolidBrush(0x808080));
        }
        rect.InflateRect(-2, -2);
        FillRect(dc.m_hDC, &rect, brush);
    }
}

RVA_COMPGEN(0x000163e0, 0x20, ??_GCObject@@UAEPAXI@Z)
RVA_COMPGEN(0x00016410, 0x7, ??1CObject@@UAE@XZ)
RVA_COMPGEN(0x00016430, 0x1e, ??_GCGdiObject@@UAEPAXI@Z)
RVA_COMPGEN(0x00016460, 0x46, ??1CGdiObject@@UAE@XZ)

RVA_COMPGEN(0x000164d0, 0x1e, ??_GCBrush@@UAEPAXI@Z)
RVA_COMPGEN(0x00016500, 0x46, ??1CBrush@@UAE@XZ)

RVA(0x00016570, 0x12)
void CBattlezDlg::OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis) {
    CWnd::OnMeasureItem(nIDCtl, lpmis);
}

RVA(0x000165a0, 0x5c0)
void CBattlezDlg::OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis) {
    COLORREF color;
    b32 shouldDraw = false;
    switch (nIDCtl) {
        case CTRL_PLAYER_COLOR0:
            if (GetPlayerColorControl(0)->IsWindowEnabled()) {
                switch (m_gameManager->m_players[0].m_color) {
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
            } else {
                color = 0xc8c8c8;
            }
            shouldDraw = true;
            break;
        case CTRL_PLAYER_COLOR1:
            if (GetPlayerColorControl(1)->IsWindowEnabled()) {
                switch (m_gameManager->m_players[1].m_color) {
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
            } else {
                color = 0xc8c8c8;
            }
            shouldDraw = true;
            break;
        case CTRL_PLAYER_COLOR2:
            if (GetPlayerColorControl(2)->IsWindowEnabled()) {
                switch (m_gameManager->m_players[2].m_color) {
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
            } else {
                color = 0xc8c8c8;
            }
            shouldDraw = true;
            break;
        case CTRL_PLAYER_COLOR3:
            if (GetPlayerColorControl(3)->IsWindowEnabled()) {
                switch (m_gameManager->m_players[3].m_color) {
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
            } else {
                color = 0xc8c8c8;
            }
            shouldDraw = true;
            break;
    }
    if (shouldDraw) {
        CDC dc;
        dc.Attach(lpdis->hDC);
        CBrush brush(color);
        FillRect(dc.m_hDC, &lpdis->rcItem, brush);
        dc.Detach();
    }
    CWnd::OnDrawItem(nIDCtl, lpdis);
}

RVA(0x00016cd0, 0x98)
void CBattlezDlg::OnPlayerColor0() {
    CBattlezDlgColors dlg(m_gameManager, 0, 0, NULL);
    if (dlg.DoModal() == 1) {
        if (SetPlayerColor(0, static_cast<ColorTint>(dlg.m_pickedColor))) {
            OnPlayerOptionsChanged();
            GetDlgItem(CTRL_PLAYER_COLOR0)->InvalidateRect(NULL, true);
        }
    }
}
RVA_COMPGEN(0x00016da0, 0x5, ??1CBattlezDlgColors@@UAE@XZ)

RVA(0x00016dc0, 0x97)
void CBattlezDlg::OnPlayerColor1() {
    CBattlezDlgColors dlg(m_gameManager, 1, 0, NULL);
    if (dlg.DoModal() == 1) {
        if (SetPlayerColor(1, static_cast<ColorTint>(dlg.m_pickedColor))) {
            OnPlayerOptionsChanged();
            GetDlgItem(CTRL_PLAYER_COLOR1)->InvalidateRect(NULL, true);
        }
    }
}

RVA(0x00016e90, 0x98)
void CBattlezDlg::OnPlayerColor2() {
    CBattlezDlgColors dlg(m_gameManager, 2, 0, NULL);
    if (dlg.DoModal() == 1) {
        if (SetPlayerColor(2, static_cast<ColorTint>(dlg.m_pickedColor))) {
            OnPlayerOptionsChanged();
            GetDlgItem(CTRL_PLAYER_COLOR2)->InvalidateRect(NULL, true);
        }
    }
}

RVA(0x00016f60, 0x98)
void CBattlezDlg::OnPlayerColor3() {
    CBattlezDlgColors dlg(m_gameManager, 3, 0, NULL);
    if (dlg.DoModal() == 1) {
        if (SetPlayerColor(3, static_cast<ColorTint>(dlg.m_pickedColor))) {
            OnPlayerOptionsChanged();
            GetDlgItem(CTRL_PLAYER_COLOR3)->InvalidateRect(NULL, true);
        }
    }
}

RVA(0x00017030, 0xc1)
void CBattlezDlg::ShowCustomDlg() {
    CBattlezDlgCustom dlg(NULL);
    if (dlg.DoModal() == 1) {
        if (dlg.m_customName.GetLength() != 0) {
            dlg.m_customName.MakeUpper();
            CWnd* item = GetDlgItem(0x4ff);
            CWnd* child = item->GetWindow(GW_CHILD);
            if (child == NULL) {
                return;
            }
            child->SetWindowTextA(dlg.m_customName);
            m_customNameFlag = true;
        }
    }
}
RVA_COMPGEN(0x00017140, 0x47, ??1CBattlezDlgCustom@@UAE@XZ)

RVA(0x000171b0, 0xca)
void CBattlezDlg::OnWorldSelectionChange() {
    CWnd* combo = GetDlgItem(0x4ff);
    if (combo == NULL) {
        return;
    }
    long selection = combo->SendMessageA(CB_GETCURSEL, 0, 0);
    if (selection == -1) {
        return;
    }
    CString worldName;
    (static_cast<CComboBox*>(combo))->GetLBText(selection, worldName);
    if (worldName.GetLength() != 0) {
        CWnd* owner = GetDlgItem(0x4ff);
        CWnd* child = owner->GetWindow(GW_CHILD);
        if (child != NULL) {
            child->SetWindowTextA(worldName);
            m_customNameFlag = false;
        }
    }
}

RVA(0x000172c0, 0x8)
void CBattlezDlg::OnPlayerNameKillFocus0() {
    ReadPlayerName(0);
}
RVA(0x000172e0, 0x8)
void CBattlezDlg::OnPlayerNameKillFocus1() {
    ReadPlayerName(1);
}
RVA(0x00017300, 0x8)
void CBattlezDlg::OnPlayerNameKillFocus2() {
    ReadPlayerName(2);
}
RVA(0x00017320, 0x8)
void CBattlezDlg::OnPlayerNameKillFocus3() {
    ReadPlayerName(3);
}

RVA(0x00017340, 0x73)
void CBattlezDlg::ReadPlayerName(i32 slot) {
    CString name;
    GetPlayerNameControl(slot)->GetWindowText(name);
    if (strlen(name) == 0) {
        return;
    }
}

RVA(0x000173e0, 0x1)
void CBattlezDlg::OnPlayerOptionsChanged() {}

// @identity-TODO BattlezNoOp - Dialogs.cpp ownership follows from the adjacent
// same-file claims and its incremental-link thunk. No caller, address-taker, or
// operand survives to prove whether the original external symbol was a member.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00017400, 0x1)
void BattlezNoOp() {}

// @identity-TODO BattlezNoOpArg - same evidence as BattlezNoOp. `ret 4` proves
// one callee-popped dword, but not the original semantic name or receiver type.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00017420, 0x3)
void __stdcall BattlezNoOpArg(i32) {}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00017440, 0x3)
i32 CBattlezDlg::UnusedMsgHandler() {
    return 0;
}

RVA(0x00017460, 0x22)
i32 CBattlezDlg::SetPlayerColor(i32 slot, ColorTint color) {
    m_gameManager->m_players[slot].m_color = color;
    return 1;
}

RVA(0x000174a0, 0x5)
void CBattlezDlg::OnOK() {
    CDialog::OnOK();
}

RVA(0x000174c0, 0x8)
void CBattlezDlg::OnPlayerNameChange0() {
    HandlePlayerNameChange(0);
}
RVA(0x000174e0, 0x8)
void CBattlezDlg::OnPlayerNameChange1() {
    HandlePlayerNameChange(1);
}
RVA(0x00017500, 0x8)
void CBattlezDlg::OnPlayerNameChange2() {
    HandlePlayerNameChange(2);
}
RVA(0x00017520, 0x8)
void CBattlezDlg::OnPlayerNameChange3() {
    HandlePlayerNameChange(3);
}
RVA(0x00017540, 0x3)
void CBattlezDlg::HandlePlayerNameChange(i32) {}

RVA(0x00017560, 0x28)
i32 CBattlezDlg::OnMaxGruntzSelection0() {
    CWnd* control = GetMaxGruntzControl(0);
    i32 count = control->SendMessageA(CB_GETCURSEL, 0, 0) + 1;
    g_gameReg->m_players[0].m_maxGruntz = count;
    return count;
}
RVA(0x000175a0, 0x28)
i32 CBattlezDlg::OnMaxGruntzSelection1() {
    CWnd* control = GetMaxGruntzControl(1);
    i32 count = control->SendMessageA(CB_GETCURSEL, 0, 0) + 1;
    g_gameReg->m_players[1].m_maxGruntz = count;
    return count;
}
RVA(0x000175e0, 0x28)
i32 CBattlezDlg::OnMaxGruntzSelection2() {
    CWnd* control = GetMaxGruntzControl(2);
    i32 count = control->SendMessageA(CB_GETCURSEL, 0, 0) + 1;
    g_gameReg->m_players[2].m_maxGruntz = count;
    return count;
}
RVA(0x00017620, 0x28)
i32 CBattlezDlg::OnMaxGruntzSelection3() {
    CWnd* control = GetMaxGruntzControl(3);
    i32 count = control->SendMessageA(CB_GETCURSEL, 0, 0) + 1;
    g_gameReg->m_players[3].m_maxGruntz = count;
    return count;
}
