#include <rva.h>

#include <Gruntz/MultiStartDlg.h>

#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Gruntz/MultiStartDlgCtrlId.h>
#include <MsgParam.h>
#include <Net/LatencyList.h>
#include <Net/NetMgr.h>

#include <string.h>

RVA_DYNINIT(0x000c1690, 0xa, g_gruntNames)
RVA_DYNINIT(0x000c16b0, 0x3d, g_gruntNames)
RVA_DYNINIT(0x000c1700, 0xe, g_gruntNames)
RVA_DYNINIT(0x000c1720, 0x14, g_gruntNames)
DATA(0x0024bdb0)
CString g_gruntNames[4] = {"Beefy", "Zed", "Serra", "Jebediah"};

DATA(0x0024bdc0)
WNDPROC g_savedMultiWndProc = NULL;

DATA(0x0024bd5c)
CMulti* g_multiState;

DATA(0x0024bdc4)
i32 g_watchBusy;
DATA(0x0024bdc8)
i32 g_watchBlinkA;
DATA(0x0024bdcc)
i32 g_watchBlinkB;

#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Net/KeyedList.h>
#include <Ints.h>
#include <Net/NetLobbyCtrlId.h>

DATA(0x0021243c)
char s_UsingCmdDelay[] = "Using CmdDelay of %d and ResendDelay of %d.";

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
// clang-format off
const AFX_MSGMAP_ENTRY CMultiStartDlg::_messageEntries[] = {
    ON_CBN_SELCHANGE(IDC_MULTI_CHANNEL0, CMultiStartDlg::ReconcileChannel0)
    ON_CBN_SELCHANGE(IDC_MULTI_CONNECT, CMultiStartDlg::ConnectStep)
    ON_CBN_SELCHANGE(IDC_MULTI_CHANNEL2, CMultiStartDlg::ReconcileChannel2)
    ON_CBN_SELCHANGE(IDC_MULTI_CHANNEL3, CMultiStartDlg::ReconcileChannel3)
    {WM_TIMER, 0, 0, 0, AfxSig_vw, reinterpret_cast<AFX_PMSG>(&CMultiStartDlg::OnTimer)}, // API-forced MFC seam.
    {WM_MEASUREITEM, 0, 0, 0, AfxSig_vOWNER, reinterpret_cast<AFX_PMSG>(&CMultiStartDlg::OnMeasureItem)}, // API-forced MFC seam.
    {WM_DRAWITEM, 0, 0, 0, AfxSig_vOWNER, reinterpret_cast<AFX_PMSG>(&CMultiStartDlg::OnDrawItem)}, // API-forced MFC seam.
    ON_BN_CLICKED(IDC_MULTI_COLOR0, CMultiStartDlg::OnColorSlot0)
    ON_BN_CLICKED(IDC_MULTI_COLOR1, CMultiStartDlg::OnColorSlot1)
    ON_BN_CLICKED(IDC_MULTI_COLOR2, CMultiStartDlg::OnColorSlot2)
    ON_BN_CLICKED(IDC_MULTI_COLOR3, CMultiStartDlg::OnColorSlot3)
    ON_BN_CLICKED(IDC_MULTI_CUSTOM_WORLD, CMultiStartDlg::OnCustomWorld)
    ON_CBN_SELCHANGE(IDC_MULTI_WORLD, CMultiStartDlg::CommitWorldHost)
    ON_BN_CLICKED(0x4c6, CMultiStartDlg::OnChatSend)
    ON_EN_CHANGE(0x50b, CMultiStartDlg::OnPlayerNameChange1)
    ON_EN_CHANGE(0x50a, CMultiStartDlg::OnPlayerNameChange0)
    ON_EN_CHANGE(0x50c, CMultiStartDlg::OnPlayerNameChange2)
    ON_EN_CHANGE(0x50d, CMultiStartDlg::OnPlayerNameChange3)
    ON_CBN_SELCHANGE(IDC_MULTI_SLOT0, CMultiStartDlg::OnSlotSelect0)
    ON_CBN_SELCHANGE(IDC_MULTI_SLOT1, CMultiStartDlg::OnSlotSelect1)
    ON_CBN_SELCHANGE(IDC_MULTI_SLOT2, CMultiStartDlg::OnSlotSelect2)
    ON_CBN_SELCHANGE(IDC_MULTI_SLOT3, CMultiStartDlg::OnSlotSelect3)
    ON_CBN_SELCHANGE(IDC_MULTI_LATENCY, CMultiStartDlg::CommitLatencyOption)
    ON_BN_CLICKED(0x51f, CMultiStartDlg::OnReadyToggle0)
    ON_BN_CLICKED(0x523, CMultiStartDlg::OnReadyToggle1)
    ON_BN_CLICKED(0x524, CMultiStartDlg::OnReadyToggle2)
    ON_BN_CLICKED(0x525, CMultiStartDlg::OnReadyToggle3)
    ON_BN_CLICKED(IDC_MULTI_ECHO_LATENCY, CMultiStartDlg::EchoLatencySettings)
    ON_CBN_SELCHANGE(IDC_MULTI_WORLD, CMultiStartDlg::CommitWorldHost)
    {0, 0, 0, 0, AfxSig_end, 0},
};
// clang-format on

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
    CWnd* combo = GetDlgItem(IDX(IDC_MULTI_WORLD));
    if (combo == NULL) {
        return 0;
    }
    CSymTab* st = m_host->m_symParser->ResolvePath("GAME_MULTI");
    if (st == NULL) {
        return 0;
    }
    CParseSource* item = st->NextSym2(st->FirstSym());
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
        item = st->NextSym3(item);
    }
    CWnd* combo2 = GetDlgItem(IDX(IDC_MULTI_WORLD));
    CWnd* child = CWnd::FromHandle(::GetWindow(combo2->m_hWnd, GW_CHILD));
    if (child == NULL) {
        return 0;
    }
    ::SendMessageA(child->m_hWnd, EM_SETREADONLY, 1, 0);
    ::SendMessageA(combo->m_hWnd, CB_SETCURSEL, 0, 0);
    HWND__* h = child->m_hWnd;
    g_savedMultiWndProc = reinterpret_cast<WNDPROC>(GetWindowLongA(h, GWL_WNDPROC));
    SetWindowLongA(h, GWL_WNDPROC, reinterpret_cast<LONG>(MultiMapComboEditProc));
    CommitWorldHost();
    return 1;
}

