#include <rva.h>

#include <Gruntz/MultiStartDlg.h>

#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <Enums.h>
#include <Gruntz/CustomMapSelection.h>
#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MpSymItem.h>
#include <Gruntz/Multi.h>
#include <MsgParam.h>
#include <Net/InterfaceObject.h>
#include <Net/LatencyList.h>
#include <Net/NetMgr.h>
#include <Utils/RegistryHelper.h>

#include <stdio.h>
#include <string.h>

enum {
    NUM_PLAYER_SLOTS = 4
};

DATA(0x001ea578)
const AFX_MSGMAP CMultiStartDlg::messageMap = {
    &CDialog::messageMap,
    &CMultiStartDlg::_messageEntries[0],
};

DATA(0x001ea580)
const AFX_MSGMAP_ENTRY CMultiStartDlg::_messageEntries[] = {

    ON_CBN_SELCHANGE(0x500, CMultiStartDlg::ReconcileChannel0)
        ON_CBN_SELCHANGE(0x50e, CMultiStartDlg::ConnectStep)
            ON_CBN_SELCHANGE(0x50f, CMultiStartDlg::ReconcileChannel2)
                ON_CBN_SELCHANGE(0x510, CMultiStartDlg::ReconcileChannel3)

                    {WM_TIMER,
                     0,
                     0,
                     0,
                     AfxSig_vw,
                     reinterpret_cast<AFX_PMSG>(&CMultiStartDlg::OnTimer)}, // API-forced MFC seam.
    {WM_MEASUREITEM,
     0,
     0,
     0,
     AfxSig_vOWNER,
     reinterpret_cast<AFX_PMSG>(&CMultiStartDlg::OnMeasureItem)}, // API-forced MFC seam.
    {WM_DRAWITEM,
     0,
     0,
     0,
     AfxSig_vOWNER,
     reinterpret_cast<AFX_PMSG>(&CMultiStartDlg::OnDrawItem)}, // API-forced MFC seam.
    ON_BN_CLICKED(0x501, CMultiStartDlg::OnColorSlot0) ON_BN_CLICKED(
        0x503,
        CMultiStartDlg::OnColorSlot1
    ) ON_BN_CLICKED(0x505, CMultiStartDlg::OnColorSlot2)
        ON_BN_CLICKED(0x507, CMultiStartDlg::OnColorSlot3) ON_BN_CLICKED(
            0x42b,
            CMultiStartDlg::OnCustomWorld
        ) ON_CBN_SELCHANGE(0x4ff, CMultiStartDlg::CommitWorldHost)
            ON_BN_CLICKED(0x4c6, CMultiStartDlg::OnChatSend) ON_EN_CHANGE(
                0x50b,
                CMultiStartDlg::OnEnChange50b
            ) ON_EN_CHANGE(0x50a, CMultiStartDlg::OnEnChange50a)
                ON_EN_CHANGE(0x50c, CMultiStartDlg::OnEnChange50c) ON_EN_CHANGE(
                    0x50d,
                    CMultiStartDlg::OnEnChange50d
                ) ON_CBN_SELCHANGE(0x51e, CMultiStartDlg::OnSlotSelect0)
                    ON_CBN_SELCHANGE(0x520, CMultiStartDlg::OnSlotSelect1)
                        ON_CBN_SELCHANGE(0x521, CMultiStartDlg::OnSlotSelect2)
                            ON_CBN_SELCHANGE(0x522, CMultiStartDlg::OnSlotSelect3)
                                ON_CBN_SELCHANGE(0x527, CMultiStartDlg::CommitLatencyOption)
                                    ON_BN_CLICKED(0x51f, CMultiStartDlg::OnCmd51f)
                                        ON_BN_CLICKED(0x523, CMultiStartDlg::OnCmd523)
                                            ON_BN_CLICKED(0x524, CMultiStartDlg::OnCmd524)
                                                ON_BN_CLICKED(0x525, CMultiStartDlg::OnCmd525)
                                                    ON_BN_CLICKED(
                                                        0x4e9,
                                                        CMultiStartDlg::EchoLatencySettings
                                                    )
                                                        ON_CBN_SELCHANGE(
                                                            0x4ff,
                                                            CMultiStartDlg::CommitWorldHost
                                                        ){0, 0, 0, 0, AfxSig_end, 0},
};

