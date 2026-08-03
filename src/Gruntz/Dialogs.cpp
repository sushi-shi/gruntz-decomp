#include <rva.h>

#include <Gruntz/Dialogs.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <EmptyString.h>
#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/Random.h>
#include <MsgParam.h>
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
    ON_BN_CLICKED(0x501, CBattlezDlg::ApplyColorSlot0) ON_BN_CLICKED(
        0x503,
        CBattlezDlg::ApplyColorSlot1
    ) ON_BN_CLICKED(0x505, CBattlezDlg::ApplyColorSlot2)
        ON_BN_CLICKED(0x507, CBattlezDlg::ApplyColorSlot3) ON_BN_CLICKED(
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

DATA(0x001e8d10)
const i32 g_msgmap_CBattlezDlgColors = 6205544;

VTBL(CBattlezDlg, 0x001e8bac);
VTBL(CBattlezDlgColors, 0x001e8d94);
VTBL(CBattlezDlgCustom, 0x001e8ee4);
DATA(0x00229d10)
WNDPROC g_savedDlgWndProc;
DATA(0x00229c50)
i32 g_battlezLastColors[4];
DATA(0x00229cf0)
i32 g_battlezLastDifficulties[4];
DATA(0x00229d00)
i32 g_battlezLastMaxGruntz[4];
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

RVA_COMPGEN(0x00014c60, 0x1e, ??_GCBattlezDlg@@UAEPAXI@Z)
RVA_COMPGEN(0x00014c90, 0x47, ??1CBattlezDlg@@UAE@XZ)

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
            g_battlezLastColors[i] = reg->GetValueDword(key, g_gameReg->m_options[i].m_colorIndex);
            g_gameReg->m_options[i].m_colorIndex = g_battlezLastColors[i];
        }

        CWnd* combo = GetDlgItem(0x4ff);
        CWnd* comboChild = CWnd::FromHandle(::GetWindow(combo->m_hWnd, GW_CHILD));
        if (comboChild == 0) {
            return;
        }
        ::SendMessageA(comboChild->m_hWnd, EM_SETREADONLY, 1, 0);
        comboChild->SetWindowTextA(g_emptyString);

        CSymTab* worlds = static_cast<CSymTab*>(m_slots->m_symParser->ResolvePath("GAME_BATTLEZ"));
        if (worlds == 0) {
            return;
        }
        CSymRec* record = static_cast<CSymRec*>(worlds->FirstSym());
        CParseSource* entry =
            record == 0 ? 0 : static_cast<CParseSource*>(worlds->NextSym2(record));
        i32 first = 1;
        while (entry != 0) {
            CString upper(entry->m_name);
            upper.MakeUpper();
            CString display;
            for (i = 0; i < upper.GetLength(); i++) {
                char c = upper[i];
                if (c == '.') {
                    break;
                }
                display += c;
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
        g_savedDlgWndProc =
            reinterpret_cast<WNDPROC>(GetWindowLongA(comboChild->m_hWnd, GWL_WNDPROC));
        SetWindowLongA(
            comboChild->m_hWnd,
            GWL_WNDPROC,
            reinterpret_cast<LONG>(&BattlezMapComboEditProc) // API-forced Win32 callback seam.
        );

        GetDlgItem(0x512)->SetWindowTextA("Battlez Setup");
        g_sharedFlag = m_hWnd;

        for (i = 0; i < 4; i++) {
            CWnd* ctrl = GetCtrlA(i);
            if (i == 0) {
                ::SendMessageA(ctrl->m_hWnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Human"));
            } else {
                ::SendMessageA(ctrl->m_hWnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("None"));
                ::SendMessageA(
                    ctrl->m_hWnd,
                    CB_ADDSTRING,
                    0,
                    reinterpret_cast<LPARAM>("Computer (easy)")
                );
                ::SendMessageA(
                    ctrl->m_hWnd,
                    CB_ADDSTRING,
                    0,
                    reinterpret_cast<LPARAM>("Computer (normal)")
                );
                ::SendMessageA(
                    ctrl->m_hWnd,
                    CB_ADDSTRING,
                    0,
                    reinterpret_cast<LPARAM>("Computer (difficult)")
                );
            }
        }
        SetCurSelA(0, 0);
        SetCurSelA(1, 2);
        SetCurSelA(2, 2);
        SetCurSelA(3, 2);

        if (g_battlezResetOptions == 0) {
            for (i = 1; i < 4; i++) {
                i32 difficulty = g_battlezLastDifficulties[i];
                if (difficulty == -1) {
                    SetCurSelA(i, 0);
                } else {
                    SetCurSelA(i, difficulty + 1);
                    m_slots->m_options[i].m_configId = difficulty;
                }
            }
        } else {
            m_slots->m_options[1].m_humanControlled = 1;
            m_slots->m_options[2].m_humanControlled = 1;
            m_slots->m_options[3].m_humanControlled = 1;
        }

        SetCtrlBText(0, "Player");
        SetCtrlBText(1, g_emptyString);
        SetCtrlBText(2, "Serra");
        SetCtrlBText(3, "Jebediah");
        for (i = 0; i < 4; i++) {
            SetCurSelC(i, defaultMax);
        }
        for (i = 0; i < 4; i++) {
            if (g_battlezResetOptions == 0) {
                SetCurSelC(i, g_battlezLastMaxGruntz[i]);
                m_slots->m_options[i].m_comboSel = g_battlezLastMaxGruntz[i];
            }
            m_slots->m_options[i].m_liveGate = 1;
        }
        for (i = 0; i < 4; i++) {
            CWnd* edit = GetCtrlB(i);
            if (edit != 0) {
                ::SendMessageA(edit->m_hWnd, EM_LIMITTEXT, 9, 0);
            }
        }

        combo->EnableWindow(1);
        GetDlgItem(0x42b)->EnableWindow(1);
        GetDlgItem(IDOK)->EnableWindow(1);
        GetDlgItem(IDCANCEL)->EnableWindow(1);
        for (i = 0; i < 4; i++) {
            CWnd* ctrlA = GetCtrlA(i);
            CWnd* ctrlB = GetCtrlB(i);
            CWnd* ctrlC = GetCtrlC(i);
            CWnd* ctrlD = GetCtrlD(i);
            ctrlB->EnableWindow(1);
            ::SendMessageA(ctrlB->m_hWnd, EM_SETREADONLY, 0, 0);
            ctrlC->EnableWindow(1);
            ctrlA->EnableWindow(1);
            ctrlD->EnableWindow(1);
            if (GetPlayerTypeSelection(i) == 0) {
                ctrlB->EnableWindow(0);
                if (i != 0) {
                    ctrlC->EnableWindow(0);
                    ctrlD->EnableWindow(0);
                }
            }
        }
        GetDlgItem(IDOK)->EnableWindow(
            GetPlayerTypeSelection(1) || GetPlayerTypeSelection(2) || GetPlayerTypeSelection(3)
        );

        i32 customMap = reg->GetValueDword("CustomMap", 2);
        if (customMap == 2) {
            CString mapName;
            if (mapName.LoadStringA(0x81ab)) {
                comboChild->SetWindowTextA(mapName);
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
                    GetCtrlC(i)->EnableWindow(1);
                    GetCtrlD(i)->EnableWindow(1);
                }
            }
            m_customNameFlag = 0;
        } else {
            char mapName[0x100];
            DWORD size = sizeof(mapName);
            reg->GetValueString("LastMap", mapName, &size, g_emptyString);
            m_customNameFlag = customMap;
            if (customMap == 0) {
                comboChild->SetWindowTextA(mapName);
            } else {
                sprintf(key, "custom\\%s", mapName);
                FILE* file = fopen(key, "rb");
                if (file != 0) {
                    comboChild->SetWindowTextA(mapName);
                    fclose(file);
                }
            }
        }
    } else {
        CWnd* combo = GetDlgItem(0x4ff);
        CWnd* comboChild = CWnd::FromHandle(::GetWindow(combo->m_hWnd, GW_CHILD));
        if (comboChild == 0) {
            return;
        }
        comboChild->GetWindowTextA(m_worldName);
        reg->SetValueString("LastMap", m_worldName);
        reg->SetValueDword("CustomMap", m_customNameFlag);

        for (i = 0; i < 4; i++) {
            CWnd* edit = GetCtrlB(i);
            if (edit != 0) {
                CString name;
                edit->GetWindowTextA(name);
                m_slots->m_options[i].m_name = name;
            }
        }
        for (i = 0; i < 4; i++) {
            i32 selection = GetPlayerTypeSelection(i);
            if (selection == 0) {
                m_slots->m_options[i].m_liveGate = 0;
                m_slots->m_options[i].m_configId = 1;
            } else {
                m_slots->m_options[i].m_liveGate = 1;
                m_slots->m_options[i].m_configId = selection - 1;
            }
        }
        g_battlezResetOptions = 0;
        g_buteMgr.GetDwordDef("Battlez", "DefaultMaxGruntz", 8);
        for (i = 0; i < 4; i++) {
            sprintf(key, "LastMaxGruntz%d", i);
            reg->SetValueDword(key, GetMaxGruntzSelection(i));
            sprintf(key, "LastDiff%d", i);
            i32 difficulty =
                m_slots->m_options[i].m_liveGate == 0 ? -1 : m_slots->m_options[i].m_configId;
            reg->SetValueDword(key, difficulty);
            sprintf(key, "LastColour%d", i);
            reg->SetValueDword(key, g_gameReg->m_options[i].m_colorIndex);
        }
        g_sharedFlag = 0;
    }
    FlashCtrlD();
}

