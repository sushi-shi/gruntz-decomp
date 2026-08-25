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

RVA_DYNINIT(0x000c1690, 0xa, g_defaultPlayerNames)
RVA_DYNINIT(0x000c16b0, 0x3d, g_defaultPlayerNames)
RVA_DYNINIT(0x000c1700, 0xe, g_defaultPlayerNames)
RVA_DYNINIT(0x000c1720, 0x14, g_defaultPlayerNames)
DATA(0x0024bdb0)
CString g_defaultPlayerNames[4] = {"Beefy", "Zed", "Serra", "Jebediah"};

DATA(0x0024bdc0)
WNDPROC g_savedMultiWndProc = NULL;

DATA(0x0024bd5c)
CMulti* g_multiState;

DATA(0x0024bdc4)
i32 g_watchdogBusy;
DATA(0x0024bdc8)
i32 g_netStatsTick;
DATA(0x0024bdcc)
i32 g_latencyDisplayTick;

#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Net/KeyedList.h>
#include <Ints.h>
#include <Net/NetLobbyCtrlId.h>

DATA(0x0021243c)
char s_UsingCmdDelay[] = "Using CmdDelay of %d and ResendDelay of %d.";

#include <rva.h>

#include <Gruntz/MultiStartDlg.h>

#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Enums.h>
#include <Gruntz/CustomMapSelection.h>
#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Rez/RezArchiveEntry.h>
#include <MsgParam.h>
#include <Net/NetProviderNode.h>
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
    ON_CBN_SELCHANGE(CTRL_PLAYER_TYPE0, CMultiStartDlg::OnPlayerTypeSelection0)
    ON_CBN_SELCHANGE(CTRL_PLAYER_TYPE1, CMultiStartDlg::OnPlayerTypeSelection1)
    ON_CBN_SELCHANGE(CTRL_PLAYER_TYPE2, CMultiStartDlg::OnPlayerTypeSelection2)
    ON_CBN_SELCHANGE(CTRL_PLAYER_TYPE3, CMultiStartDlg::OnPlayerTypeSelection3)
    {WM_TIMER, 0, 0, 0, AfxSig_vw, GZ_MFC_PMSG(&CMultiStartDlg::OnTimer)},
    {WM_MEASUREITEM, 0, 0, 0, AfxSig_vOWNER, GZ_MFC_PMSG(&CMultiStartDlg::OnMeasureItem)},
    {WM_DRAWITEM, 0, 0, 0, AfxSig_vOWNER, GZ_MFC_PMSG(&CMultiStartDlg::OnDrawItem)},
    ON_BN_CLICKED(CTRL_PLAYER_COLOR0, CMultiStartDlg::OnPlayerColor0)
    ON_BN_CLICKED(CTRL_PLAYER_COLOR1, CMultiStartDlg::OnPlayerColor1)
    ON_BN_CLICKED(CTRL_PLAYER_COLOR2, CMultiStartDlg::OnPlayerColor2)
    ON_BN_CLICKED(CTRL_PLAYER_COLOR3, CMultiStartDlg::OnPlayerColor3)
    ON_BN_CLICKED(IDC_MULTI_CUSTOM_WORLD, CMultiStartDlg::OnCustomWorld)
    ON_CBN_SELCHANGE(IDC_MULTI_WORLD, CMultiStartDlg::CommitWorldSelection)
    ON_BN_CLICKED(0x4c6, CMultiStartDlg::OnChatSend)
    ON_EN_CHANGE(CTRL_PLAYER_NAME1, CMultiStartDlg::OnPlayerNameChange1)
    ON_EN_CHANGE(CTRL_PLAYER_NAME0, CMultiStartDlg::OnPlayerNameChange0)
    ON_EN_CHANGE(CTRL_PLAYER_NAME2, CMultiStartDlg::OnPlayerNameChange2)
    ON_EN_CHANGE(CTRL_PLAYER_NAME3, CMultiStartDlg::OnPlayerNameChange3)
    ON_CBN_SELCHANGE(CTRL_PLAYER_MAX_GRUNTZ0, CMultiStartDlg::OnMaxGruntzSelection0)
    ON_CBN_SELCHANGE(CTRL_PLAYER_MAX_GRUNTZ1, CMultiStartDlg::OnMaxGruntzSelection1)
    ON_CBN_SELCHANGE(CTRL_PLAYER_MAX_GRUNTZ2, CMultiStartDlg::OnMaxGruntzSelection2)
    ON_CBN_SELCHANGE(CTRL_PLAYER_MAX_GRUNTZ3, CMultiStartDlg::OnMaxGruntzSelection3)
    ON_CBN_SELCHANGE(IDC_MULTI_LATENCY, CMultiStartDlg::CommitLatencySelection)
    ON_BN_CLICKED(CTRL_PLAYER_READY0, CMultiStartDlg::OnReadyToggle0)
    ON_BN_CLICKED(CTRL_PLAYER_READY1, CMultiStartDlg::OnReadyToggle1)
    ON_BN_CLICKED(CTRL_PLAYER_READY2, CMultiStartDlg::OnReadyToggle2)
    ON_BN_CLICKED(CTRL_PLAYER_READY3, CMultiStartDlg::OnReadyToggle3)
    ON_BN_CLICKED(IDC_MULTI_ECHO_LATENCY, CMultiStartDlg::EchoLatencySettings)
    ON_CBN_SELCHANGE(IDC_MULTI_WORLD, CMultiStartDlg::CommitWorldSelection)
    {0, 0, 0, 0, AfxSig_end, 0},
};
// clang-format on

RVA(0x000c1750, 0x88)
CMultiStartDlg::CMultiStartDlg(CGruntzMgr* gameManager, CWnd* pParent)
    : CDialog(0xc5, pParent), m_reserved74(0xa) {
    m_gameManager = gameManager;
    m_customWorldFlag = 0;
    m_latencyOptions = NULL;
    g_multiState = static_cast<CMulti*>(g_gameReg->m_curState);
}

RVA_COMPGEN(0x000c1810, 0x1e, ??_GCMultiStartDlg@@UAEPAXI@Z)