RVA(0x000c1750, 0x88)
CMultiStartDlg::CMultiStartDlg(CGruntzMgr* mgr, CWnd* pParent)
    : CDialog(0xc5, pParent), m_reserved74(0xa) {
    m_host = mgr;
    m_customWorldFlag = 0;
    m_slotList = NULL;
    g_multiState = static_cast<CMulti*>(g_gameReg->m_curState);
}

RVA_COMPGEN(0x000c1810, 0x1e, ??_GCMultiStartDlg@@UAEPAXI@Z)
RVA(0x000c1840, 0x16e)
i32 CMultiStartDlg::SetupWorldCombo() {
    CWnd* combo = GetDlgItem(0x4ff);
    if (combo == NULL) {
        return 0;
    }
    CSymTab* st = static_cast<CSymTab*>(m_host->m_symParser->ResolvePath("GAME_MULTI"));
    if (st == NULL) {
        return 0;
    }
    MpSymItem* item = static_cast<MpSymItem*>(st->NextSym2(st->FirstSym()));
    while (item != NULL) {
        CString name(item->m_name);
        name.MakeUpper();
        MsgParam text;
        ::SendMessageA(
            combo->m_hWnd,
            CB_ADDSTRING,
            0,
            (text.m_str = static_cast<LPCTSTR>(name), text.m_lparam)
        );
        item = static_cast<MpSymItem*>(st->NextSym3(item));
    }
    CWnd* combo2 = GetDlgItem(0x4ff);
    CWnd* child = CWnd::FromHandle(::GetWindow(combo2->m_hWnd, GW_CHILD));
    if (child == NULL) {
        return 0;
    }
    ::SendMessageA(child->m_hWnd, EM_SETREADONLY, 1, 0);
    ::SendMessageA(combo->m_hWnd, CB_SETCURSEL, 0, 0);
    HWND__* h = child->m_hWnd;
    g_savedMultiWndProc = GetWindowLongA(h, GWL_WNDPROC);

    MsgParam proc;
    proc.m_intProc = MultiMapComboEditProc;
    SetWindowLongA(h, GWL_WNDPROC, proc.m_long);
    CommitWorldHost();
    return 1;
}

// @identity-TODO _MultiMapComboEditProc@16 - thunk oracle: retail gave this NO incremental
// thunk, so it came from the static LIBRARY, while the rest of this TU
// (16 fns) was a link-line object. It belongs to another compiland.
RVA(0x000c1a10, 0x70)
i32 CALLBACK MultiMapComboEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_SETTEXT) {

        MsgParam text;
        text.m_lparam = lParam;
        if (strcmp(g_emptyString, text.m_str) == 0) {
            return 0;
        }
    }

    MsgParam prev;
    return CallWindowProcA(
        (prev.m_long = g_savedMultiWndProc, prev.m_wndproc),
        hWnd,
        msg,
        wParam,
        lParam
    );
}