RVA_COMPGEN(0x000163e0, 0x20, ??_GCObject@@UAEPAXI@Z)
RVA_COMPGEN(0x00016410, 0x7, ??1CObject@@UAE@XZ)
RVA_COMPGEN(0x00016430, 0x1e, ??_GCGdiObject@@UAEPAXI@Z)
RVA_COMPGEN(0x00016460, 0x46, ??1CGdiObject@@UAE@XZ)

RVA_COMPGEN(0x000164d0, 0x1e, ??_GCBrush@@UAEPAXI@Z)
RVA_COMPGEN(0x00016500, 0x46, ??1CBrush@@UAE@XZ)
RVA_COMPGEN(0x00016da0, 0x5, ??1CBattlezDlgColors@@UAE@XZ)

RVA_COMPGEN(0x00017140, 0x47, ??1CBattlezDlgCustom@@UAE@XZ)
RVA_COMPGEN(0x00017980, 0x1e, ??_GCBattlezDlgColors@@UAEPAXI@Z)

RVA(0x00018030, 0x56)
CBattlezDlgCustom::CBattlezDlgCustom(CWnd* pParent) : CDialog(0xc3, pParent) {}

// @identity-TODO _BattlezMapComboEditProc@16 - thunk oracle: retail gave this NO incremental
// thunk, so it came from the static LIBRARY, while the rest of this TU
// (63 fns) was a link-line object. It belongs to another compiland.
RVA(0x00015a10, 0x70)
i32 CALLBACK BattlezMapComboEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_SETTEXT) {

        MsgParam text;
        text.m_lparam = lParam;
        if (strcmp(g_emptyString, text.m_str) == 0) {
            return 0;
        }
    }
    return CallWindowProcA(g_savedDlgWndProc, hWnd, msg, wParam, lParam);
}