RVA(0x000c1a10, 0x70)
LRESULT CALLBACK MultiMapComboEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_SETTEXT) {
        if (strcmp("", reinterpret_cast<LPCTSTR>(lParam)) == 0) {
            return 0;
        }
    }
    return CallWindowProcA(g_savedMultiWndProc, hWnd, msg, wParam, lParam);
}

// @early-stop
RVA(0x000c1aa0, 0x2f8)
i32 CMultiStartDlg::UpdateColorItems() {
    if (g_multiState->m_isHost != 0) {
        CWnd* it4ff = GetDlgItem(IDX(IDC_MULTI_WORLD));
        CWnd* itChild = CWnd::FromHandle(::GetWindow(GetDlgItem(IDX(IDC_MULTI_WORLD))->m_hWnd, 5));
        CWnd* it42b = GetDlgItem(IDX(IDC_MULTI_CUSTOM_WORLD));
        CWnd* it4e9 = GetDlgItem(IDX(IDC_MULTI_ECHO_LATENCY));
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
    CWnd* it4ff = GetDlgItem(IDX(IDC_MULTI_WORLD));
    CWnd* itChild = CWnd::FromHandle(::GetWindow(GetDlgItem(IDX(IDC_MULTI_WORLD))->m_hWnd, 5));
    CWnd* it42b = GetDlgItem(IDX(IDC_MULTI_CUSTOM_WORLD));
    CWnd* it4e9 = GetDlgItem(IDX(IDC_MULTI_ECHO_LATENCY));
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
    m_slotList->FillCombo(v, IDX(IDC_MULTI_LATENCY));
    m_slotList->SelectItem(v, IDX(IDC_MULTI_LATENCY), 0, 0);
    g_multiState->m_autoCommandDelay = 1;
    return 1;
}

// @early-stop
RVA(0x000c1fd0, 0x99)
i32 CMultiStartDlg::UpdateSlot() {
    CWnd* w = GetDlgItem(IDX(IDC_MULTI_LATENCY));
    if (w == NULL) {
        return 0;
    }
    CMulti* reg = g_multiState;
    if (reg->m_isHost) {
        i32 idx = GetSlotIndex();
        w->EnableWindow(m_host->m_options[idx].m_readyFlag == 0);
    } else {
        w->EnableWindow(0);
    }
    HWND v = GetSafe1c();
    CMulti* reg2 = g_multiState;
    if (reg2->m_autoCommandDelay) {
        m_slotList->SelectItem(v, IDX(IDC_MULTI_LATENCY), 0, 0);
    } else {
        m_slotList
            ->SelectItem(v, IDX(IDC_MULTI_LATENCY), reg2->m_commandDelay, reg2->m_drainReload);
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
        HWND chatEdit = GetDlgItem(IDX(IDC_MULTI_CHAT_INPUT))->m_hWnd;
        pSend(chatEdit, EM_LIMITTEXT, 100, 0);
        CustomMapSelection customFlag = static_cast<CustomMapSelection>(
            reg->GetValueDword("CustomMultiMap", IDX(CUSTOM_MAP_UNINITIALIZED))
        );
        if (g_multiState->m_isHost != 0 && customFlag != CUSTOM_MAP_UNINITIALIZED) {
            char mapName[0x100];
            DWORD size = 0x100;
            reg->GetValueString("LastMultiMap", mapName, &size, "");
            m_customWorldFlag = IDX(customFlag);
            if (customFlag != CUSTOM_MAP_STANDARD) {
                char path[0x100];
                sprintf(path, "custom\\%s", mapName);
                FILE* f = fopen(path, "rb");
                if (f != NULL) {
                    HWND worldCombo = GetDlgItem(IDX(IDC_MULTI_WORLD))->m_hWnd;
                    CWnd* child = CWnd::FromHandle(::GetWindow(worldCombo, GW_CHILD));
                    if (child == NULL) {
                        return;
                    }
                    child->SetWindowTextA(mapName);
                    g_multiState->m_customLevel = 1;
                    g_multiState->m_customLevelName = mapName;
                    g_multiState->m_builtInLevelName = "";
                    fclose(f);
                }
            } else {
                CWnd* child = CWnd::FromHandle(
                    ::GetWindow(GetDlgItem(IDX(IDC_MULTI_WORLD))->m_hWnd, GW_CHILD)
                );
                if (child == NULL) {
                    return;
                }
                child->SetWindowTextA(mapName);
                g_multiState->m_customLevel = 0;
                g_multiState->m_customLevelName = "";
                g_multiState->m_builtInLevelName = mapName;
            }
        }
        {
            CWnd* w = GetDlgItem(IDX(IDC_MULTI_CHAT_LOG));
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
        HWND worldCombo = GetDlgItem(IDX(IDC_MULTI_WORLD))->m_hWnd;
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c28c0, 0x27)
void CMultiStartDlg::SetComboSelE(i32 index, i32 sel) {
    CWnd* c = GetCtrlE(index);
    if (c != NULL) {
        ::SendMessageA(c->m_hWnd, CB_SETCURSEL, sel, 0);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c2900, 0x2a)
i32 CMultiStartDlg::GetComboSelE(i32 index) {
    CWnd* c = GetCtrlE(index);
    if (c == NULL) {
        return -1;
    }
    return ::SendMessageA(c->m_hWnd, CB_GETCURSEL, 0, 0);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c2940, 0x2b)
i32 CMultiStartDlg::GetComboSelC(i32 id) {
    CWnd* c = GetCtrlC(id);
    if (c == NULL) {
        return -1;
    }
    return ::SendMessageA(c->m_hWnd, CB_GETCURSEL, 0, 0) + 1;
}

RVA(0x000c2980, 0x28)
void CMultiStartDlg::SetListCurSel(i32 id, i32 wParam) {
    CWnd* it = GetCtrlC(id);
    if (it) {
        ::SendMessageA(it->m_hWnd, CB_SETCURSEL, wParam - 1, 0);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c29c0, 0x1d)
void CMultiStartDlg::SetPlayerName(i32 index, const char* name) {
    CWnd* item = GetCtrlB(index);
    if (item != NULL) {
        item->SetWindowTextA(name);
    }
}

RVA(0x000c29f0, 0x13)
void CMultiStartDlg::ReconcileChannel0() {
    SyncChannelSlot(0);
    Drive();
}

RVA(0x000c2a20, 0x13)
void CMultiStartDlg::ConnectStep() {
    SyncChannelSlot(1);
    Drive();
}

RVA(0x000c2a50, 0x13)
void CMultiStartDlg::ReconcileChannel2() {
    SyncChannelSlot(2);
    Drive();
}

RVA(0x000c2a80, 0x13)
void CMultiStartDlg::ReconcileChannel3() {
    SyncChannelSlot(3);
    Drive();
}

// @early-stop The zero-selection arm is a compound guard followed by an
// else-if: retail re-tests m_humanControlled when the second conjunct fails,
// but jumps over that re-test after DropChannelPlayer.
RVA(0x000c2ab0, 0x161)
void CMultiStartDlg::SyncChannelSlot(i32 ch) {
    CWnd* owner = GetCtrlE(ch);
    CWnd* c1 = GetCtrlB(ch);
    CWnd* c2 = GetCtrlD(ch);
    GetCtrlC(ch);
    GetCtrlA(ch);
    GruntzPlayer* s = &m_host->m_options[ch];
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM) = ::SendMessageA;
    if (pSend(owner->m_hWnd, CB_GETCURSEL, 0, 0) == 0) {
        if (s->m_humanControlled && s->m_liveGate) {
            g_multiState->DropChannelPlayer(s->m_playerIndex);
        } else if (!s->m_humanControlled && s->m_liveGate) {
            ChannelSlots_Set(IDX(s->m_colorIndex), 1);
        }
        s->m_liveGate = 0;
        s->m_readyFlag = 0;
        c1->EnableWindow(0);
        c2->EnableWindow(0);
    } else {
        if (static_cast<MultiplayerPlayerKind>(pSend(owner->m_hWnd, CB_GETCURSEL, 0, 0))
            != MULTI_PLAYER_HUMAN) {
            if (s->m_humanControlled != 0) {
                if (s->m_liveGate != 0) {
                    g_multiState->DropChannelPlayer(s->m_playerIndex);
                }
                i32 free = ChannelSlots_FindFree();
                s->m_colorIndex = static_cast<ColorTint>(free);
                ChannelSlots_Set(free, 0);
            } else if (s->m_liveGate == 0) {
                i32 free = ChannelSlots_FindFree();
                s->m_colorIndex = static_cast<ColorTint>(free);
                ChannelSlots_Set(free, 0);
            }
            s->m_readyFlag = 1;
            s->m_humanControlled = 0;
            s->m_configId = static_cast<i32>(pSend(owner->m_hWnd, CB_GETCURSEL, 0, 0)) - 1;
            s->m_liveGate = 1;
            s->m_name = g_gruntNames[ch];
        }
        c1->EnableWindow(1);
        c2->EnableWindow(1);
    }
}

RVA(0x000c2c80, 0x1a)
void CMultiStartDlg::OnTimer(u32 nIDEvent) {
    switch (nIDEvent) {
        case MULTI_START_WATCHDOG_TIMER:
            Watchdog();
            break;
    }
    Default();
}

RVA(0x000c2cb0, 0x1f)
i32 CMultiStartDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    ::SetTimer(m_hWnd, MULTI_START_WATCHDOG_TIMER, 0x32, 0);
    return 1;
}

RVA(0x000c2ce0, 0xf3)
void CMultiStartDlg::AppendChatLine(char* str) {
    CWnd* item = GetDlgItem(IDX(IDC_MULTI_CHAT_LOG));
    HWND edit;
    if (!item) {
        edit = NULL;
    } else {
        edit = item->m_hWnd;
    }
    if (!edit || !str || !str[0]) {
        return;
    }
    i32 len = ::GetWindowTextLengthA(edit);
    if (len == 0) {
        ::SendMessageA(edit, EM_SETSEL, len, -1);
    } else {
        ::SendMessageA(edit, EM_SETSEL, len, len);
    }
    char buf[0x80];
    buf[0] = 0;
    if (len > 0) {
        strcat(buf, "\r\n");
    }
    strcat(buf, str);
    MsgParam text;
    text.m_str = buf;
    ::SendMessageA(edit, EM_REPLACESEL, 0, text.m_lparam);
    ::SendMessageA(edit, EM_LINESCROLL, 0, 0x270f);
}

RVA(0x000c2e20, 0x21d)
i32 CMultiStartDlg::FlashCtrlD() {
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
        // INSIDE each arm and cross-jumps only the shared `call CreateSolidBrush`.
        if (it->IsWindowEnabled()) {
            GetRandomNumber();
            GetRandomNumber();
            i32 v = (GetRandomNumber() % 0xff) & 0xff;
            scratch.Attach(CreateSolidBrush((v << 8 | v) << 8 | v));
        } else {
            scratch.Attach(CreateSolidBrush(0x808080));
        }
        FillRect(dc.m_hDC, &rc, scratch);
    }
    return 1;
}

RVA(0x000c30d0, 0x12)
void CMultiStartDlg::OnMeasureItem(i32 nIDCtl, MEASUREITEMSTRUCT* lpmis) {
    CWnd::OnMeasureItem(nIDCtl, lpmis);
}

RVA(0x000c3100, 0x5c0)
void CMultiStartDlg::OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis) {
    COLORREF color;
    i32 bDraw = 0;
    switch (nIDCtl) {
        case CTRL_PLAYER_COLOR0:
            if (GetCtrlD(0)->IsWindowEnabled()) {
                switch (m_host->m_options[0].m_colorIndex) {
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
                switch (m_host->m_options[1].m_colorIndex) {
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
                switch (m_host->m_options[2].m_colorIndex) {
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
                switch (m_host->m_options[3].m_colorIndex) {
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

RVA(0x000c3830, 0xd1)
void CMultiStartDlg::OnColorSlot0() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || m_host->m_options[0].m_humanControlled != 0)
        && (m_host->m_options[0].m_readyFlag != 0
            || m_host->m_options[0].m_slotKey != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 0, 1, 0);
    if (dlg.DoModal() == 1) {
        if (SelectColor(0, static_cast<ColorTint>(dlg.m_pickedColor))) {
            Drive();
            GetDlgItem(CTRL_PLAYER_COLOR0)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x000c3950, 0xd1)
void CMultiStartDlg::OnColorSlot1() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || m_host->m_options[1].m_humanControlled != 0)
        && (m_host->m_options[1].m_readyFlag != 0
            || m_host->m_options[1].m_slotKey != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 1, 1, 0);
    if (dlg.DoModal() == 1) {
        if (SelectColor(1, static_cast<ColorTint>(dlg.m_pickedColor))) {
            Drive();
            GetDlgItem(CTRL_PLAYER_COLOR1)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x000c3a70, 0xd1)
void CMultiStartDlg::OnColorSlot2() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || m_host->m_options[2].m_humanControlled != 0)
        && (m_host->m_options[2].m_readyFlag != 0
            || m_host->m_options[2].m_slotKey != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 2, 1, 0);
    if (dlg.DoModal() == 1) {
        if (SelectColor(2, static_cast<ColorTint>(dlg.m_pickedColor))) {
            Drive();
            GetDlgItem(CTRL_PLAYER_COLOR2)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x000c3b90, 0xd1)
void CMultiStartDlg::OnColorSlot3() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || m_host->m_options[3].m_humanControlled != 0)
        && (m_host->m_options[3].m_readyFlag != 0
            || m_host->m_options[3].m_slotKey != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 3, 1, 0);
    if (dlg.DoModal() == 1) {
        if (SelectColor(3, static_cast<ColorTint>(dlg.m_pickedColor))) {
            Drive();
            GetDlgItem(CTRL_PLAYER_COLOR3)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x000c3cb0, 0x128)
void CMultiStartDlg::OnCustomWorld() {
    if (g_multiState->m_isHost == 0) {
        return;
    }
    CBattlezDlgCustom dlg(0);
    if (dlg.DoModal() == 1 && dlg.m_customName.GetLength() != 0) {

        CWnd* item = GetDlgItem(IDX(IDC_MULTI_WORLD));
        CWnd* child = CWnd::FromHandle(::GetWindow(item->m_hWnd, GW_CHILD));

        if (child == NULL) {
            return;
        }
        dlg.m_customName.MakeUpper();
        child->SetWindowTextA(static_cast<LPCTSTR>(dlg.m_customName));
        m_customWorldFlag = 1;
        g_multiState->m_customLevel = 1;
        g_multiState->m_customLevelName = static_cast<LPCTSTR>(dlg.m_customName);
        g_multiState->m_builtInLevelName = "";
        g_multiState->SaveConfig(0);
    }
}

RVA(0x000c3e30, 0xfe)
void CMultiStartDlg::CommitWorldHost() {
    if (g_multiState->m_isHost != 0) {
        CWnd* item = GetDlgItem(IDX(IDC_MULTI_WORLD));
        if (item != NULL) {
            i32 r = ::SendMessageA(item->m_hWnd, CB_GETCURSEL, 0, 0);
            if (r != -1) {
                CString name;
                (static_cast<CComboBox*>(item))->GetLBText(r, name);
                if (name.GetLength() != 0) {
                    m_customWorldFlag = 0;
                }
                g_multiState->m_customLevel = 0;
                g_multiState->m_customLevelName = "";
                g_multiState->m_builtInLevelName = static_cast<LPCTSTR>(name);
                g_multiState->SaveConfig(0);
            }
        }
    }
}

RVA(0x000c3f70, 0xfb)
void CMultiStartDlg::OnChatSend() {
    CWnd* input = GetDlgItem(IDX(IDC_MULTI_CHAT_INPUT));
    if (input == NULL) {
        return;
    }
    CString a, b;
    GetCtrlB(GetSlotIndex())->GetWindowTextA(a);
    a += " says: ";
    input->GetWindowTextA(b);
    if (b.GetLength() != 0) {
        a += b;
        AppendChatLine(const_cast<char*>(static_cast<const char*>(a)));
        input->SetWindowTextA("");
        g_multiState->BroadcastChatLine(const_cast<char*>(static_cast<const char*>(a)), 0, 0, 0);
    }
}

RVA(0x000c40b0, 0x42)
void CMultiStartDlg::Drive() {
    CMulti* netMgr = g_multiState;
    if (netMgr->m_isHost != 0) {
        netMgr->BroadcastChannelTable(0);
        UpdatePlayers(1);
    } else {
        g_multiState->BroadcastOneChannel(m_host->FindOptionsSlot(netMgr->m_hostIndex));
    }
}

RVA(0x000c4120, 0xc2)
i32 CMultiStartDlg::EnableControls() {
    CWnd* w = GetDlgItem(IDCANCEL);
    w->EnableWindow(1);
    w = GetDlgItem(IDX(IDC_NETCHAT_SEND));
    w->EnableWindow(1);
    w = GetDlgItem(IDX(IDC_MULTI_CHAT_INPUT));
    w->EnableWindow(1);
    w = GetDlgItem(IDX(IDC_MULTI_CHAT_LOG));
    w->EnableWindow(1);
    CString s1;
    if (g_multiState->m_customLevel == 0) {
        CString s2;
    }
    return 1;
}

// @early-stop
RVA(0x000c4230, 0x38e)
i32 CMultiStartDlg::UpdatePlayers(i32 force) {
    CWnd::FromHandle(::GetFocus());
    i32 f1c = 1;
    i32 f18 = 0;
    i32 t = this->GetSlotIndex();
    i32 localColour = g_multiState->m_isHost ? m_host->m_options[t].m_readyFlag : 1;
    for (i32 idx = 0; idx < 4; idx++) {
        GruntzPlayer* slot = &g_gameReg->m_options[idx];
        if (slot) {
            if (slot->m_slotKey != g_multiState->m_hostIndex && slot->m_humanControlled
                && slot->m_liveGate) {
                f18 = 1;
            }
            CWnd* name = GetCtrlB(idx);
            if ((g_multiState->m_isHost && slot->m_humanControlled == 0)
                || slot->m_slotKey == g_multiState->m_hostIndex) {
                name->EnableWindow(1);
            } else {
                name->EnableWindow(0);
            }
            CWnd* kind = GetCtrlE(idx);
            if (g_multiState->m_isHost && localColour == 0
                && slot->m_slotKey != g_multiState->m_hostIndex) {
                kind->EnableWindow(1);
            } else {
                kind->EnableWindow(0);
            }
            CWnd* ready = GetCtrlA(idx);
            if (slot->m_slotKey == g_multiState->m_hostIndex) {
                ready->EnableWindow(1);
            } else {
                ready->EnableWindow(0);
            }
            if (slot->m_readyFlag == 0) {
                if (slot->m_liveGate) {
                    ::SendMessageA(ready->m_hWnd, BM_SETCHECK, 0, 0);
                    f1c = 0;
                } else {
                    ::SendMessageA(ready->m_hWnd, BM_SETCHECK, 0, 0);
                }
            } else if (slot->m_liveGate) {
                ::SendMessageA(ready->m_hWnd, BM_SETCHECK, 1, 0);
            } else {
                ::SendMessageA(ready->m_hWnd, BM_SETCHECK, 0, 0);
            }
            CWnd* color = GetCtrlC(idx);
            // The &&-chain as the ARGUMENT, not an if/else round two calls: retail
            // materialises the flag (mov edx,1 / jmp / xor edx,edx) and pushes it once.
            color->EnableWindow(g_multiState->m_isHost && slot->m_liveGate && localColour == 0);
            SetListCurSel(idx, slot->m_liveGate ? slot->m_comboSel : 0);
            if (force == 0) {
                if (this->GetSlotIndex() == idx) {
                    continue;
                }
                if (g_multiState->m_isHost && slot->m_humanControlled == 0) {
                    continue;
                }
            }
            if (slot->m_liveGate) {
                {
                    force = 0;
                    GetCtrlB(idx)->SetWindowTextA(slot->GetName());
                }
                // The combo is fetched into a local FIRST at each of the three sites -
                // retail calls GetCtrlE inside both arms and only then pushes the
                // message args. Written inline the constants push first, which lets cl
                // cross-jump the two arms onto ONE GetCtrlE call (retail has two).
                if (slot->m_humanControlled) {
                    CWnd* cb = GetCtrlE(idx);
                    ::SendMessageA(cb->m_hWnd, CB_SETCURSEL, 4, 0);
                } else {
                    i32 sel = slot->m_configId;
                    CWnd* cb = GetCtrlE(idx);
                    ::SendMessageA(cb->m_hWnd, CB_SETCURSEL, sel + 1, 0);
                }
            } else {
                GetCtrlB(idx)->SetWindowTextA("");
                CWnd* cb = GetCtrlE(idx);
                ::SendMessageA(cb->m_hWnd, CB_SETCURSEL, 0, 0);
            }
            this->SyncChannelSlot(idx);
        }
    }
    if (g_multiState->m_isHost) {
        CWnd* ok = this->GetDlgItem(1);
        if (ok == NULL) {
            return 0;
        }
        ok->EnableWindow(f18 & f1c);
    }
    HWND color0 = this->GetDlgItem(CTRL_PLAYER_COLOR0)->m_hWnd;
    ::InvalidateRect(color0, 0, 1);
    HWND color1 = this->GetDlgItem(CTRL_PLAYER_COLOR1)->m_hWnd;
    ::InvalidateRect(color1, 0, 1);
    HWND color2 = this->GetDlgItem(CTRL_PLAYER_COLOR2)->m_hWnd;
    ::InvalidateRect(color2, 0, 1);
    HWND color3 = this->GetDlgItem(CTRL_PLAYER_COLOR3)->m_hWnd;
    ::InvalidateRect(color3, 0, 1);
    return 1;
}

// @early-stop
RVA(0x000c46b0, 0x384)
void CMultiStartDlg::Watchdog() {
    if (g_watchBusy != 0) {
        return;
    }
    g_watchBusy = 1;
    CNetPlayerListNode* h = g_multiState->m_netGate->m_playerSel;
    if (h == NULL) {
        return;
    }
    g_multiState->m_netGate->EnumGroupsRange(h, 0);
    g_multiState->ResolveLocalPlayer();
    if (g_watchBlinkA == 0) {
        u32 t = timeGetTime();
        g_multiState->SendNetStat(NETMSG_STAT_REQUEST, static_cast<i32>(t), 0);
    }
    if (g_multiState->m_isHost == 0) {
        if (g_watchBlinkA == 0) {
            g_multiState->ReportAckLatency();
        }
        EnableWindow(0);
        i32 r = g_multiState->VerifyCustomLevel(h, g_multiState->m_localPlayer);
        EnableWindow(1);
        if (r != 0) {
            EndDialog(1);
            g_watchBusy = 0;
            return;
        }
    } else {
        g_multiState->PollSession();
        if (g_multiState->m_autoCommandDelay != 0) {
            g_multiState->AutoTuneCmdDelay();
        }
    }
    i32 a = g_watchBlinkA + 1;
    g_watchBlinkA = a;
    if (a > 3) {
        g_watchBlinkA = 0;
    }
    if (g_watchBlinkB == 0) {
        for (i32 i = 0; i < 4; i++) {
            GruntzPlayer* slot = &g_gameReg->m_options[i];
            CWnd* item1;
            CWnd* item2;
            switch (static_cast<PlayerSlot>(i)) {
                case PLAYER_SLOT_0:
                    item1 = GetDlgItem(CTRL_PLAYER_LATENCY0);
                    item2 = GetDlgItem(CTRL_PLAYER_READY0);
                    break;
                case PLAYER_SLOT_1:
                    item1 = GetDlgItem(CTRL_PLAYER_LATENCY1);
                    item2 = GetDlgItem(CTRL_PLAYER_READY1);
                    break;
                case PLAYER_SLOT_2:
                    item1 = GetDlgItem(CTRL_PLAYER_LATENCY2);
                    item2 = GetDlgItem(CTRL_PLAYER_READY2);
                    break;
                case PLAYER_SLOT_3:
                    item1 = GetDlgItem(CTRL_PLAYER_LATENCY3);
                    item2 = GetDlgItem(CTRL_PLAYER_READY3);
                    break;
            }
            if (slot->m_liveGate != 0 && slot->m_humanControlled != 0) {
                char buf[0x20];
                wsprintfA(buf, "%d", slot->m_latency.m_avg);
                item1->SetWindowTextA(buf);
                item2->SetWindowTextA("ms");
            } else {
                item1->SetWindowTextA("");
                item2->SetWindowTextA("");
            }
        }
    }
    i32 b = g_watchBlinkB + 1;
    g_watchBlinkB = b;
    if (b > 0x31) {
        g_watchBlinkB = 0;
    }
    if (g_multiState->m_sessionTerminated != 0) {
        ::KillTimer(m_hWnd, 1);
        g_multiState->ReportVersionMsg("The game session has been terminated.", 0);
        g_watchBusy = 0;
        return;
    }
    if (g_multiState->m_colorSelectionRejected != 0) {
        g_multiState->m_colorSelectionRejected = 0;
        g_multiState->ReportVersionMsg("Someone has already selected that color.", 0);
        g_watchBusy = 0;
        return;
    }
    char* msg;
    if (g_multiState->m_removedByHost != 0) {
        ::KillTimer(m_hWnd, 1);
        msg = "You have been removed from the game by the host.";
    } else if (g_multiState->m_gameClosed != 0) {
        ::KillTimer(m_hWnd, 1);
        msg = "This game is closed.";
    } else if (g_multiState->m_gameFull != 0) {
        ::KillTimer(m_hWnd, 1);
        msg = "This game is already full.";
    } else if (g_multiState->m_versionMismatch != 0) {
        ::KillTimer(m_hWnd, 1);
        msg = "This version is not the same as the host computer's version of the game.";
    } else {
        if (g_playerLeftFlag != 0) {
            UpdatePlayers(1);
            EnableControls();
            UpdateColorItems();
            UpdateSlot();
            g_playerLeftFlag = 0;
        }
        if (g_multiState->m_connectAccepted != 0) {
            EnableControls();
            UpdateColorItems();
            UpdateSlot();
            g_multiState->m_connectAccepted = 0;
        }
        g_watchBusy = 0;
        return;
    }
    g_multiState->ReportVersionMsg(msg, 0);
    EndDialog(0);
    g_watchBusy = 0;
}

RVA(0x000c4b30, 0x1f)
i32 CMultiStartDlg::GetSlotIndex() {
    GruntzPlayer* slot = m_host->FindOptionsSlot(g_multiState->m_hostIndex);
    if (slot == NULL) {
        return -1;
    }
    return slot->m_playerIndex;
}

RVA(0x000c4b60, 0x77)
i32 CMultiStartDlg::SelectColor(i32 colorIndex, ColorTint playerColor) {
    GruntzPlayer* colorSlot = &m_host->m_options[colorIndex];
    if (g_multiState->m_isHost != 0) {
        i32 r = ChannelSlots_Get(IDX(playerColor));
        if (r == 0) {
            g_multiState->ReportVersionMsg("Someone has already selected that color.", r);
            return 0;
        }
        ChannelSlots_Set(IDX(colorSlot->m_colorIndex), 1);
        ChannelSlots_Set(IDX(playerColor), 0);
    }
    colorSlot->m_colorIndex = playerColor;
    return 1;
}

RVA(0x000c4c00, 0x190)
void CMultiStartDlg::OnOK() {
    if (g_multiState->m_isHost == 0) {
        return;
    }
    if (&CMulti::GetCommandDelay == NULL) {
        return;
    }
    if (&CMulti::GetResendDelay == NULL) {
        return;
    }
    g_multiState->SendStatFlag(NETMSG_VERIFY_CUSTOM_LEVEL, 1);
    i32 custom = g_multiState->m_customLevel;
    i32 token = g_gameReg->BuildLevelRezPath(
        0,
        0,
        custom,
        0,
        custom != 0 ? g_multiState->GetConfigNameB() : g_multiState->GetConfigNameA()
    );
    g_multiState->m_levelVerifyResult = 0;
    if (g_multiState->Poll(token) == 0) {
        g_multiState->m_customLevelVerificationPending = 0;
        EnableWindow(0);
        g_gameReg->EnterModalUI(
            "Unable to verify custom level with other players. The game will not start."
        );
        EnableWindow(1);
    } else if (g_multiState->m_levelVerifyResult != 0) {
        g_multiState->m_customLevelVerificationPending = 1;
        CDialog::OnOK();
    } else {
        g_multiState->m_customLevelVerificationPending = 0;
        EnableWindow(0);
        g_gameReg->EnterModalUI("Not all players have the (same) custom level.");
        EnableWindow(1);
    }
}

RVA(0x000c4e00, 0x7)
i32 CMulti::GetCommandDelay() {
    return m_commandDelay;
}

RVA(0x000c4e20, 0x7)
i32 CMulti::GetResendDelay() {
    return m_drainReload;
}

RVA(0x000c4e40, 0x8)
void CMultiStartDlg::OnPlayerNameChange0() {
    HandlePlayerNameChange(0);
}

RVA(0x000c4e60, 0x8)
void CMultiStartDlg::OnPlayerNameChange1() {
    HandlePlayerNameChange(1);
}

RVA(0x000c4e80, 0x8)
void CMultiStartDlg::OnPlayerNameChange2() {
    HandlePlayerNameChange(2);
}

RVA(0x000c4ea0, 0x8)
void CMultiStartDlg::OnPlayerNameChange3() {
    HandlePlayerNameChange(3);
}

// Defined after its four callers so cl cannot inline the empty body away -
// retail keeps the `push <slot>; call` at every EN_CHANGE site.
RVA(0x000c4ec0, 0x3)
void CMultiStartDlg::HandlePlayerNameChange(i32 slot) {}

RVA(0x000c4ee0, 0x33)
void CMultiStartDlg::OnSlotSelect0() {
    HWND h = GetCtrlC(0)->m_hWnd;
    g_gameReg->m_options[0].m_comboSel = ::SendMessageA(h, CB_GETCURSEL, 0, 0) + 1;
    Drive();
}

RVA(0x000c4f30, 0x33)
void CMultiStartDlg::OnSlotSelect1() {
    HWND h = GetCtrlC(1)->m_hWnd;
    g_gameReg->m_options[1].m_comboSel = ::SendMessageA(h, CB_GETCURSEL, 0, 0) + 1;
    Drive();
}

RVA(0x000c4f80, 0x33)
void CMultiStartDlg::OnSlotSelect2() {
    HWND h = GetCtrlC(2)->m_hWnd;
    g_gameReg->m_options[2].m_comboSel = ::SendMessageA(h, CB_GETCURSEL, 0, 0) + 1;
    Drive();
}

RVA(0x000c4fd0, 0x33)
void CMultiStartDlg::OnSlotSelect3() {
    HWND h = GetCtrlC(3)->m_hWnd;
    g_gameReg->m_options[3].m_comboSel = ::SendMessageA(h, CB_GETCURSEL, 0, 0) + 1;
    Drive();
}

// @early-stop
RVA(0x000c5020, 0x95)
void CMultiStartDlg::CommitLatencyOption() {
    if (g_multiState->m_isHost == 0) {
        return;
    }
    i32 lo, hi;
    HWND h = GetSafe1c();
    GetSelItemData(h, IDX(IDC_MULTI_LATENCY), &lo, &hi);
    if (lo != 0 || hi != 0) {
        g_multiState->m_commandDelay = lo;
        g_multiState->m_drainReload = hi;
        g_multiState->m_autoCommandDelay = 0;
        g_multiState->SaveConfig(0);
    } else {
        g_multiState->m_autoCommandDelay = 1;
    }
}

RVA(0x000c50f0, 0x9b)
void CMultiStartDlg::ToggleReady(i32 idx) {
    CWnd* it = GetCtrlA(idx);
    if (!it) {
        return;
    }
    i32 sel = ::SendMessageA(it->m_hWnd, BM_GETCHECK, 0, 0);
    GruntzPlayer* slot = &g_gameReg->m_options[idx];
    if (!slot) {
        return;
    }
    if (sel) {
        slot->m_readyFlag = 1;
    } else {
        slot->m_readyFlag = 0;
    }
    if (g_multiState->m_isHost) {
        g_multiState->BroadcastChannelTable(0);
        UpdatePlayers(1);
        EnableControls();
        UpdateColorItems();
        UpdateSlot();
    } else {
        g_multiState->BroadcastOneChannel(slot);
    }
}

RVA(0x000c51c0, 0x8)
void CMultiStartDlg::OnReadyToggle0() {
    ToggleReady(0);
}

RVA(0x000c51e0, 0x8)
void CMultiStartDlg::OnReadyToggle1() {
    ToggleReady(1);
}

RVA(0x000c5200, 0x8)
void CMultiStartDlg::OnReadyToggle2() {
    ToggleReady(2);
}

RVA(0x000c5220, 0x8)
void CMultiStartDlg::OnReadyToggle3() {
    ToggleReady(3);
}

RVA(0x000c5240, 0x2c)
i32 CMultiStartDlg::DestroyWindow() {
    CLatencyList* p = m_slotList;
    if (p) {
        p->CKeyedList::~CKeyedList();
        ::operator delete(p);
        m_slotList = NULL;
    }
    return CWnd::DestroyWindow();
}

RVA(0x000c52f0, 0x43)
void CMultiStartDlg::EchoLatencySettings() {
    char buf[128];
    wsprintfA(buf, s_UsingCmdDelay, g_multiState->m_commandDelay, g_multiState->m_drainReload);
    AppendChatLine(buf);
}