// @early-stop
RVA(0x000c1aa0, 0x2f8)
i32 CMultiStartDlg::UpdateColorItems() {
    if (g_multiState->m_isHost != 0) {
        CWnd* it4ff = GetDlgItem(0x4ff);
        CWnd* itChild = CWnd::FromHandle(::GetWindow(GetDlgItem(0x4ff)->m_hWnd, 5));
        CWnd* it42b = GetDlgItem(0x42b);
        CWnd* it4e9 = GetDlgItem(0x4e9);
        if (!itChild) {
            return 0;
        }
        if (!it4ff) {
            return 0;
        }
        if (!it42b) {
            return 0;
        }
        if (!it4e9) {
            return 0;
        }
        i32 idx = GetSlotIndex();
        i32 en = (m_host->m_options[idx].m_readyFlag == 0);
        it4ff->EnableWindow(en);
        it42b->EnableWindow(en);
        it4e9->EnableWindow(0);
        return 1;
    }
    CWnd* it4ff = GetDlgItem(0x4ff);
    CWnd* itChild = CWnd::FromHandle(::GetWindow(GetDlgItem(0x4ff)->m_hWnd, 5));
    CWnd* it42b = GetDlgItem(0x42b);
    CWnd* it4e9 = GetDlgItem(0x4e9);
    if (!itChild) {
        return 0;
    }
    if (!it4ff) {
        return 0;
    }
    if (!it42b) {
        return 0;
    }
    if (!it4e9) {
        return 0;
    }
    ::SendMessageA(it4ff->m_hWnd, CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    m_customWorldFlag = g_multiState->m_customLevel;
    if (m_customWorldFlag != 0) {
        itChild->SetWindowTextA(g_multiState->GetConfigNameB());
    } else {
        CString cur;
        itChild->GetWindowTextA(cur);
        if (strcmp(cur, g_multiState->GetConfigNameA())) {
            itChild->SetWindowTextA(g_multiState->GetConfigNameA());
        }
    }
    it4ff->EnableWindow(0);
    it42b->EnableWindow(0);
    it4e9->EnableWindow(0);
    return 1;
}

RVA(0x000c1e60, 0x115)
i32 CMultiStartDlg::BuildSlotList() {
    m_slotList = new CLatencyList(0xa);
    CMulti* reg = g_multiState;
    i32 count = 5;
    InterfaceObject* pi = reg->m_netGate->m_groupSel;
    if (reg->m_lobbyLaunch) {
        count = 2;
    } else if (pi) {
        if (pi->IsIpxProvider()) {
            count = 1;
        }
        if (pi->IsTcpIpProvider()) {
            count = 2;
        }
        if (pi->IsModemProvider()) {
            count = 3;
        }
        if (pi->IsSerialProvider()) {
            count = 4;
        }
    }
    m_slotList->Dispatch(count);
    HWND v = GetSafe1c();
    m_slotList->FillCombo(v, 0x527);
    m_slotList->SelectItem(v, 0x527, 0, 0);
    g_multiState->m_autoCommandDelay = 1;
    return 1;
}

// @early-stop
RVA(0x000c1fd0, 0x99)
i32 CMultiStartDlg::UpdateSlot() {
    CWnd* w = GetDlgItem(0x527);
    if (w == NULL) {
        return 0;
    }
    CMulti* reg = g_multiState;
    i32 enable;
    if (reg->m_isHost) {
        i32 idx = GetSlotIndex();
        enable = (m_host->m_options[idx].m_readyFlag == 0);
    } else {
        enable = 0;
    }
    w->EnableWindow(enable);
    HWND v = GetSafe1c();
    CMulti* reg2 = g_multiState;
    if (reg2->m_autoCommandDelay) {
        m_slotList->SelectItem(v, 0x527, 0, 0);
    } else {
        m_slotList->SelectItem(v, 0x527, reg2->m_commandDelay, reg2->m_drainReload);
    }
    return 1;
}

// @early-stop
RVA(0x000c20a0, 0x45a)
void CMultiStartDlg::DoDataExchange(CDataExchange* pDX) {
    Utils::RegistryHelper* reg = static_cast<Utils::RegistryHelper*>(g_gameReg->m_settings);
    if (pDX->m_bSaveAndValidate == 0) {
        GetDlgItem(0x512)->SetWindowTextA(g_multiState->GetString59c());
        NetLobby::g_curDlg = GetSafe1c();
        if (!SetupWorldCombo()) {
            return;
        }
        if (!BuildSlotList()) {
            return;
        }
        WapSendMessageA pSend = ::SendMessageA;
        i32 i;

        MsgParam item;
        for (i = 0; i < NUM_PLAYER_SLOTS; i++) {
            HWND kc;
            kc = GetCtrlE(i)->m_hWnd;
            item.m_str = "None";
            pSend(kc, CB_ADDSTRING, 0, item.m_lparam);
            kc = GetCtrlE(i)->m_hWnd;
            item.m_str = "Computer (easy)";
            pSend(kc, CB_ADDSTRING, 0, item.m_lparam);
            kc = GetCtrlE(i)->m_hWnd;
            item.m_str = "Computer (normal)";
            pSend(kc, CB_ADDSTRING, 0, item.m_lparam);
            kc = GetCtrlE(i)->m_hWnd;
            item.m_str = "Computer (difficult)";
            pSend(kc, CB_ADDSTRING, 0, item.m_lparam);
            kc = GetCtrlE(i)->m_hWnd;
            item.m_str = "Human";
            pSend(kc, CB_ADDSTRING, 0, item.m_lparam);
        }
        for (i = 0; i < NUM_PLAYER_SLOTS; i++) {
            CWnd* e = GetCtrlB(i);
            if (e != NULL) {
                pSend(e->m_hWnd, EM_LIMITTEXT, 9, 0);
            }
        }
        HWND chatEdit = GetDlgItem(0x42d)->m_hWnd;
        pSend(chatEdit, EM_LIMITTEXT, 100, 0);
        CustomMapSelection customFlag = static_cast<CustomMapSelection>(
            reg->GetValueDword("CustomMultiMap", IDX(CUSTOM_MAP_UNINITIALIZED))
        );
        if (g_multiState->m_isHost != 0 && customFlag != CUSTOM_MAP_UNINITIALIZED) {
            char mapName[0x100];
            DWORD size = 0x100;
            reg->GetValueString("LastMultiMap", mapName, &size, g_emptyString);
            m_customWorldFlag = IDX(customFlag);
            if (customFlag != CUSTOM_MAP_STANDARD) {
                char path[0x100];
                sprintf(path, "custom\\%s", mapName);
                FILE* f = fopen(path, "rb");
                if (f != NULL) {
                    HWND worldCombo = GetDlgItem(0x4ff)->m_hWnd;
                    CWnd* child = CWnd::FromHandle(::GetWindow(worldCombo, GW_CHILD));
                    if (child == NULL) {
                        return;
                    }
                    child->SetWindowTextA(mapName);
                    g_multiState->m_customLevel = 1;
                    g_multiState->m_customLevelName = mapName;
                    g_multiState->m_builtInLevelName = g_emptyString;
                    fclose(f);
                }
            } else {
                CWnd* child = CWnd::FromHandle(::GetWindow(GetDlgItem(0x4ff)->m_hWnd, GW_CHILD));
                if (child == NULL) {
                    return;
                }
                child->SetWindowTextA(mapName);
                g_multiState->m_customLevel = 0;
                g_multiState->m_customLevelName = g_emptyString;
                g_multiState->m_builtInLevelName = mapName;
            }
        }
        {
            CWnd* w = GetDlgItem(0x511);
            g_sharedFlag = (w == NULL) ? 0 : w->m_hWnd;
        }
        g_multiState->m_netGate->m_sessionSel = NULL;
        g_multiState->PollSession();
        if (!UpdateColorItems()) {
            return;
        }
        if (!UpdateSlot()) {
            return;
        }
        if (!UpdatePlayers(1)) {
            return;
        }
    } else {
        HWND worldCombo = GetDlgItem(0x4ff)->m_hWnd;
        CWnd* child = CWnd::FromHandle(::GetWindow(worldCombo, GW_CHILD));
        if (child == NULL) {
            return;
        }
        child->GetWindowTextA(m_worldName);
        if (g_multiState->m_isHost != 0) {
            reg->SetValueString("LastMultiMap", m_worldName);
            reg->SetValueDword("CustomMultiMap", m_customWorldFlag);
        }
        GruntzPlayer* slots = m_host->m_options;
        for (i32 i = 0; i < NUM_PLAYER_SLOTS; i++) {
            CWnd* e = GetCtrlB(i);
            if (e != NULL) {
                CString temp;
                e->GetWindowTextA(temp);
                slots[i].m_name = temp;
            }
        }
        NetLobby::g_curDlg = NULL;
    }
    FlashCtrlD();
}

RVA(0x000c2620, 0x6)
const AFX_MSGMAP* CMultiStartDlg::GetMessageMap() const {
    return &messageMap;
}

RVA(0x000c2640, 0x60)
CWnd* CMultiStartDlg::GetCtrlE(i32 index) {
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

RVA(0x000c26c0, 0x60)
CWnd* CMultiStartDlg::GetCtrlA(i32 index) {
    CWnd* result = 0;
    switch (static_cast<PlayerSlot>(index)) {
        case PLAYER_SLOT_0:
            result = GetDlgItem(CTRL_PLAYER_CTRL_A0);
            break;
        case PLAYER_SLOT_1:
            result = GetDlgItem(CTRL_PLAYER_CTRL_A1);
            break;
        case PLAYER_SLOT_2:
            result = GetDlgItem(CTRL_PLAYER_CTRL_A2);
            break;
        case PLAYER_SLOT_3:
            result = GetDlgItem(CTRL_PLAYER_CTRL_A3);
            break;
    }
    return result;
}

RVA(0x000c2740, 0x60)
CWnd* CMultiStartDlg::GetCtrlB(i32 index) {
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

RVA(0x000c27c0, 0x60)
CWnd* CMultiStartDlg::GetCtrlC(i32 index) {
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

RVA(0x000c2840, 0x60)
CWnd* CMultiStartDlg::GetCtrlD(i32 index) {
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

RVA(0x000c28c0, 0x27)
void CMultiStartDlg::SetComboSelE(i32 index, i32 sel) {
    CWnd* c = GetCtrlE(index);
    if (c != NULL) {
        ::SendMessageA(c->m_hWnd, CB_SETCURSEL, sel, 0);
    }
}

RVA(0x000c2900, 0x2a)
i32 CMultiStartDlg::GetComboSelE(i32 index) {
    CWnd* c = GetCtrlE(index);
    if (c == NULL) {
        return -1;
    }
    return ::SendMessageA(c->m_hWnd, CB_GETCURSEL, 0, 0);
}

RVA(0x000c2940, 0x2b)
i32 CMultiStartDlg::GetComboSelC(i32 id) {
    CWnd* c = GetCtrlC(id);
    if (c == NULL) {
        return -1;
    }
    return ::SendMessageA(c->m_hWnd, CB_GETCURSEL, 0, 0) + 1;
}

RVA(0x000c2c80, 0x0)
// @confidence: high
// @source: msgmap-pfn (the AFX_MSGMAP_ENTRY pfn slot names this handler)
// @stub
void CMultiStartDlg::OnTimer(u32 nIDEvent) {}

RVA(0x000c30d0, 0x0)
// @confidence: high
// @source: msgmap-pfn (the AFX_MSGMAP_ENTRY pfn slot names this handler)
// @stub
void CMultiStartDlg::OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis) {}

RVA(0x000c4e40, 0x0)
// @confidence: high
// @source: msgmap-pfn (the AFX_MSGMAP_ENTRY pfn slot names this handler)
// @stub
void CMultiStartDlg::OnEnChange50a() {}

RVA(0x000c4e60, 0x0)
// @confidence: high
// @source: msgmap-pfn (the AFX_MSGMAP_ENTRY pfn slot names this handler)
// @stub
void CMultiStartDlg::OnEnChange50b() {}

RVA(0x000c4e80, 0x0)
// @confidence: high
// @source: msgmap-pfn (the AFX_MSGMAP_ENTRY pfn slot names this handler)
// @stub
void CMultiStartDlg::OnEnChange50c() {}

RVA(0x000c4ea0, 0x0)
// @confidence: high
// @source: msgmap-pfn (the AFX_MSGMAP_ENTRY pfn slot names this handler)
// @stub
void CMultiStartDlg::OnEnChange50d() {}

RVA(0x000c51c0, 0x0)
// @confidence: high
// @source: msgmap-pfn (the AFX_MSGMAP_ENTRY pfn slot names this handler)
// @stub
void CMultiStartDlg::OnCmd51f() {}

RVA(0x000c51e0, 0x0)
// @confidence: high
// @source: msgmap-pfn (the AFX_MSGMAP_ENTRY pfn slot names this handler)
// @stub
void CMultiStartDlg::OnCmd523() {}

RVA(0x000c5200, 0x0)
// @confidence: high
// @source: msgmap-pfn (the AFX_MSGMAP_ENTRY pfn slot names this handler)
// @stub
void CMultiStartDlg::OnCmd524() {}

RVA(0x000c5220, 0x0)
// @confidence: high
// @source: msgmap-pfn (the AFX_MSGMAP_ENTRY pfn slot names this handler)
// @stub
void CMultiStartDlg::OnCmd525() {}