RVA(0x00015aa0, 0x6)
const AFX_MSGMAP* CBattlezDlg::GetMessageMap() const {
    return &messageMap;
}

// @early-stop
RVA(0x00017030, 0xc1)
void CBattlezDlg::ShowCustomDlg() {
    CBattlezDlgCustom dlg(0);
    if (dlg.DoModal() == 1) {
        if (dlg.m_customName.GetLength() != 0) {
            dlg.m_customName.MakeUpper();
            CWnd* item = GetDlgItem(0x4ff);
            CWnd* child = CWnd::FromHandle(::GetWindow(item->m_hWnd, GW_CHILD));
            if (child != 0) {
                child->SetWindowTextA(dlg.m_customName);
                m_customNameFlag = 1;
            }
        }
    }
}

RVA(0x00017930, 0x3a)
CBattlezDlgColors::CBattlezDlgColors(CGruntzMgr* mgr, i32 slotIndex, i32 networked, CWnd* pParent)
    : CDialog(0xc2, pParent) {
    m_slots = mgr;
    m_slotIndex = slotIndex;
    m_pickedColor = 0;
    m_networked = networked;
}

RVA(0x000179b0, 0xcb)
void CBattlezDlgColors::DoDataExchange(CDataExchange* pDX) {
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM);
    if (pDX->m_bSaveAndValidate) {
        CWnd* lb = GetDlgItem(0x515);
        pSend = ::SendMessageA;
        long sel = pSend(lb->m_hWnd, 0x188, 0, 0);
        long data = pSend(lb->m_hWnd, 0x199, sel, 0);
        m_pickedColor = data;
        if (data >= 0x11) {
            m_pickedColor = 0x10;
        }
    } else {
        CWnd* lb = GetDlgItem(0x515);
        pSend = ::SendMessageA;
        for (i32 i = 0; i < 0x11; i++) {
            i32 avail = 1;
            GruntzPlayer* rec = m_slots->m_options;
            for (i32 j = 0; j < 4; j++) {
                if (rec->m_liveGate != 0 && rec->m_colorIndex == i) {
                    avail = 0;
                }
                rec++;
            }
            if (avail) {

                MsgParam name;
                name.m_str = "Color";
                long idx = pSend(lb->m_hWnd, 0x180, 0, name.m_lparam);
                pSend(lb->m_hWnd, 0x19a, idx, i);
            }
        }
        pSend(lb->m_hWnd, 0x186, 0, 0);
    }
}

