#include <rva.h>

#include <Gruntz/Dialogs.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CustomMapSelection.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Gruntz/ParseSource.h>
#include <MsgParam.h>
#include <Net/NetLobby.h>
#include <Utils/RegistryHelper.h>

#include <stdio.h>
#include <string.h>

DATA(0x001e88b0)
const AFX_MSGMAP CBattlezDlg::messageMap = {
    &CDialog::messageMap,
    &CBattlezDlg::_messageEntries[0],
};

DATA(0x001e88b8)
const AFX_MSGMAP_ENTRY CBattlezDlg::_messageEntries[] = {

    ON_CBN_SELCHANGE(0x500, CBattlezDlg::ApplyOption0) ON_CBN_SELCHANGE(
        0x50e,
        CBattlezDlg::ApplyOption1
    ) ON_CBN_SELCHANGE(0x50f, CBattlezDlg::ApplyOption2)
        ON_CBN_SELCHANGE(0x510, CBattlezDlg::ApplyOption3)

            {WM_MEASUREITEM,
             0,
             0,
             0,
             AfxSig_vOWNER,

             reinterpret_cast<AFX_PMSG>(&CBattlezDlg::OnMeasureItem)}, // API-forced MFC seam.
    {WM_DRAWITEM,
     0,
     0,
     0,
     AfxSig_vOWNER,
     reinterpret_cast<AFX_PMSG>(&CBattlezDlg::OnDrawItem)}, // API-forced MFC seam.
    ON_BN_CLICKED(CTRL_PLAYER_COLOR0, CBattlezDlg::ApplyColorSlot0) ON_BN_CLICKED(
        CTRL_PLAYER_COLOR1,
        CBattlezDlg::ApplyColorSlot1
    ) ON_BN_CLICKED(CTRL_PLAYER_COLOR2, CBattlezDlg::ApplyColorSlot2)
        ON_BN_CLICKED(CTRL_PLAYER_COLOR3, CBattlezDlg::ApplyColorSlot3) ON_BN_CLICKED(
            0x42b,
            CBattlezDlg::ShowCustomDlg
        ) ON_CBN_SELCHANGE(0x4ff, CBattlezDlg::CopyComboSelToChild)

            ON_CONTROL(0x200, 0x50a, CBattlezDlg::OnPlayerNameKillFocus0)
                ON_CONTROL(0x200, 0x50b, CBattlezDlg::OnPlayerNameKillFocus1)
                    ON_CONTROL(0x200, 0x50c, CBattlezDlg::OnPlayerNameKillFocus2)
                        ON_CONTROL(0x200, 0x50d, CBattlezDlg::OnPlayerNameKillFocus3)
                            ON_CONTROL(0x300, 0x50b, CBattlezDlg::OnPlayerNameChange1)
                                ON_CONTROL(0x300, 0x50a, CBattlezDlg::OnPlayerNameChange0)
                                    ON_CONTROL(0x300, 0x50c, CBattlezDlg::OnPlayerNameChange2)
                                        ON_CONTROL(0x300, 0x50d, CBattlezDlg::OnPlayerNameChange3)
    // API-forced MFC message-map representation seam.
    {WM_PAINT, 0, 0, 0, AfxSig_vv, reinterpret_cast<AFX_PMSG>(&CBattlezDlg::OnPaint)},
    ON_CBN_SELCHANGE(0x51e, CBattlezDlg::SaveOptionCombo0)
        ON_CBN_SELCHANGE(0x520, CBattlezDlg::SaveOptionCombo1)
            ON_CBN_SELCHANGE(0x521, CBattlezDlg::SaveOptionCombo2)
                ON_CBN_SELCHANGE(0x522, CBattlezDlg::SaveOptionCombo3){0, 0, 0, 0, AfxSig_end, 0},
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
i32 g_battlezResetOptions;

RVA(0x00014b10, 0x5)
long CBattlezDlg::OnPaint() {
    return Default();
}

RVA(0x00014b30, 0x64)
CBattlezDlg::CBattlezDlg(CGruntzMgr* mgr, CWnd* pParent) : CDialog(0xc0, pParent) {
    m_slots = mgr;
    m_customNameFlag = 0;
}

// The five MFC inline bodies cl emits as COMDATs here, and that this TU wins:
// `ret 4` / `ret` / `ret 4`, then EnableWindow(m_hWnd, FALSE/TRUE). Each is
// byte-identical to dialogs.obj's own COMDAT and sits inside this unit's band,
// so they are cl's output, not NAFXCW's.
RVA_COMPGEN(0x00014bc0, 0x3, ?Serialize@CObject@@UAEXAAVCArchive@@@Z)
RVA_COMPGEN(0x00014be0, 0x1, ?AssertValid@CObject@@UBEXXZ)
RVA_COMPGEN(0x00014c00, 0x3, ?Dump@CObject@@UBEXAAVCDumpContext@@@Z)
RVA_COMPGEN(0x00014c20, 0xd, ?BeginModalState@CWnd@@UAEXXZ)
RVA_COMPGEN(0x00014c40, 0xd, ?EndModalState@CWnd@@UAEXXZ)
RVA_COMPGEN(0x00014c60, 0x1e, ??_GCBattlezDlg@@UAEPAXI@Z)
RVA_COMPGEN(0x00014c90, 0x47, ??1CBattlezDlg@@UAE@XZ)

// @early-stop
// Instruction counts agree exactly; the residue is one `lea` where retail folds the
// slot stride with a destructive `add`, and the block sizes it shifts around it.
RVA(0x00014d00, 0xa68)
void CBattlezDlg::DoDataExchange(CDataExchange* pDX) {
    Utils::RegistryHelper* reg = g_gameReg->m_settings;
    char key[0x100];
    i32 i;

    if (!pDX->m_bSaveAndValidate) {
        i32 defaultMax = g_buteMgr.GetDwordDef("Battlez", "DefaultMaxGruntz", 8);
        for (i = 0; i < 4; i++) {
            sprintf(key, "LastMaxGruntz%d", i);
            g_battlezLastMaxGruntz[i] = reg->GetValueDword(key, defaultMax);
            sprintf(key, "LastDiff%d", i);
            g_battlezLastDifficulties[i] = reg->GetValueDword(key, 1);
            sprintf(key, "LastColour%d", i);
            g_battlezLastColors[i] =
                reg->GetValueDword(key, IDX(g_gameReg->m_options[i].m_colorIndex));
            g_gameReg->m_options[i].m_colorIndex = static_cast<ColorTint>(g_battlezLastColors[i]);
        }

        CWnd* comboChild = GetDlgItem(0x4ff)->GetWindow(GW_CHILD);
        if (comboChild == NULL) {
            return;
        }
        comboChild->SendMessageA(EM_SETREADONLY, 1, 0);
        comboChild->SetWindowTextA("");

        CWnd* combo = GetDlgItem(0x4ff);
        CSymTab* worlds = m_slots->m_symParser->ResolvePath("GAME_BATTLEZ");
        if (worlds == NULL) {
            return;
        }
        CParseSource* entry = static_cast<CParseSource*>(worlds->NextSym2(worlds->FirstSym()));
        i32 first = 1;
        while (entry != NULL) {
            CString upper(entry->m_name);
            upper.MakeUpper();
            CString display;
            // The extension is cut by carrying the last character into the loop
            // guard, so the '.' is seen twice: once to skip the append and once,
            // on the next turn, to leave.
            char c = 0;
            for (i = 0; i < upper.GetLength() && c != '.'; i++) {
                c = upper[i];
                if (c != '.') {
                    display += c;
                }
            }
            ::SendMessageA(
                combo->m_hWnd,
                CB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(static_cast<const char*>(display))
            );
            if (first != 0) {
                first = 0;
                comboChild->SetWindowTextA(display);
            }
            entry = static_cast<CParseSource*>(worlds->NextSym3(entry));
        }
        ::SendMessageA(combo->m_hWnd, CB_SETCURSEL, 0, 0);
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
                GetCtrlA(i)->SendMessageA(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("None"));
                GetCtrlA(i)
                    ->SendMessageA(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Computer (easy)"));
                GetCtrlA(i)
                    ->SendMessageA(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Computer (normal)"));
                GetCtrlA(i)->SendMessageA(
                    CB_ADDSTRING,
                    0,
                    reinterpret_cast<LPARAM>("Computer (difficult)")
                );
            } else {
                GetCtrlA(i)->SendMessageA(CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Human"));
            }
        }
        SetCurSelA(0, 0);
        SetCurSelA(1, 2);
        SetCurSelA(2, 2);
        SetCurSelA(3, 2);

        if (g_battlezResetOptions != 0) {
            m_slots->m_options[1].m_configId = 1;
            m_slots->m_options[2].m_configId = 1;
            m_slots->m_options[3].m_configId = 1;
        } else {
            for (i = 1; i < 4; i++) {
                if (g_battlezLastDifficulties[i] != -1) {
                    SetCurSelA(i, g_battlezLastDifficulties[i] + 1);
                    m_slots->m_options[i].m_configId = g_battlezLastDifficulties[i];
                } else {
                    SetCurSelA(i, 0);
                }
            }
        }

        SetCtrlBText(0, "Player");
        SetCtrlBText(1, "Zed");
        SetCtrlBText(2, "Serra");
        SetCtrlBText(3, "Jebediah");
        SetCurSelC(0, defaultMax);
        SetCurSelC(1, defaultMax);
        SetCurSelC(2, defaultMax);
        SetCurSelC(3, defaultMax);
        for (i = 0; i < 4; i++) {
            if (g_battlezResetOptions == 0) {
                SetCurSelC(i, g_battlezLastMaxGruntz[i]);
                m_slots->m_options[i].m_comboSel = g_battlezLastMaxGruntz[i];
            }
            GruntzPlayer* slot = &m_slots->m_options[i];
            if (slot != NULL) {
                slot->m_liveGate = 1;
            }
        }
        for (i = 0; i < 4; i++) {
            CWnd* edit = GetCtrlB(i);
            if (edit != NULL) {
                edit->SendMessageA(EM_LIMITTEXT, 9, 0);
            }
        }

        combo->EnableWindow(1);
        GetDlgItem(0x42b)->EnableWindow(1);
        GetDlgItem(IDOK)->EnableWindow(1);
        GetDlgItem(IDCANCEL)->EnableWindow(1);
        for (i = 0; i < 4; i++) {
            CWnd* ctrlA = GetCtrlA(i);
            CWnd* ctrlB = GetCtrlB(i);
            CWnd* ctrlD = GetCtrlD(i);
            CWnd* ctrlC = GetCtrlC(i);
            ctrlB->EnableWindow(1);
            ctrlB->SendMessageA(EM_SETREADONLY, 0, 0);
            ctrlD->EnableWindow(1);
            ctrlA->EnableWindow(1);
            ctrlC->EnableWindow(1);
            if (GetPlayerTypeSelection(i) == 0) {
                ctrlB->EnableWindow(0);
                if (i != 0) {
                    ctrlD->EnableWindow(0);
                }
                if (i != 0) {
                    ctrlC->EnableWindow(0);
                }
            }
        }
        if (GetPlayerTypeSelection(1) || GetPlayerTypeSelection(2) || GetPlayerTypeSelection(3)) {
            GetDlgItem(IDOK)->EnableWindow(1);
        } else {
            GetDlgItem(IDOK)->EnableWindow(0);
        }

        CustomMapSelection customMap = static_cast<CustomMapSelection>(
            reg->GetValueDword("CustomMap", IDX(CUSTOM_MAP_UNINITIALIZED))
        );
        if (customMap != CUSTOM_MAP_UNINITIALIZED) {
            char mapName[0x100];
            DWORD size = sizeof(mapName);
            reg->GetValueString("LastMap", mapName, &size, "");
            m_customNameFlag = IDX(customMap);
            if (customMap != CUSTOM_MAP_STANDARD) {
                sprintf(key, "custom\\%s", mapName);
                FILE* file = fopen(key, "rb");
                if (file != NULL) {
                    // Retail re-walks the combo down to its edit child here rather than
                    // reusing the one it took at the top of the branch.
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
                SetCurSelC(0, 15);
                SetCurSelC(1, 1);
                SetCurSelC(2, 1);
                SetCurSelC(3, 1);
                m_slots->m_options[0].m_comboSel = 15;
                m_slots->m_options[1].m_comboSel = 1;
                m_slots->m_options[2].m_comboSel = 1;
                m_slots->m_options[3].m_comboSel = 1;
                for (i = 1; i < 4; i++) {
                    SetCurSelA(i, 1);
                    GetCtrlB(i)->EnableWindow(1);
                    GetCtrlA(i)->EnableWindow(1);
                    GetCtrlD(i)->EnableWindow(1);
                    GetCtrlC(i)->EnableWindow(1);
                }
            }
            m_customNameFlag = 0;
        }
    } else {
        CWnd* comboChild = GetDlgItem(0x4ff)->GetWindow(GW_CHILD);
        if (comboChild == NULL) {
            return;
        }
        comboChild->GetWindowTextA(m_worldName);
        reg->SetValueString("LastMap", m_worldName);
        reg->SetValueDword("CustomMap", m_customNameFlag);

        for (i = 0; i < 4; i++) {
            CWnd* edit = GetCtrlB(i);
            if (edit != NULL) {
                CString name;
                edit->GetWindowTextA(name);
                m_slots->m_options[i].m_name = name;
            }
        }
        for (i = 0; i < 4; i++) {
            // Spelled out rather than through GetPlayerTypeSelection: retail
            // expands the combo query here and calls the accessor elsewhere.
            i32 selection = static_cast<i32>(GetCtrlA(i)->SendMessageA(CB_GETCURSEL, 0, 0));
            if (selection != 0) {
                m_slots->m_options[i].m_liveGate = 1;
                m_slots->m_options[i].m_configId = selection - 1;
            } else {
                m_slots->m_options[i].m_liveGate = 0;
                m_slots->m_options[i].m_configId = 1;
            }
        }
        if (g_battlezResetOptions != 0) {
            g_battlezResetOptions = 0;
        }
        g_buteMgr.GetDwordDef("Battlez", "DefaultMaxGruntz", 8);
        for (i = 0; i < 4; i++) {
            sprintf(key, "LastMaxGruntz%d", i);
            reg->SetValueDword(key, GetMaxGruntzSelection(i));
            sprintf(key, "LastDiff%d", i);
            if (m_slots->m_options[i].m_liveGate != 0) {
                reg->SetValueDword(key, m_slots->m_options[i].m_configId);
            } else {
                reg->SetValueDword(key, -1);
            }
            sprintf(key, "LastColour%d", i);
            reg->SetValueDword(key, IDX(g_gameReg->m_options[i].m_colorIndex));
        }
        NetLobby::g_curDlg = NULL;
    }
    FlashCtrlD();
}

// @identity-TODO _BattlezMapComboEditProc@16 - thunk oracle: retail gave this NO incremental
// thunk, so it came from the static LIBRARY, while the rest of this TU
// (63 fns) was a link-line object. It belongs to another compiland.
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
CWnd* CBattlezDlg::GetCtrlA(i32 index) {
    CWnd* result = 0;
    switch (static_cast<PlayerSlot>(index)) {
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
CWnd* CBattlezDlg::GetCtrlB(i32 index) {
    CWnd* result = 0;
    switch (static_cast<PlayerSlot>(index)) {
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
CWnd* CBattlezDlg::GetCtrlC(i32 index) {
    CWnd* result = 0;
    switch (static_cast<PlayerSlot>(index)) {
        case PLAYER_SLOT_0:
            result = GetDlgItem(CTRL_PLAYER_COMBO_C0);
            break;
        case PLAYER_SLOT_1:
            result = GetDlgItem(CTRL_PLAYER_COMBO_C1);
            break;
        case PLAYER_SLOT_2:
            result = GetDlgItem(CTRL_PLAYER_COMBO_C2);
            break;
        case PLAYER_SLOT_3:
            result = GetDlgItem(CTRL_PLAYER_COMBO_C3);
            break;
    }
    return result;
}

RVA(0x00015c40, 0x60)
CWnd* CBattlezDlg::GetCtrlD(i32 index) {
    CWnd* result = 0;
    switch (static_cast<PlayerSlot>(index)) {
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
i32 CBattlezDlg::SetCurSelA(i32 id, i32 sel) {
    CWnd* c = GetCtrlA(id);
    return ::SendMessageA(c->m_hWnd, CB_SETCURSEL, sel, 0);
}

RVA(0x00015d00, 0x20)
i32 CBattlezDlg::GetPlayerTypeSelection(i32 slot) {
    CWnd* c = GetCtrlA(slot);
    return ::SendMessageA(c->m_hWnd, CB_GETCURSEL, 0, 0);
}

RVA(0x00015d30, 0x21)
i32 CBattlezDlg::GetMaxGruntzSelection(i32 id) {
    CWnd* c = GetCtrlC(id);
    return ::SendMessageA(c->m_hWnd, CB_GETCURSEL, 0, 0) + 1;
}

RVA(0x00015d70, 0x24)
i32 CBattlezDlg::SetCurSelC(i32 id, i32 sel) {
    CWnd* c = GetCtrlC(id);
    return ::SendMessageA(c->m_hWnd, CB_SETCURSEL, sel - 1, 0);
}

RVA(0x00015db0, 0x19)
void CBattlezDlg::SetCtrlBText(i32 index, const char* text) {
    CWnd* w = GetCtrlB(index);
    w->SetWindowTextA(text);
}
RVA(0x00015de0, 0x5f)
void CBattlezDlg::ApplyOption0() {
    ToggleRow(0);
    RefreshOptionState();
    if (GetPlayerTypeSelection(1) || GetPlayerTypeSelection(2) || GetPlayerTypeSelection(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

RVA(0x00015e60, 0x5f)
void CBattlezDlg::ApplyOption1() {
    ToggleRow(1);
    RefreshOptionState();
    if (GetPlayerTypeSelection(1) || GetPlayerTypeSelection(2) || GetPlayerTypeSelection(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

RVA(0x00015ee0, 0x5f)
void CBattlezDlg::ApplyOption2() {
    ToggleRow(2);
    RefreshOptionState();
    if (GetPlayerTypeSelection(1) || GetPlayerTypeSelection(2) || GetPlayerTypeSelection(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

RVA(0x00015f60, 0x5f)
void CBattlezDlg::ApplyOption3() {
    ToggleRow(3);
    RefreshOptionState();
    if (GetPlayerTypeSelection(1) || GetPlayerTypeSelection(2) || GetPlayerTypeSelection(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

RVA(0x00015fe0, 0xbe)
void CBattlezDlg::ToggleRow(i32 row) {
    CWnd* a = GetCtrlA(row);
    CWnd* b = GetCtrlB(row);
    CWnd* d = GetCtrlD(row);
    CWnd* c = GetCtrlC(row);
    if (row == 0) {
        return;
    }
    GruntzPlayer* rec = &m_slots->m_options[row];
    if (::SendMessageA(a->m_hWnd, CB_GETCURSEL, 0, 0) != 0) {
        b->EnableWindow(1);
        d->EnableWindow(1);
        rec->m_liveGate = 1;
        c->EnableWindow(1);
        return;
    }
    b->EnableWindow(0);
    d->EnableWindow(0);
    rec->m_liveGate = 0;
    c->EnableWindow(0);
}

RVA(0x000160d0, 0xb)
i32 CBattlezDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    return 1;
}

RVA(0x000160f0, 0x245)
void CBattlezDlg::FlashCtrlD() {
    CPaintDC dc(this);
    BOOL(WINAPI * cts)(HWND, LPPOINT) = ::ClientToScreen;
    BOOL(WINAPI * stc)(HWND, LPPOINT) = ::ScreenToClient;
    for (i32 i = 0; i < 4; i++) {
        CWnd* it = GetCtrlD(i);
        if (it == NULL) {
            continue;
        }

        CRect rc;
        ::GetClientRect(it->m_hWnd, &rc);
        cts(it->m_hWnd, &rc.TopLeft());
        cts(it->m_hWnd, &rc.BottomRight());
        stc(m_hWnd, &rc.TopLeft());
        stc(m_hWnd, &rc.BottomRight());
        CBrush scratch;
        // Two Attach sites, not one hoisted `color`: retail pushes the argument
        // INSIDE each arm and cross-jumps only the shared `call CreateSolidBrush`
        // (`push eax / jmp` against `push 0x808080`). Hoisting the colour into a
        // local is what parks 0 in ebx and costs ScreenToClient its register.
        if (it->IsWindowEnabled()) {
            GetRandomNumber();
            GetRandomNumber();
            i32 v = (GetRandomNumber() % 0xff) & 0xff;
            scratch.Attach(CreateSolidBrush((v << 8 | v) << 8 | v));
        } else {
            scratch.Attach(CreateSolidBrush(0x808080));
        }
        rc.left += 2;
        rc.top += 2;
        rc.right -= 2;
        rc.bottom -= 2;
        FillRect(dc.m_hDC, &rc, scratch);
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
    i32 bDraw = 0;
    switch (nIDCtl) {
        case CTRL_PLAYER_COLOR0:
            if (GetCtrlD(0)->IsWindowEnabled()) {
                switch (m_slots->m_options[0].m_colorIndex) {
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
            bDraw = 1;
            break;
        case CTRL_PLAYER_COLOR1:
            if (GetCtrlD(1)->IsWindowEnabled()) {
                switch (m_slots->m_options[1].m_colorIndex) {
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
            bDraw = 1;
            break;
        case CTRL_PLAYER_COLOR2:
            if (GetCtrlD(2)->IsWindowEnabled()) {
                switch (m_slots->m_options[2].m_colorIndex) {
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
            bDraw = 1;
            break;
        case CTRL_PLAYER_COLOR3:
            if (GetCtrlD(3)->IsWindowEnabled()) {
                switch (m_slots->m_options[3].m_colorIndex) {
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
            bDraw = 1;
            break;
    }
    if (bDraw) {
        CDC dc;
        dc.Attach(lpdis->hDC);
        CBrush brush(color);
        FillRect(dc.m_hDC, &lpdis->rcItem, brush);
        dc.Detach();
    }
    CWnd::OnDrawItem(nIDCtl, lpdis);
}

RVA(0x00016cd0, 0x98)
void CBattlezDlg::ApplyColorSlot0() {
    CBattlezDlgColors dlg(m_slots, 0, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(0, static_cast<ColorTint>(dlg.m_pickedColor))) {
            RefreshOptionState();
            GetDlgItem(CTRL_PLAYER_COLOR0)->InvalidateRect(0, 1);
        }
    }
}
RVA_COMPGEN(0x00016da0, 0x5, ??1CBattlezDlgColors@@UAE@XZ)

RVA(0x00016dc0, 0x97)
void CBattlezDlg::ApplyColorSlot1() {
    CBattlezDlgColors dlg(m_slots, 1, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(1, static_cast<ColorTint>(dlg.m_pickedColor))) {
            RefreshOptionState();
            GetDlgItem(CTRL_PLAYER_COLOR1)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x00016e90, 0x98)
void CBattlezDlg::ApplyColorSlot2() {
    CBattlezDlgColors dlg(m_slots, 2, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(2, static_cast<ColorTint>(dlg.m_pickedColor))) {
            RefreshOptionState();
            GetDlgItem(CTRL_PLAYER_COLOR2)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x00016f60, 0x98)
void CBattlezDlg::ApplyColorSlot3() {
    CBattlezDlgColors dlg(m_slots, 3, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(3, static_cast<ColorTint>(dlg.m_pickedColor))) {
            RefreshOptionState();
            GetDlgItem(CTRL_PLAYER_COLOR3)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x00017030, 0xc1)
void CBattlezDlg::ShowCustomDlg() {
    CBattlezDlgCustom dlg(0);
    if (dlg.DoModal() == 1) {
        if (dlg.m_customName.GetLength() != 0) {
            dlg.m_customName.MakeUpper();
            CWnd* item = GetDlgItem(0x4ff);
            CWnd* child = CWnd::FromHandle(::GetWindow(item->m_hWnd, GW_CHILD));
            if (child == NULL) {
                return;
            }
            child->SetWindowTextA(dlg.m_customName);
            m_customNameFlag = 1;
        }
    }
}
RVA_COMPGEN(0x00017140, 0x47, ??1CBattlezDlgCustom@@UAE@XZ)

RVA(0x000171b0, 0xca)
void CBattlezDlg::CopyComboSelToChild() {
    CWnd* combo = GetDlgItem(0x4ff);
    if (combo == NULL) {
        return;
    }
    long sel = ::SendMessageA(combo->m_hWnd, CB_GETCURSEL, 0, 0);
    if (sel == -1) {
        return;
    }
    CString s;
    (static_cast<CComboBox*>(combo))->GetLBText(sel, s);
    if (s.GetLength() != 0) {
        HWND owner = GetDlgItem(0x4ff)->m_hWnd;
        CWnd* child = CWnd::FromHandle(::GetWindow(owner, GW_CHILD));
        if (child != NULL) {
            child->SetWindowTextA(s);
            m_customNameFlag = 0;
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
void CBattlezDlg::ReadPlayerName(i32 index) {
    CString s;
    GetCtrlB(index)->GetWindowText(s);
    if (strlen(s) == 0) {
        return;
    }
}

RVA(0x000173e0, 0x1)
void CBattlezDlg::RefreshOptionState() {}

RVA(0x00017440, 0x3)
i32 CBattlezDlg::UnusedMsgHandler() {
    return 0;
}

RVA(0x00017460, 0x22)
i32 CBattlezDlg::SetSlotValue(i32 index, ColorTint val) {
    m_slots->m_options[index].m_colorIndex = val;
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
i32 CBattlezDlg::SaveOptionCombo0() {
    CWnd* c = GetCtrlC(0);
    i32 v = ::SendMessageA(c->m_hWnd, CB_GETCURSEL, 0, 0) + 1;
    g_gameReg->m_options[0].m_comboSel = v;
    return v;
}
RVA(0x000175a0, 0x28)
i32 CBattlezDlg::SaveOptionCombo1() {
    CWnd* c = GetCtrlC(1);
    i32 v = ::SendMessageA(c->m_hWnd, CB_GETCURSEL, 0, 0) + 1;
    g_gameReg->m_options[1].m_comboSel = v;
    return v;
}
RVA(0x000175e0, 0x28)
i32 CBattlezDlg::SaveOptionCombo2() {
    CWnd* c = GetCtrlC(2);
    i32 v = ::SendMessageA(c->m_hWnd, CB_GETCURSEL, 0, 0) + 1;
    g_gameReg->m_options[2].m_comboSel = v;
    return v;
}
RVA(0x00017620, 0x28)
i32 CBattlezDlg::SaveOptionCombo3() {
    CWnd* c = GetCtrlC(3);
    i32 v = ::SendMessageA(c->m_hWnd, CB_GETCURSEL, 0, 0) + 1;
    g_gameReg->m_options[3].m_comboSel = v;
    return v;
}