RVA(0x000c1840, 0x16e)
i32 CMultiStartDlg::InitializeWorldCombo() {
    CWnd* combo = GetDlgItem(IDX(IDC_MULTI_WORLD));
    if (combo == NULL) {
        return 0;
    }
    CRezArchiveDir* worlds = m_gameManager->m_resourceArchive->FindDirectoryByPath("GAME_MULTI");
    if (worlds == NULL) {
        return 0;
    }
    CRezArchiveEntry* entry = worlds->FirstEntry(worlds->FirstType());
    while (entry != NULL) {
        CString name(entry->m_name);
        name.MakeUpper();
        MsgParam text;
        ::SendMessageA(
            combo->m_hWnd,
            CB_ADDSTRING,
            0,
            (text.m_str = static_cast<LPCTSTR>(name), text.m_lparam)
        );
        entry = worlds->NextEntry(entry);
    }
    CWnd* reloadedCombo = GetDlgItem(IDX(IDC_MULTI_WORLD));
    CWnd* editControl = CWnd::FromHandle(::GetWindow(reloadedCombo->m_hWnd, GW_CHILD));
    if (editControl == NULL) {
        return 0;
    }
    ::SendMessageA(editControl->m_hWnd, EM_SETREADONLY, 1, 0);
    ::SendMessageA(combo->m_hWnd, CB_SETCURSEL, 0, 0);
    HWND__* editHwnd = editControl->m_hWnd;
    g_savedMultiWndProc = reinterpret_cast<WNDPROC>(GetWindowLongA(editHwnd, GWL_WNDPROC));
    SetWindowLongA(editHwnd, GWL_WNDPROC, reinterpret_cast<LONG>(MultiMapComboEditProc));
    CommitWorldSelection();
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
i32 CMultiStartDlg::RefreshWorldControls() {
    if (g_multiState->m_isHost != 0) {
        CWnd* worldCombo = GetDlgItem(IDX(IDC_MULTI_WORLD));
        CWnd* worldEdit =
            CWnd::FromHandle(::GetWindow(GetDlgItem(IDX(IDC_MULTI_WORLD))->m_hWnd, 5));
        CWnd* customWorldButton = GetDlgItem(IDX(IDC_MULTI_CUSTOM_WORLD));
        CWnd* echoLatencyButton = GetDlgItem(IDX(IDC_MULTI_ECHO_LATENCY));
        if (!worldEdit) {
            return 0;
        }
        if (!worldCombo) {
            return 0;
        }
        if (!customWorldButton) {
            return 0;
        }
        if (!echoLatencyButton) {
            return 0;
        }
        i32 localSlot = GetLocalPlayerSlotIndex();
        i32 canEditWorld = (m_gameManager->m_options[localSlot].m_readyFlag == 0);
        worldCombo->EnableWindow(canEditWorld);
        customWorldButton->EnableWindow(canEditWorld);
        echoLatencyButton->EnableWindow(0);
        return 1;
    }
    CWnd* worldCombo = GetDlgItem(IDX(IDC_MULTI_WORLD));
    CWnd* worldEdit = CWnd::FromHandle(::GetWindow(GetDlgItem(IDX(IDC_MULTI_WORLD))->m_hWnd, 5));
    CWnd* customWorldButton = GetDlgItem(IDX(IDC_MULTI_CUSTOM_WORLD));
    CWnd* echoLatencyButton = GetDlgItem(IDX(IDC_MULTI_ECHO_LATENCY));
    if (!worldEdit) {
        return 0;
    }
    if (!worldCombo) {
        return 0;
    }
    if (!customWorldButton) {
        return 0;
    }
    if (!echoLatencyButton) {
        return 0;
    }
    ::SendMessageA(worldCombo->m_hWnd, CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    m_customWorldFlag = g_multiState->m_customLevel;
    if (m_customWorldFlag != 0) {
        worldEdit->SetWindowTextA(g_multiState->CustomLevelName());
    } else {
        CString currentName;
        worldEdit->GetWindowTextA(currentName);
        if (strcmp(currentName, g_multiState->BuiltInLevelName())) {
            worldEdit->SetWindowTextA(g_multiState->BuiltInLevelName());
        }
    }
    worldCombo->EnableWindow(0);
    customWorldButton->EnableWindow(0);
    echoLatencyButton->EnableWindow(0);
    return 1;
}

RVA(0x000c1e60, 0x115)
i32 CMultiStartDlg::BuildLatencyOptions() {
    m_latencyOptions = new CLatencyList(0xa);
    CMulti* multi = g_multiState;
    i32 presetCount = 5;
    CNetProviderNode* provider = multi->m_netMgr->m_selectedProvider;
    if (multi->m_lobbyLaunch) {
        presetCount = 2;
    } else if (provider) {
        if (provider->IsIpxProvider()) {
            presetCount = 1;
        }
        if (provider->IsTcpIpProvider()) {
            presetCount = 2;
        }
        if (provider->IsModemProvider()) {
            presetCount = 3;
        }
        if (provider->IsSerialProvider()) {
            presetCount = 4;
        }
    }
    m_latencyOptions->Dispatch(presetCount);
    HWND dialogHwnd = GetSafeHwnd();
    m_latencyOptions->FillCombo(dialogHwnd, IDX(IDC_MULTI_LATENCY));
    m_latencyOptions->SelectItem(dialogHwnd, IDX(IDC_MULTI_LATENCY), 0, 0);
    g_multiState->m_autoCommandDelay = 1;
    return 1;
}

// @early-stop
RVA(0x000c1fd0, 0x99)
i32 CMultiStartDlg::RefreshLatencyControl() {
    CWnd* latencyCombo = GetDlgItem(IDX(IDC_MULTI_LATENCY));
    if (latencyCombo == NULL) {
        return 0;
    }
    CMulti* multi = g_multiState;
    if (multi->m_isHost) {
        i32 localSlot = GetLocalPlayerSlotIndex();
        latencyCombo->EnableWindow(m_gameManager->m_options[localSlot].m_readyFlag == 0);
    } else {
        latencyCombo->EnableWindow(0);
    }
    HWND dialogHwnd = GetSafeHwnd();
    CMulti* currentMulti = g_multiState;
    if (currentMulti->m_autoCommandDelay) {
        m_latencyOptions->SelectItem(dialogHwnd, IDX(IDC_MULTI_LATENCY), 0, 0);
    } else {
        m_latencyOptions->SelectItem(
            dialogHwnd,
            IDX(IDC_MULTI_LATENCY),
            currentMulti->m_commandDelay,
            currentMulti->m_resendInterval
        );
    }
    return 1;
}

// @early-stop
RVA(0x000c20a0, 0x45a)
void CMultiStartDlg::DoDataExchange(CDataExchange* pDX) {
    Utils::RegistryHelper* reg = static_cast<Utils::RegistryHelper*>(g_gameReg->m_settings);
    if (pDX->m_bSaveAndValidate == 0) {
        GetDlgItem(IDX(IDC_MULTI_GAME_NAME))->SetWindowTextA(g_multiState->GameName());
        NetLobby::g_curDlg = GetSafeHwnd();
        if (!InitializeWorldCombo()) {
            return;
        }
        if (!BuildLatencyOptions()) {
            return;
        }
        WapSendMessageA sendMessage = ::SendMessageA;
        i32 i;

        MsgParam item;
        for (i = 0; i < NUM_PLAYER_SLOTS; i++) {
            HWND typeCombo;
            typeCombo = GetPlayerTypeControl(i)->m_hWnd;
            item.m_str = "None";
            sendMessage(typeCombo, CB_ADDSTRING, 0, item.m_lparam);
            typeCombo = GetPlayerTypeControl(i)->m_hWnd;
            item.m_str = "Computer (easy)";
            sendMessage(typeCombo, CB_ADDSTRING, 0, item.m_lparam);
            typeCombo = GetPlayerTypeControl(i)->m_hWnd;
            item.m_str = "Computer (normal)";
            sendMessage(typeCombo, CB_ADDSTRING, 0, item.m_lparam);
            typeCombo = GetPlayerTypeControl(i)->m_hWnd;
            item.m_str = "Computer (difficult)";
            sendMessage(typeCombo, CB_ADDSTRING, 0, item.m_lparam);
            typeCombo = GetPlayerTypeControl(i)->m_hWnd;
            item.m_str = "Human";
            sendMessage(typeCombo, CB_ADDSTRING, 0, item.m_lparam);
        }
        for (i = 0; i < NUM_PLAYER_SLOTS; i++) {
            CWnd* nameControl = GetPlayerNameControl(i);
            if (nameControl != NULL) {
                sendMessage(nameControl->m_hWnd, EM_LIMITTEXT, 9, 0);
            }
        }
        HWND chatEdit = GetDlgItem(IDX(IDC_MULTI_CHAT_INPUT))->m_hWnd;
        sendMessage(chatEdit, EM_LIMITTEXT, 100, 0);
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
                FILE* file = fopen(path, "rb");
                if (file != NULL) {
                    HWND worldCombo = GetDlgItem(IDX(IDC_MULTI_WORLD))->m_hWnd;
                    CWnd* child = CWnd::FromHandle(::GetWindow(worldCombo, GW_CHILD));
                    if (child == NULL) {
                        return;
                    }
                    child->SetWindowTextA(mapName);
                    g_multiState->m_customLevel = 1;
                    g_multiState->m_customLevelName = mapName;
                    g_multiState->m_builtInLevelName = "";
                    fclose(file);
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
            CWnd* chatLog = GetDlgItem(IDX(IDC_MULTI_CHAT_LOG));
            g_sharedFlag = (chatLog == NULL) ? NULL : chatLog->m_hWnd;
        }
        g_multiState->m_netMgr->m_selectedPlayer = NULL;
        g_multiState->PollSession();
        if (!RefreshWorldControls()) {
            return;
        }
        if (!RefreshLatencyControl()) {
            return;
        }
        if (!RefreshPlayerControls(1)) {
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
        GruntzPlayer* slots = m_gameManager->m_options;
        for (i32 i = 0; i < NUM_PLAYER_SLOTS; i++) {
            CWnd* nameControl = GetPlayerNameControl(i);
            if (nameControl != NULL) {
                CString name;
                nameControl->GetWindowTextA(name);
                slots[i].m_name = name;
            }
        }
        NetLobby::g_curDlg = NULL;
    }
    PaintPlayerColorControls();
}

RVA(0x000c2620, 0x6)
const AFX_MSGMAP* CMultiStartDlg::GetMessageMap() const {
    return &messageMap;
}

RVA(0x000c2640, 0x60)
CWnd* CMultiStartDlg::GetPlayerTypeControl(i32 slot) {
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

RVA(0x000c26c0, 0x60)
CWnd* CMultiStartDlg::GetReadyControl(i32 slot) {
    CWnd* result = NULL;
    switch (static_cast<PlayerSlot>(slot)) {
        case PLAYER_SLOT_0:
            result = GetDlgItem(CTRL_PLAYER_READY0);
            break;
        case PLAYER_SLOT_1:
            result = GetDlgItem(CTRL_PLAYER_READY1);
            break;
        case PLAYER_SLOT_2:
            result = GetDlgItem(CTRL_PLAYER_READY2);
            break;
        case PLAYER_SLOT_3:
            result = GetDlgItem(CTRL_PLAYER_READY3);
            break;
    }
    return result;
}

RVA(0x000c2740, 0x60)
CWnd* CMultiStartDlg::GetPlayerNameControl(i32 slot) {
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

RVA(0x000c27c0, 0x60)
CWnd* CMultiStartDlg::GetMaxGruntzControl(i32 slot) {
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

RVA(0x000c2840, 0x60)
CWnd* CMultiStartDlg::GetPlayerColorControl(i32 slot) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c28c0, 0x27)
void CMultiStartDlg::SetPlayerTypeSelection(i32 slot, i32 selection) {
    CWnd* control = GetPlayerTypeControl(slot);
    if (control != NULL) {
        ::SendMessageA(control->m_hWnd, CB_SETCURSEL, selection, 0);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c2900, 0x2a)
i32 CMultiStartDlg::GetPlayerTypeSelection(i32 slot) {
    CWnd* control = GetPlayerTypeControl(slot);
    if (control == NULL) {
        return -1;
    }
    return ::SendMessageA(control->m_hWnd, CB_GETCURSEL, 0, 0);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c2940, 0x2b)
i32 CMultiStartDlg::GetMaxGruntzSelection(i32 slot) {
    CWnd* control = GetMaxGruntzControl(slot);
    if (control == NULL) {
        return -1;
    }
    return ::SendMessageA(control->m_hWnd, CB_GETCURSEL, 0, 0) + 1;
}

RVA(0x000c2980, 0x28)
void CMultiStartDlg::SetMaxGruntzSelection(i32 slot, i32 count) {
    CWnd* control = GetMaxGruntzControl(slot);
    if (control) {
        ::SendMessageA(control->m_hWnd, CB_SETCURSEL, count - 1, 0);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c29c0, 0x1d)
void CMultiStartDlg::SetPlayerName(i32 slot, const char* name) {
    CWnd* control = GetPlayerNameControl(slot);
    if (control != NULL) {
        control->SetWindowTextA(name);
    }
}

RVA(0x000c29f0, 0x13)
void CMultiStartDlg::OnPlayerTypeSelection0() {
    ApplyPlayerTypeSelection(0);
    BroadcastPlayerSlotChanges();
}

RVA(0x000c2a20, 0x13)
void CMultiStartDlg::OnPlayerTypeSelection1() {
    ApplyPlayerTypeSelection(1);
    BroadcastPlayerSlotChanges();
}

RVA(0x000c2a50, 0x13)
void CMultiStartDlg::OnPlayerTypeSelection2() {
    ApplyPlayerTypeSelection(2);
    BroadcastPlayerSlotChanges();
}

RVA(0x000c2a80, 0x13)
void CMultiStartDlg::OnPlayerTypeSelection3() {
    ApplyPlayerTypeSelection(3);
    BroadcastPlayerSlotChanges();
}

RVA(0x000c2ab0, 0x161)
void CMultiStartDlg::ApplyPlayerTypeSelection(i32 slot) {
    CWnd* typeControl = GetPlayerTypeControl(slot);
    CWnd* nameControl = GetPlayerNameControl(slot);
    CWnd* colorControl = GetPlayerColorControl(slot);
    GetMaxGruntzControl(slot);
    GetReadyControl(slot);
    GruntzPlayer* player = &m_gameManager->m_options[slot];
    LRESULT(WINAPI * sendMessage)(HWND, UINT, WPARAM, LPARAM) = ::SendMessageA;
    if (sendMessage(typeControl->m_hWnd, CB_GETCURSEL, 0, 0) == 0) {
        if (player->m_humanControlled && player->m_liveGate) {
            g_multiState->DropChannelPlayer(player->m_playerIndex);
        } else if (!player->m_humanControlled && player->m_liveGate) {
            ChannelSlots_Set(IDX(player->m_colorIndex), 1);
        }
        player->m_liveGate = 0;
        player->m_readyFlag = 0;
        nameControl->EnableWindow(0);
        colorControl->EnableWindow(0);
    } else {
        if (static_cast<MultiplayerPlayerKind>(sendMessage(typeControl->m_hWnd, CB_GETCURSEL, 0, 0))
            != MULTI_PLAYER_HUMAN) {
            if (player->m_humanControlled != 0) {
                if (player->m_liveGate != 0) {
                    g_multiState->DropChannelPlayer(player->m_playerIndex);
                }
                i32 freeColor = ChannelSlots_FindFree();
                player->m_colorIndex = static_cast<ColorTint>(freeColor);
                ChannelSlots_Set(freeColor, 0);
            } else if (player->m_liveGate == 0) {
                i32 freeColor = ChannelSlots_FindFree();
                player->m_colorIndex = static_cast<ColorTint>(freeColor);
                ChannelSlots_Set(freeColor, 0);
            }
            player->m_readyFlag = 1;
            player->m_humanControlled = 0;
            player->m_configId =
                static_cast<i32>(sendMessage(typeControl->m_hWnd, CB_GETCURSEL, 0, 0)) - 1;
            player->m_liveGate = 1;
            player->m_name = g_defaultPlayerNames[slot];
        }
        nameControl->EnableWindow(1);
        colorControl->EnableWindow(1);
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
    ::SetTimer(m_hWnd, MULTI_START_WATCHDOG_TIMER, 0x32, NULL);
    return 1;
}

RVA(0x000c2ce0, 0xf3)
void CMultiStartDlg::AppendChatLine(char* line) {
    CWnd* item = GetDlgItem(IDX(IDC_MULTI_CHAT_LOG));
    HWND edit;
    if (!item) {
        edit = NULL;
    } else {
        edit = item->m_hWnd;
    }
    if (!edit || !line || !line[0]) {
        return;
    }
    i32 len = ::GetWindowTextLengthA(edit);
    if (len == 0) {
        ::SendMessageA(edit, EM_SETSEL, len, -1);
    } else {
        ::SendMessageA(edit, EM_SETSEL, len, len);
    }
    char buffer[0x80];
    buffer[0] = 0;
    if (len > 0) {
        strcat(buffer, "\r\n");
    }
    strcat(buffer, line);
    MsgParam text;
    text.m_str = buffer;
    ::SendMessageA(edit, EM_REPLACESEL, 0, text.m_lparam);
    ::SendMessageA(edit, EM_LINESCROLL, 0, 0x270f);
}

RVA(0x000c2e20, 0x21d)
i32 CMultiStartDlg::PaintPlayerColorControls() {
    CPaintDC dc(this);
    BOOL(WINAPI * clientToScreen)(HWND, LPPOINT) = ::ClientToScreen;
    BOOL(WINAPI * screenToClient)(HWND, LPPOINT) = ::ScreenToClient;
    for (i32 i = 0; i < 4; i++) {
        CWnd* colorControl = GetPlayerColorControl(i);
        if (colorControl == NULL) {
            continue;
        }

        CRect rect;
        ::GetClientRect(colorControl->m_hWnd, &rect);
        clientToScreen(colorControl->m_hWnd, &rect.TopLeft());
        clientToScreen(colorControl->m_hWnd, &rect.BottomRight());
        screenToClient(m_hWnd, &rect.TopLeft());
        screenToClient(m_hWnd, &rect.BottomRight());
        CBrush brush;
        // Two Attach sites, not one hoisted `color`: retail pushes the argument
        // INSIDE each arm and cross-jumps only the shared `call CreateSolidBrush`.
        if (colorControl->IsWindowEnabled()) {
            GetRandomNumber();
            GetRandomNumber();
            i32 shade = (GetRandomNumber() % 0xff) & 0xff;
            brush.Attach(CreateSolidBrush((shade << 8 | shade) << 8 | shade));
        } else {
            brush.Attach(CreateSolidBrush(0x808080));
        }
        FillRect(dc.m_hDC, &rect, brush);
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
    i32 shouldDraw = 0;
    switch (nIDCtl) {
        case CTRL_PLAYER_COLOR0:
            if (GetPlayerColorControl(0)->IsWindowEnabled()) {
                switch (m_gameManager->m_options[0].m_colorIndex) {
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
            shouldDraw = 1;
            break;
        case CTRL_PLAYER_COLOR1:
            if (GetPlayerColorControl(1)->IsWindowEnabled()) {
                switch (m_gameManager->m_options[1].m_colorIndex) {
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
            shouldDraw = 1;
            break;
        case CTRL_PLAYER_COLOR2:
            if (GetPlayerColorControl(2)->IsWindowEnabled()) {
                switch (m_gameManager->m_options[2].m_colorIndex) {
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
            shouldDraw = 1;
            break;
        case CTRL_PLAYER_COLOR3:
            if (GetPlayerColorControl(3)->IsWindowEnabled()) {
                switch (m_gameManager->m_options[3].m_colorIndex) {
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
            shouldDraw = 1;
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

RVA(0x000c3830, 0xd1)
void CMultiStartDlg::OnPlayerColor0() {
    CMulti* multi = g_multiState;
    if ((multi->m_isHost == 0 || m_gameManager->m_options[0].m_humanControlled != 0)
        && (m_gameManager->m_options[0].m_readyFlag != 0
            || m_gameManager->m_options[0].m_slotKey != multi->m_localPlayerId)) {
        return;
    }
    CBattlezDlgColors colorDialog(m_gameManager, 0, 1, NULL);
    if (colorDialog.DoModal() == 1) {
        if (SetPlayerColor(0, static_cast<ColorTint>(colorDialog.m_pickedColor))) {
            BroadcastPlayerSlotChanges();
            GetDlgItem(CTRL_PLAYER_COLOR0)->InvalidateRect(NULL, 1);
        }
    }
}

RVA(0x000c3950, 0xd1)
void CMultiStartDlg::OnPlayerColor1() {
    CMulti* multi = g_multiState;
    if ((multi->m_isHost == 0 || m_gameManager->m_options[1].m_humanControlled != 0)
        && (m_gameManager->m_options[1].m_readyFlag != 0
            || m_gameManager->m_options[1].m_slotKey != multi->m_localPlayerId)) {
        return;
    }
    CBattlezDlgColors colorDialog(m_gameManager, 1, 1, NULL);
    if (colorDialog.DoModal() == 1) {
        if (SetPlayerColor(1, static_cast<ColorTint>(colorDialog.m_pickedColor))) {
            BroadcastPlayerSlotChanges();
            GetDlgItem(CTRL_PLAYER_COLOR1)->InvalidateRect(NULL, 1);
        }
    }
}

RVA(0x000c3a70, 0xd1)
void CMultiStartDlg::OnPlayerColor2() {
    CMulti* multi = g_multiState;
    if ((multi->m_isHost == 0 || m_gameManager->m_options[2].m_humanControlled != 0)
        && (m_gameManager->m_options[2].m_readyFlag != 0
            || m_gameManager->m_options[2].m_slotKey != multi->m_localPlayerId)) {
        return;
    }
    CBattlezDlgColors colorDialog(m_gameManager, 2, 1, NULL);
    if (colorDialog.DoModal() == 1) {
        if (SetPlayerColor(2, static_cast<ColorTint>(colorDialog.m_pickedColor))) {
            BroadcastPlayerSlotChanges();
            GetDlgItem(CTRL_PLAYER_COLOR2)->InvalidateRect(NULL, 1);
        }
    }
}

RVA(0x000c3b90, 0xd1)
void CMultiStartDlg::OnPlayerColor3() {
    CMulti* multi = g_multiState;
    if ((multi->m_isHost == 0 || m_gameManager->m_options[3].m_humanControlled != 0)
        && (m_gameManager->m_options[3].m_readyFlag != 0
            || m_gameManager->m_options[3].m_slotKey != multi->m_localPlayerId)) {
        return;
    }
    CBattlezDlgColors colorDialog(m_gameManager, 3, 1, NULL);
    if (colorDialog.DoModal() == 1) {
        if (SetPlayerColor(3, static_cast<ColorTint>(colorDialog.m_pickedColor))) {
            BroadcastPlayerSlotChanges();
            GetDlgItem(CTRL_PLAYER_COLOR3)->InvalidateRect(NULL, 1);
        }
    }
}

RVA(0x000c3cb0, 0x128)
void CMultiStartDlg::OnCustomWorld() {
    if (g_multiState->m_isHost == 0) {
        return;
    }
    CBattlezDlgCustom dlg(NULL);
    if (dlg.DoModal() == 1 && dlg.m_customName.GetLength() != 0) {

        CWnd* worldCombo = GetDlgItem(IDX(IDC_MULTI_WORLD));
        CWnd* worldEdit = CWnd::FromHandle(::GetWindow(worldCombo->m_hWnd, GW_CHILD));

        if (worldEdit == NULL) {
            return;
        }
        dlg.m_customName.MakeUpper();
        worldEdit->SetWindowTextA(static_cast<LPCTSTR>(dlg.m_customName));
        m_customWorldFlag = 1;
        g_multiState->m_customLevel = 1;
        g_multiState->m_customLevelName = static_cast<LPCTSTR>(dlg.m_customName);
        g_multiState->m_builtInLevelName = "";
        g_multiState->SaveConfig(NULL);
    }
}

RVA(0x000c3e30, 0xfe)
void CMultiStartDlg::CommitWorldSelection() {
    if (g_multiState->m_isHost != 0) {
        CWnd* worldCombo = GetDlgItem(IDX(IDC_MULTI_WORLD));
        if (worldCombo != NULL) {
            i32 selection = ::SendMessageA(worldCombo->m_hWnd, CB_GETCURSEL, 0, 0);
            if (selection != -1) {
                CString worldName;
                (static_cast<CComboBox*>(worldCombo))->GetLBText(selection, worldName);
                if (worldName.GetLength() != 0) {
                    m_customWorldFlag = 0;
                }
                g_multiState->m_customLevel = 0;
                g_multiState->m_customLevelName = "";
                g_multiState->m_builtInLevelName = static_cast<LPCTSTR>(worldName);
                g_multiState->SaveConfig(NULL);
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
    CString message, inputText;
    GetPlayerNameControl(GetLocalPlayerSlotIndex())->GetWindowTextA(message);
    message += " says: ";
    input->GetWindowTextA(inputText);
    if (inputText.GetLength() != 0) {
        message += inputText;
        AppendChatLine(const_cast<char*>(static_cast<const char*>(message)));
        input->SetWindowTextA("");
        g_multiState
            ->BroadcastChatLine(const_cast<char*>(static_cast<const char*>(message)), 0, 0, NULL);
    }
}

RVA(0x000c40b0, 0x42)
void CMultiStartDlg::BroadcastPlayerSlotChanges() {
    CMulti* multi = g_multiState;
    if (multi->m_isHost != 0) {
        multi->BroadcastChannelTable(NULL);
        RefreshPlayerControls(1);
    } else {
        g_multiState->BroadcastOneChannel(m_gameManager->FindOptionsSlot(multi->m_localPlayerId));
    }
}

RVA(0x000c4120, 0xc2)
i32 CMultiStartDlg::EnableChatControls() {
    CWnd* control = GetDlgItem(IDCANCEL);
    control->EnableWindow(1);
    control = GetDlgItem(IDX(IDC_NETCHAT_SEND));
    control->EnableWindow(1);
    control = GetDlgItem(IDX(IDC_MULTI_CHAT_INPUT));
    control->EnableWindow(1);
    control = GetDlgItem(IDX(IDC_MULTI_CHAT_LOG));
    control->EnableWindow(1);
    CString s1;
    if (g_multiState->m_customLevel == 0) {
        CString s2;
    }
    return 1;
}

// @early-stop
RVA(0x000c4230, 0x38e)
i32 CMultiStartDlg::RefreshPlayerControls(i32 force) {
    CWnd::FromHandle(::GetFocus());
    i32 allLivePlayersReady = 1;
    i32 hasRemoteHumanPlayer = 0;
    i32 localSlotIndex = this->GetLocalPlayerSlotIndex();
    i32 localReadyFlag =
        g_multiState->m_isHost ? m_gameManager->m_options[localSlotIndex].m_readyFlag : 1;
    for (i32 slotIndex = 0; slotIndex < 4; slotIndex++) {
        GruntzPlayer* player = &g_gameReg->m_options[slotIndex];
        if (player) {
            if (player->m_slotKey != g_multiState->m_localPlayerId && player->m_humanControlled
                && player->m_liveGate) {
                hasRemoteHumanPlayer = 1;
            }
            CWnd* nameControl = GetPlayerNameControl(slotIndex);
            if ((g_multiState->m_isHost && player->m_humanControlled == 0)
                || player->m_slotKey == g_multiState->m_localPlayerId) {
                nameControl->EnableWindow(1);
            } else {
                nameControl->EnableWindow(0);
            }
            CWnd* typeControl = GetPlayerTypeControl(slotIndex);
            if (g_multiState->m_isHost && localReadyFlag == 0
                && player->m_slotKey != g_multiState->m_localPlayerId) {
                typeControl->EnableWindow(1);
            } else {
                typeControl->EnableWindow(0);
            }
            CWnd* readyControl = GetReadyControl(slotIndex);
            if (player->m_slotKey == g_multiState->m_localPlayerId) {
                readyControl->EnableWindow(1);
            } else {
                readyControl->EnableWindow(0);
            }
            if (player->m_readyFlag == 0) {
                if (player->m_liveGate) {
                    ::SendMessageA(readyControl->m_hWnd, BM_SETCHECK, 0, 0);
                    allLivePlayersReady = 0;
                } else {
                    ::SendMessageA(readyControl->m_hWnd, BM_SETCHECK, 0, 0);
                }
            } else if (player->m_liveGate) {
                ::SendMessageA(readyControl->m_hWnd, BM_SETCHECK, 1, 0);
            } else {
                ::SendMessageA(readyControl->m_hWnd, BM_SETCHECK, 0, 0);
            }
            CWnd* maxGruntzControl = GetMaxGruntzControl(slotIndex);
            // The &&-chain as the ARGUMENT, not an if/else round two calls: retail
            // materialises the flag (mov edx,1 / jmp / xor edx,edx) and pushes it once.
            maxGruntzControl->EnableWindow(
                g_multiState->m_isHost && player->m_liveGate && localReadyFlag == 0
            );
            SetMaxGruntzSelection(slotIndex, player->m_liveGate ? player->m_comboSel : 0);
            if (force == 0) {
                if (this->GetLocalPlayerSlotIndex() == slotIndex) {
                    continue;
                }
                if (g_multiState->m_isHost && player->m_humanControlled == 0) {
                    continue;
                }
            }
            if (player->m_liveGate) {
                {
                    force = 0;
                    GetPlayerNameControl(slotIndex)->SetWindowTextA(player->GetName());
                }
                // The combo is fetched into a local FIRST at each of the three sites -
                // retail calls GetPlayerTypeControl inside both arms and only then pushes the
                // message args. Written inline the constants push first, which lets cl
                // cross-jump the two arms onto ONE GetPlayerTypeControl call (retail has two).
                if (player->m_humanControlled) {
                    CWnd* typeCombo = GetPlayerTypeControl(slotIndex);
                    ::SendMessageA(typeCombo->m_hWnd, CB_SETCURSEL, 4, 0);
                } else {
                    i32 selection = player->m_configId;
                    CWnd* typeCombo = GetPlayerTypeControl(slotIndex);
                    ::SendMessageA(typeCombo->m_hWnd, CB_SETCURSEL, selection + 1, 0);
                }
            } else {
                GetPlayerNameControl(slotIndex)->SetWindowTextA("");
                CWnd* typeCombo = GetPlayerTypeControl(slotIndex);
                ::SendMessageA(typeCombo->m_hWnd, CB_SETCURSEL, 0, 0);
            }
            this->ApplyPlayerTypeSelection(slotIndex);
        }
    }
    if (g_multiState->m_isHost) {
        CWnd* ok = this->GetDlgItem(1);
        if (ok == NULL) {
            return 0;
        }
        ok->EnableWindow(hasRemoteHumanPlayer & allLivePlayersReady);
    }
    HWND color0 = this->GetDlgItem(CTRL_PLAYER_COLOR0)->m_hWnd;
    ::InvalidateRect(color0, NULL, 1);
    HWND color1 = this->GetDlgItem(CTRL_PLAYER_COLOR1)->m_hWnd;
    ::InvalidateRect(color1, NULL, 1);
    HWND color2 = this->GetDlgItem(CTRL_PLAYER_COLOR2)->m_hWnd;
    ::InvalidateRect(color2, NULL, 1);
    HWND color3 = this->GetDlgItem(CTRL_PLAYER_COLOR3)->m_hWnd;
    ::InvalidateRect(color3, NULL, 1);
    return 1;
}

// @early-stop
RVA(0x000c46b0, 0x384)
void CMultiStartDlg::Watchdog() {
    if (g_watchdogBusy != 0) {
        return;
    }
    g_watchdogBusy = 1;
    CNetSessionListNode* session = g_multiState->m_netMgr->m_selectedSession;
    if (session == NULL) {
        return;
    }
    g_multiState->m_netMgr->EnumerateSessionPlayers(session, 0);
    g_multiState->ResolveLocalPlayer();
    if (g_netStatsTick == 0) {
        u32 timestamp = timeGetTime();
        g_multiState->SendNetStat(NETMSG_STAT_REQUEST, static_cast<i32>(timestamp), 0);
    }
    if (g_multiState->m_isHost == 0) {
        if (g_netStatsTick == 0) {
            g_multiState->ReportAckLatency();
        }
        EnableWindow(0);
        i32 verificationResult =
            g_multiState->VerifyCustomLevel(session, g_multiState->m_localPlayer);
        EnableWindow(1);
        if (verificationResult != 0) {
            EndDialog(1);
            g_watchdogBusy = 0;
            return;
        }
    } else {
        g_multiState->PollSession();
        if (g_multiState->m_autoCommandDelay != 0) {
            g_multiState->AutoTuneCmdDelay();
        }
    }
    i32 nextNetStatsTick = g_netStatsTick + 1;
    g_netStatsTick = nextNetStatsTick;
    if (nextNetStatsTick > 3) {
        g_netStatsTick = 0;
    }
    if (g_latencyDisplayTick == 0) {
        for (i32 i = 0; i < 4; i++) {
            GruntzPlayer* player = &g_gameReg->m_options[i];
            CWnd* latencyValueControl;
            CWnd* latencyUnitControl;
            switch (static_cast<PlayerSlot>(i)) {
                case PLAYER_SLOT_0:
                    latencyValueControl = GetDlgItem(CTRL_PLAYER_LATENCY_VALUE0);
                    latencyUnitControl = GetDlgItem(CTRL_PLAYER_LATENCY_UNIT0);
                    break;
                case PLAYER_SLOT_1:
                    latencyValueControl = GetDlgItem(CTRL_PLAYER_LATENCY_VALUE1);
                    latencyUnitControl = GetDlgItem(CTRL_PLAYER_LATENCY_UNIT1);
                    break;
                case PLAYER_SLOT_2:
                    latencyValueControl = GetDlgItem(CTRL_PLAYER_LATENCY_VALUE2);
                    latencyUnitControl = GetDlgItem(CTRL_PLAYER_LATENCY_UNIT2);
                    break;
                case PLAYER_SLOT_3:
                    latencyValueControl = GetDlgItem(CTRL_PLAYER_LATENCY_VALUE3);
                    latencyUnitControl = GetDlgItem(CTRL_PLAYER_LATENCY_UNIT3);
                    break;
            }
            if (player->m_liveGate != 0 && player->m_humanControlled != 0) {
                char latencyText[0x20];
                wsprintfA(latencyText, "%d", player->m_latency.m_avg);
                latencyValueControl->SetWindowTextA(latencyText);
                latencyUnitControl->SetWindowTextA("ms");
            } else {
                latencyValueControl->SetWindowTextA("");
                latencyUnitControl->SetWindowTextA("");
            }
        }
    }
    i32 nextLatencyDisplayTick = g_latencyDisplayTick + 1;
    g_latencyDisplayTick = nextLatencyDisplayTick;
    if (nextLatencyDisplayTick > 0x31) {
        g_latencyDisplayTick = 0;
    }
    if (g_multiState->m_sessionTerminated != 0) {
        ::KillTimer(m_hWnd, 1);
        g_multiState->ReportVersionMsg("The game session has been terminated.", 0);
        g_watchdogBusy = 0;
        return;
    }
    if (g_multiState->m_colorSelectionRejected != 0) {
        g_multiState->m_colorSelectionRejected = 0;
        g_multiState->ReportVersionMsg("Someone has already selected that color.", 0);
        g_watchdogBusy = 0;
        return;
    }
    char* errorMessage;
    if (g_multiState->m_removedByHost != 0) {
        ::KillTimer(m_hWnd, 1);
        errorMessage = "You have been removed from the game by the host.";
    } else if (g_multiState->m_gameClosed != 0) {
        ::KillTimer(m_hWnd, 1);
        errorMessage = "This game is closed.";
    } else if (g_multiState->m_gameFull != 0) {
        ::KillTimer(m_hWnd, 1);
        errorMessage = "This game is already full.";
    } else if (g_multiState->m_versionMismatch != 0) {
        ::KillTimer(m_hWnd, 1);
        errorMessage = "This version is not the same as the host computer's version of the game.";
    } else {
        if (g_playerLeftFlag != 0) {
            RefreshPlayerControls(1);
            EnableChatControls();
            RefreshWorldControls();
            RefreshLatencyControl();
            g_playerLeftFlag = 0;
        }
        if (g_multiState->m_connectAccepted != 0) {
            EnableChatControls();
            RefreshWorldControls();
            RefreshLatencyControl();
            g_multiState->m_connectAccepted = 0;
        }
        g_watchdogBusy = 0;
        return;
    }
    g_multiState->ReportVersionMsg(errorMessage, 0);
    EndDialog(0);
    g_watchdogBusy = 0;
}

RVA(0x000c4b30, 0x1f)
i32 CMultiStartDlg::GetLocalPlayerSlotIndex() {
    GruntzPlayer* slot = m_gameManager->FindOptionsSlot(g_multiState->m_localPlayerId);
    if (slot == NULL) {
        return -1;
    }
    return slot->m_playerIndex;
}

RVA(0x000c4b60, 0x77)
i32 CMultiStartDlg::SetPlayerColor(i32 slot, ColorTint color) {
    GruntzPlayer* player = &m_gameManager->m_options[slot];
    if (g_multiState->m_isHost != 0) {
        i32 available = ChannelSlots_Get(IDX(color));
        if (available == 0) {
            g_multiState->ReportVersionMsg("Someone has already selected that color.", available);
            return 0;
        }
        ChannelSlots_Set(IDX(player->m_colorIndex), 1);
        ChannelSlots_Set(IDX(color), 0);
    }
    player->m_colorIndex = color;
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
    i32 customLevel = g_multiState->m_customLevel;
    i32 verificationToken = g_gameReg->ResolveLevelChecksum(
        0,
        0,
        customLevel,
        0,
        customLevel != 0 ? g_multiState->CustomLevelName() : g_multiState->BuiltInLevelName()
    );
    g_multiState->m_levelVerifyResult = 0;
    if (g_multiState->Poll(verificationToken) == 0) {
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
    return m_resendInterval;
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
void CMultiStartDlg::OnMaxGruntzSelection0() {
    HWND comboHwnd = GetMaxGruntzControl(0)->m_hWnd;
    g_gameReg->m_options[0].m_comboSel = ::SendMessageA(comboHwnd, CB_GETCURSEL, 0, 0) + 1;
    BroadcastPlayerSlotChanges();
}

RVA(0x000c4f30, 0x33)
void CMultiStartDlg::OnMaxGruntzSelection1() {
    HWND comboHwnd = GetMaxGruntzControl(1)->m_hWnd;
    g_gameReg->m_options[1].m_comboSel = ::SendMessageA(comboHwnd, CB_GETCURSEL, 0, 0) + 1;
    BroadcastPlayerSlotChanges();
}

RVA(0x000c4f80, 0x33)
void CMultiStartDlg::OnMaxGruntzSelection2() {
    HWND comboHwnd = GetMaxGruntzControl(2)->m_hWnd;
    g_gameReg->m_options[2].m_comboSel = ::SendMessageA(comboHwnd, CB_GETCURSEL, 0, 0) + 1;
    BroadcastPlayerSlotChanges();
}

RVA(0x000c4fd0, 0x33)
void CMultiStartDlg::OnMaxGruntzSelection3() {
    HWND comboHwnd = GetMaxGruntzControl(3)->m_hWnd;
    g_gameReg->m_options[3].m_comboSel = ::SendMessageA(comboHwnd, CB_GETCURSEL, 0, 0) + 1;
    BroadcastPlayerSlotChanges();
}

RVA(0x000c5020, 0x95)
void CMultiStartDlg::CommitLatencySelection() {
    if (g_multiState->m_isHost == 0) {
        return;
    }
    i32 commandDelay, resendInterval;
    HWND dialogHwnd = GetSafeHwnd();
    m_latencyOptions
        ->GetSelItemData(dialogHwnd, IDX(IDC_MULTI_LATENCY), &commandDelay, &resendInterval);
    if (commandDelay != 0 || resendInterval != 0) {
        g_multiState->m_commandDelay = commandDelay;
        g_multiState->m_resendInterval = resendInterval;
        g_multiState->m_autoCommandDelay = 0;
        g_multiState->SaveConfig(NULL);
    } else {
        g_multiState->m_autoCommandDelay = 1;
    }
}

RVA(0x000c50f0, 0x9b)
void CMultiStartDlg::CommitReadySelection(i32 slotIndex) {
    CWnd* readyControl = GetReadyControl(slotIndex);
    if (!readyControl) {
        return;
    }
    i32 checked = ::SendMessageA(readyControl->m_hWnd, BM_GETCHECK, 0, 0);
    GruntzPlayer* player = &g_gameReg->m_options[slotIndex];
    if (!player) {
        return;
    }
    if (checked) {
        player->m_readyFlag = 1;
    } else {
        player->m_readyFlag = 0;
    }
    if (g_multiState->m_isHost) {
        g_multiState->BroadcastChannelTable(NULL);
        RefreshPlayerControls(1);
        EnableChatControls();
        RefreshWorldControls();
        RefreshLatencyControl();
    } else {
        g_multiState->BroadcastOneChannel(player);
    }
}

RVA(0x000c51c0, 0x8)
void CMultiStartDlg::OnReadyToggle0() {
    CommitReadySelection(0);
}

RVA(0x000c51e0, 0x8)
void CMultiStartDlg::OnReadyToggle1() {
    CommitReadySelection(1);
}

RVA(0x000c5200, 0x8)
void CMultiStartDlg::OnReadyToggle2() {
    CommitReadySelection(2);
}

RVA(0x000c5220, 0x8)
void CMultiStartDlg::OnReadyToggle3() {
    CommitReadySelection(3);
}

RVA(0x000c5240, 0x2c)
i32 CMultiStartDlg::DestroyWindow() {
    CLatencyList* latencyOptions = m_latencyOptions;
    if (latencyOptions) {
        latencyOptions->CKeyedList::~CKeyedList();
        ::operator delete(latencyOptions);
        m_latencyOptions = NULL;
    }
    return CWnd::DestroyWindow();
}

RVA(0x000c52f0, 0x43)
void CMultiStartDlg::EchoLatencySettings() {
    char message[128];
    wsprintfA(
        message,
        s_UsingCmdDelay,
        g_multiState->m_commandDelay,
        g_multiState->m_resendInterval
    );
    AppendChatLine(message);
}