RVA(0x00017ac0, 0x6)
const AFX_MSGMAP* CBattlezDlgColors::GetMessageMap() const {
    // API-forced MFC message-map representation seam.

    return reinterpret_cast<const AFX_MSGMAP*>(&g_msgmap_CBattlezDlgColors);
}

RVA(0x00017ae0, 0x20)
void CBattlezDlgColors::OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis) {
    lpmis->itemWidth = 0xc8;
    lpmis->itemHeight = 0x1e;
    CWnd::OnMeasureItem(nIDCtl, lpmis);
}

RVA(0x00015ac0, 0x60)
CWnd* CBattlezDlg::GetCtrlA(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItem(0x500);
            break;
        case 1:
            result = GetDlgItem(0x50e);
            break;
        case 2:
            result = GetDlgItem(0x50f);
            break;
        case 3:
            result = GetDlgItem(0x510);
            break;
    }
    return result;
}

RVA(0x00015b40, 0x60)
CWnd* CBattlezDlg::GetCtrlB(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItem(0x50a);
            break;
        case 1:
            result = GetDlgItem(0x50b);
            break;
        case 2:
            result = GetDlgItem(0x50c);
            break;
        case 3:
            result = GetDlgItem(0x50d);
            break;
    }
    return result;
}

RVA(0x00015bc0, 0x60)
CWnd* CBattlezDlg::GetCtrlC(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItem(0x51e);
            break;
        case 1:
            result = GetDlgItem(0x520);
            break;
        case 2:
            result = GetDlgItem(0x521);
            break;
        case 3:
            result = GetDlgItem(0x522);
            break;
    }
    return result;
}

RVA(0x00015c40, 0x60)
CWnd* CBattlezDlg::GetCtrlD(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItem(0x501);
            break;
        case 1:
            result = GetDlgItem(0x503);
            break;
        case 2:
            result = GetDlgItem(0x505);
            break;
        case 3:
            result = GetDlgItem(0x507);
            break;
    }
    return result;
}

RVA(0x00015cc0, 0x23)
i32 CBattlezDlg::SetCurSelA(i32 id, i32 sel) {
    CWnd* c = GetCtrlA(id);
    return ::SendMessageA(c->m_hWnd, 0x14e, sel, 0);
}

RVA(0x00015d00, 0x20)
i32 CBattlezDlg::GetPlayerTypeSelection(i32 slot) {
    CWnd* c = GetCtrlA(slot);
    return ::SendMessageA(c->m_hWnd, 0x147, 0, 0);
}

RVA(0x00015d30, 0x21)
i32 CBattlezDlg::GetMaxGruntzSelection(i32 id) {
    CWnd* c = GetCtrlC(id);
    return ::SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
}

RVA(0x00015d70, 0x24)
i32 CBattlezDlg::SetCurSelC(i32 id, i32 sel) {
    CWnd* c = GetCtrlC(id);
    return ::SendMessageA(c->m_hWnd, 0x14e, sel - 1, 0);
}

RVA(0x00017460, 0x22)
i32 CBattlezDlg::SetSlotValue(i32 index, i32 val) {
    m_slots->m_options[index].m_colorIndex = val;
    return 1;
}

RVA(0x00017560, 0x28)
i32 CBattlezDlg::SaveOptionCombo0() {
    CWnd* c = GetCtrlC(0);
    i32 v = ::SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
    g_gameReg->m_options[0].m_comboSel = v;
    return v;
}
RVA(0x000175a0, 0x28)
i32 CBattlezDlg::SaveOptionCombo1() {
    CWnd* c = GetCtrlC(1);
    i32 v = ::SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
    g_gameReg->m_options[1].m_comboSel = v;
    return v;
}
RVA(0x000175e0, 0x28)
i32 CBattlezDlg::SaveOptionCombo2() {
    CWnd* c = GetCtrlC(2);
    i32 v = ::SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
    g_gameReg->m_options[2].m_comboSel = v;
    return v;
}
RVA(0x00017620, 0x28)
i32 CBattlezDlg::SaveOptionCombo3() {
    CWnd* c = GetCtrlC(3);
    i32 v = ::SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
    g_gameReg->m_options[3].m_comboSel = v;
    return v;
}

RVA(0x00015db0, 0x19)
void CBattlezDlg::SetCtrlBText(i32 index, const char* text) {
    CWnd* w = GetCtrlB(index);
    w->SetWindowTextA(text);
}

RVA(0x000173e0, 0x1)
void CBattlezDlg::RefreshOptionState() {}
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

RVA(0x00016cd0, 0x98)
void CBattlezDlg::ApplyColorSlot0() {
    CBattlezDlgColors dlg(m_slots, 0, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(0, dlg.m_pickedColor)) {
            RefreshOptionState();
            GetDlgItem(0x501)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x00016dc0, 0x97)
void CBattlezDlg::ApplyColorSlot1() {
    CBattlezDlgColors dlg(m_slots, 1, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(1, dlg.m_pickedColor)) {
            RefreshOptionState();
            GetDlgItem(0x503)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x00016e90, 0x98)
void CBattlezDlg::ApplyColorSlot2() {
    CBattlezDlgColors dlg(m_slots, 2, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(2, dlg.m_pickedColor)) {
            RefreshOptionState();
            GetDlgItem(0x505)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x00016f60, 0x98)
void CBattlezDlg::ApplyColorSlot3() {
    CBattlezDlgColors dlg(m_slots, 3, 0, 0);
    if (dlg.DoModal() == 1) {
        if (SetSlotValue(3, dlg.m_pickedColor)) {
            RefreshOptionState();
            GetDlgItem(0x507)->InvalidateRect(0, 1);
        }
    }
}

// @early-stop
RVA(0x000171b0, 0xca)
void CBattlezDlg::CopyComboSelToChild() {
    CWnd* combo = GetDlgItem(0x4ff);
    if (combo == 0) {
        return;
    }
    long sel = ::SendMessageA(combo->m_hWnd, 0x147, 0, 0);
    if (sel == -1) {
        return;
    }
    CString s;
    (static_cast<CComboBox*>(combo))->GetLBText(sel, s);
    if (s.GetLength() != 0) {
        CWnd* child = CWnd::FromHandle(::GetWindow(GetDlgItem(0x4ff)->m_hWnd, 5));
        if (child != 0) {
            child->SetWindowTextA(s);
            m_customNameFlag = 0;
        }
    }
}

// @early-stop
RVA(0x00017340, 0x73)
void CBattlezDlg::ReadPlayerName(i32 index) {
    CString s;
    GetCtrlB(index)->GetWindowText(s);
}

RVA(0x000160d0, 0xb)
i32 CBattlezDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    return 1;
}

// @early-stop
RVA(0x000160f0, 0x245)
void CBattlezDlg::FlashCtrlD() {
    CPaintDC dc(this);
    BOOL(WINAPI * cts)(HWND, LPPOINT) = ::ClientToScreen;
    BOOL(WINAPI * stc)(HWND, LPPOINT) = ::ScreenToClient;
    for (i32 i = 0; i < 4; i++) {
        CWnd* it = GetCtrlD(i);
        if (it == 0) {
            continue;
        }

        CRect rc;
        ::GetClientRect(it->m_hWnd, &rc);
        cts(it->m_hWnd, &rc.TopLeft());
        cts(it->m_hWnd, &rc.BottomRight());
        stc(m_hWnd, &rc.TopLeft());
        stc(m_hWnd, &rc.BottomRight());
        CBrush scratch;
        i32 color;
        if (it->IsWindowEnabled()) {
            GameRand();
            GameRand();
            i32 v = (GameRand() % 0xff) & 0xff;
            color = (v << 8 | v) << 8 | v;
        } else {
            color = 0x808080;
        }
        scratch.Attach(CreateSolidBrush(color));
        rc.left += 2;
        rc.top += 2;
        rc.right -= 2;
        rc.bottom -= 2;
        FillRect(dc.m_hDC, &rc, scratch);
    }
}

RVA(0x00016570, 0x12)
void CBattlezDlg::OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis) {
    CWnd::OnMeasureItem(nIDCtl, lpmis);
}

RVA(0x000165a0, 0x5c0)
void CBattlezDlg::OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis) {
    COLORREF color;
    i32 bDraw = 0;
    switch (nIDCtl) {
        case 0x501:
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
        case 0x503:
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
        case 0x505:
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
        case 0x507:
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

RVA(0x00017440, 0x3)
i32 CBattlezDlg::UnusedMsgHandler() {
    return 0;
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

RVA(0x00017d40, 0x8)
RVA_COMPGEN(0x000180b0, 0x1e, ??_GCBattlezDlgCustom@@UAEPAXI@Z)
void CBattlezDlg::OnOkCommand() {
    OnOK();
}
