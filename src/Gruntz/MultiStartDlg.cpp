#include <rva.h>

#include <Gruntz/MultiStartDlg.h>

#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CustomMapSelection.h>
#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/Multi.h>
#include <Gruntz/MultiStartDlgCtrlId.h>
#include <Ints.h>
#include <MsgParam.h>
#include <Net/KeyedList.h>
#include <Net/LatencyList.h>
#include <Net/NetLobbyCtrlId.h>
#include <Net/NetMgr.h>
#include <Net/NetProviderNode.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>
#include <Utils/RegistryHelper.h>

#include <stdio.h>
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
b32 g_watchdogBusy;
DATA(0x0024bdc8)
i32 g_netStatsTick;
DATA(0x0024bdcc)
i32 g_latencyDisplayTick;

DATA(0x0021243c)
char s_UsingCmdDelay[] = "Using CmdDelay of %d and ResendDelay of %d.";

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
    m_customMapSelection = CUSTOM_MAP_STANDARD;
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
        combo->SendMessageA(
            CB_ADDSTRING,
            0,
            (text.m_str = static_cast<LPCTSTR>(name), text.m_lparam)
        );
        entry = worlds->NextEntry(entry);
    }
    CWnd* reloadedCombo = GetDlgItem(IDX(IDC_MULTI_WORLD));
    CWnd* editControl = reloadedCombo->GetWindow(GW_CHILD);
    if (editControl == NULL) {
        return 0;
    }
    editControl->SendMessageA(EM_SETREADONLY, 1, 0);
    combo->SendMessageA(CB_SETCURSEL, 0, 0);
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
    if (g_multiState->m_isHost != false) {
        CWnd* worldCombo = GetDlgItem(IDX(IDC_MULTI_WORLD));
        CWnd* worldEdit = GetDlgItem(IDX(IDC_MULTI_WORLD))->GetWindow(GW_CHILD);
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
        b32 canEditWorld = (m_gameManager->m_players[localSlot].m_ready == false);
        worldCombo->EnableWindow(canEditWorld);
        customWorldButton->EnableWindow(canEditWorld);
        echoLatencyButton->EnableWindow(false);
        return 1;
    }
    CWnd* worldCombo = GetDlgItem(IDX(IDC_MULTI_WORLD));
    CWnd* worldEdit = GetDlgItem(IDX(IDC_MULTI_WORLD))->GetWindow(GW_CHILD);
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
    worldCombo->SendMessageA(CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    m_customMapSelection =
        g_multiState->m_usesCustomLevel != false ? CUSTOM_MAP_SELECTED : CUSTOM_MAP_STANDARD;
    if (m_customMapSelection != CUSTOM_MAP_STANDARD) {
        worldEdit->SetWindowTextA(g_multiState->CustomLevelName());
    } else {
        CString currentName;
        worldEdit->GetWindowTextA(currentName);
        if (strcmp(currentName, g_multiState->BuiltInLevelName())) {
            worldEdit->SetWindowTextA(g_multiState->BuiltInLevelName());
        }
    }
    worldCombo->EnableWindow(false);
    customWorldButton->EnableWindow(false);
    echoLatencyButton->EnableWindow(false);
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
    g_multiState->m_autoCommandDelay = true;
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
        latencyCombo->EnableWindow(m_gameManager->m_players[localSlot].m_ready == false);
    } else {
        latencyCombo->EnableWindow(false);
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
    if (pDX->m_bSaveAndValidate == false) {
        GetDlgItem(IDX(IDC_MULTI_GAME_NAME))->SetWindowTextA(g_multiState->GameName());
        NetLobby::g_curDlg = GetSafeHwnd();
        if (!InitializeWorldCombo()) {
            return;
        }
        if (!BuildLatencyOptions()) {
            return;
        }
        i32 i;

        MsgParam item;
        for (i = 0; i < NUM_PLAYER_SLOTS; i++) {
            CWnd* typeCombo = GetPlayerTypeControl(i);
            item.m_str = "None";
            typeCombo->SendMessageA(CB_ADDSTRING, 0, item.m_lparam);
            typeCombo = GetPlayerTypeControl(i);
            item.m_str = "Computer (easy)";
            typeCombo->SendMessageA(CB_ADDSTRING, 0, item.m_lparam);
            typeCombo = GetPlayerTypeControl(i);
            item.m_str = "Computer (normal)";
            typeCombo->SendMessageA(CB_ADDSTRING, 0, item.m_lparam);
            typeCombo = GetPlayerTypeControl(i);
            item.m_str = "Computer (difficult)";
            typeCombo->SendMessageA(CB_ADDSTRING, 0, item.m_lparam);
            typeCombo = GetPlayerTypeControl(i);
            item.m_str = "Human";
            typeCombo->SendMessageA(CB_ADDSTRING, 0, item.m_lparam);
        }
        for (i = 0; i < NUM_PLAYER_SLOTS; i++) {
            CWnd* nameControl = GetPlayerNameControl(i);
            if (nameControl != NULL) {
                nameControl->SendMessageA(EM_LIMITTEXT, 9, 0);
            }
        }
        GetDlgItem(IDX(IDC_MULTI_CHAT_INPUT))->SendMessageA(EM_LIMITTEXT, 100, 0);
        CustomMapSelection customFlag = static_cast<CustomMapSelection>(
            reg->GetValueDword("CustomMultiMap", IDX(CUSTOM_MAP_UNINITIALIZED))
        );
        if (g_multiState->m_isHost != false && customFlag != CUSTOM_MAP_UNINITIALIZED) {
            char mapName[0x100];
            DWORD size = 0x100;
            reg->GetValueString("LastMultiMap", mapName, &size, "");
            m_customMapSelection = customFlag;
            if (customFlag != CUSTOM_MAP_STANDARD) {
                char path[0x100];
                sprintf(path, "custom\\%s", mapName);
                FILE* file = fopen(path, "rb");
                if (file != NULL) {
                    CWnd* worldCombo = GetDlgItem(IDX(IDC_MULTI_WORLD));
                    CWnd* child = worldCombo->GetWindow(GW_CHILD);
                    if (child == NULL) {
                        return;
                    }
                    child->SetWindowTextA(mapName);
                    g_multiState->m_usesCustomLevel = true;
                    g_multiState->m_customLevelName = mapName;
                    g_multiState->m_builtInLevelName = "";
                    fclose(file);
                }
            } else {
                CWnd* child = GetDlgItem(IDX(IDC_MULTI_WORLD))->GetWindow(GW_CHILD);
                if (child == NULL) {
                    return;
                }
                child->SetWindowTextA(mapName);
                g_multiState->m_usesCustomLevel = false;
                g_multiState->m_customLevelName = "";
                g_multiState->m_builtInLevelName = mapName;
            }
        }
        {
            CWnd* chatLog = GetDlgItem(IDX(IDC_MULTI_CHAT_LOG));
            g_netMessageEditHwnd = (chatLog == NULL) ? NULL : chatLog->m_hWnd;
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
        CWnd* worldCombo = GetDlgItem(IDX(IDC_MULTI_WORLD));
        CWnd* child = worldCombo->GetWindow(GW_CHILD);
        if (child == NULL) {
            return;
        }
        child->GetWindowTextA(m_worldName);
        if (g_multiState->m_isHost != false) {
            reg->SetValueString("LastMultiMap", m_worldName);
            reg->SetValueDword("CustomMultiMap", IDX(m_customMapSelection));
        }
        GruntzPlayer* slots = m_gameManager->m_players;
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
        control->SendMessageA(CB_SETCURSEL, selection, 0);
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
    return control->SendMessageA(CB_GETCURSEL, 0, 0);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000c2940, 0x2b)
i32 CMultiStartDlg::GetMaxGruntzSelection(i32 slot) {
    CWnd* control = GetMaxGruntzControl(slot);
    if (control == NULL) {
        return -1;
    }
    return control->SendMessageA(CB_GETCURSEL, 0, 0) + 1;
}

RVA(0x000c2980, 0x28)
void CMultiStartDlg::SetMaxGruntzSelection(i32 slot, i32 count) {
    CWnd* control = GetMaxGruntzControl(slot);
    if (control) {
        control->SendMessageA(CB_SETCURSEL, count - 1, 0);
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
    GruntzPlayer* player = &m_gameManager->m_players[slot];
    if (typeControl->SendMessageA(CB_GETCURSEL, 0, 0) == 0) {
        if (player->m_humanControlled && player->m_active) {
            g_multiState->DropLobbyPlayer(player->m_playerIndex);
        } else if (!player->m_humanControlled && player->m_active) {
            SetPlayerColorAvailable(player->m_color, true);
        }
        player->m_active = false;
        player->m_ready = false;
        nameControl->EnableWindow(false);
        colorControl->EnableWindow(false);
    } else {
        if (static_cast<MultiplayerPlayerKind>(typeControl->SendMessageA(CB_GETCURSEL, 0, 0))
            != MULTI_PLAYER_HUMAN) {
            if (player->m_humanControlled != false) {
                if (player->m_active != false) {
                    g_multiState->DropLobbyPlayer(player->m_playerIndex);
                }
                ColorTint freeColor = FindAvailablePlayerColor();
                player->m_color = freeColor;
                SetPlayerColorAvailable(freeColor, false);
            } else if (player->m_active == false) {
                ColorTint freeColor = FindAvailablePlayerColor();
                player->m_color = freeColor;
                SetPlayerColorAvailable(freeColor, false);
            }
            player->m_ready = true;
            player->m_humanControlled = false;
            player->m_difficulty = static_cast<BattlezDifficulty>(
                static_cast<i32>(typeControl->SendMessageA(CB_GETCURSEL, 0, 0)) - 1
            );
            player->m_active = true;
            player->m_name = g_defaultPlayerNames[slot];
        }
        nameControl->EnableWindow(true);
        colorControl->EnableWindow(true);
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
    SetTimer(MULTI_START_WATCHDOG_TIMER, 0x32, NULL);
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

RVA(0x000c3830, 0xd1)
void CMultiStartDlg::OnPlayerColor0() {
    CMulti* multi = g_multiState;
    if ((multi->m_isHost == false || m_gameManager->m_players[0].m_humanControlled != false)
        && (m_gameManager->m_players[0].m_ready != false
            || m_gameManager->m_players[0].m_networkPlayerId != multi->m_localPlayerId)) {
        return;
    }
    CBattlezDlgColors colorDialog(m_gameManager, 0, 1, NULL);
    if (colorDialog.DoModal() == 1) {
        if (SetPlayerColor(0, static_cast<ColorTint>(colorDialog.m_pickedColor))) {
            BroadcastPlayerSlotChanges();
            GetDlgItem(CTRL_PLAYER_COLOR0)->InvalidateRect(NULL, true);
        }
    }
}

RVA(0x000c3950, 0xd1)
void CMultiStartDlg::OnPlayerColor1() {
    CMulti* multi = g_multiState;
    if ((multi->m_isHost == false || m_gameManager->m_players[1].m_humanControlled != false)
        && (m_gameManager->m_players[1].m_ready != false
            || m_gameManager->m_players[1].m_networkPlayerId != multi->m_localPlayerId)) {
        return;
    }
    CBattlezDlgColors colorDialog(m_gameManager, 1, 1, NULL);
    if (colorDialog.DoModal() == 1) {
        if (SetPlayerColor(1, static_cast<ColorTint>(colorDialog.m_pickedColor))) {
            BroadcastPlayerSlotChanges();
            GetDlgItem(CTRL_PLAYER_COLOR1)->InvalidateRect(NULL, true);
        }
    }
}

RVA(0x000c3a70, 0xd1)
void CMultiStartDlg::OnPlayerColor2() {
    CMulti* multi = g_multiState;
    if ((multi->m_isHost == false || m_gameManager->m_players[2].m_humanControlled != false)
        && (m_gameManager->m_players[2].m_ready != false
            || m_gameManager->m_players[2].m_networkPlayerId != multi->m_localPlayerId)) {
        return;
    }
    CBattlezDlgColors colorDialog(m_gameManager, 2, 1, NULL);
    if (colorDialog.DoModal() == 1) {
        if (SetPlayerColor(2, static_cast<ColorTint>(colorDialog.m_pickedColor))) {
            BroadcastPlayerSlotChanges();
            GetDlgItem(CTRL_PLAYER_COLOR2)->InvalidateRect(NULL, true);
        }
    }
}

RVA(0x000c3b90, 0xd1)
void CMultiStartDlg::OnPlayerColor3() {
    CMulti* multi = g_multiState;
    if ((multi->m_isHost == false || m_gameManager->m_players[3].m_humanControlled != false)
        && (m_gameManager->m_players[3].m_ready != false
            || m_gameManager->m_players[3].m_networkPlayerId != multi->m_localPlayerId)) {
        return;
    }
    CBattlezDlgColors colorDialog(m_gameManager, 3, 1, NULL);
    if (colorDialog.DoModal() == 1) {
        if (SetPlayerColor(3, static_cast<ColorTint>(colorDialog.m_pickedColor))) {
            BroadcastPlayerSlotChanges();
            GetDlgItem(CTRL_PLAYER_COLOR3)->InvalidateRect(NULL, true);
        }
    }
}

RVA(0x000c3cb0, 0x128)
void CMultiStartDlg::OnCustomWorld() {
    if (g_multiState->m_isHost == false) {
        return;
    }
    CBattlezDlgCustom dlg(NULL);
    if (dlg.DoModal() == 1 && dlg.m_customName.GetLength() != 0) {

        CWnd* worldCombo = GetDlgItem(IDX(IDC_MULTI_WORLD));
        CWnd* worldEdit = worldCombo->GetWindow(GW_CHILD);

        if (worldEdit == NULL) {
            return;
        }
        dlg.m_customName.MakeUpper();
        worldEdit->SetWindowTextA(static_cast<LPCTSTR>(dlg.m_customName));
        m_customMapSelection = CUSTOM_MAP_SELECTED;
        g_multiState->m_usesCustomLevel = true;
        g_multiState->m_customLevelName = static_cast<LPCTSTR>(dlg.m_customName);
        g_multiState->m_builtInLevelName = "";
        g_multiState->SendGameConfig(NULL);
    }
}

RVA(0x000c3e30, 0xfe)
void CMultiStartDlg::CommitWorldSelection() {
    if (g_multiState->m_isHost != false) {
        CWnd* worldCombo = GetDlgItem(IDX(IDC_MULTI_WORLD));
        if (worldCombo != NULL) {
            i32 selection = worldCombo->SendMessageA(CB_GETCURSEL, 0, 0);
            if (selection != -1) {
                CString worldName;
                (static_cast<CComboBox*>(worldCombo))->GetLBText(selection, worldName);
                if (worldName.GetLength() != 0) {
                    m_customMapSelection = CUSTOM_MAP_STANDARD;
                }
                g_multiState->m_usesCustomLevel = false;
                g_multiState->m_customLevelName = "";
                g_multiState->m_builtInLevelName = static_cast<LPCTSTR>(worldName);
                g_multiState->SendGameConfig(NULL);
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
    if (multi->m_isHost != false) {
        multi->BroadcastPlayerTable(NULL);
        RefreshPlayerControls(1);
    } else {
        g_multiState->BroadcastPlayerUpdate(
            m_gameManager->FindPlayerByNetworkId(multi->m_localPlayerId)
        );
    }
}

RVA(0x000c4120, 0xc2)
i32 CMultiStartDlg::EnableChatControls() {
    CWnd* control = GetDlgItem(IDCANCEL);
    control->EnableWindow(true);
    control = GetDlgItem(IDX(IDC_NETCHAT_SEND));
    control->EnableWindow(true);
    control = GetDlgItem(IDX(IDC_MULTI_CHAT_INPUT));
    control->EnableWindow(true);
    control = GetDlgItem(IDX(IDC_MULTI_CHAT_LOG));
    control->EnableWindow(true);
    CString s1;
    if (g_multiState->m_usesCustomLevel == false) {
        CString s2;
    }
    return 1;
}

// @early-stop
RVA(0x000c4230, 0x38e)
i32 CMultiStartDlg::RefreshPlayerControls(i32 force) {
    CWnd::GetFocus();
    b32 allLivePlayersReady = true;
    b32 hasRemoteHumanPlayer = false;
    i32 localSlotIndex = this->GetLocalPlayerSlotIndex();
    b32 localReadyFlag =
        g_multiState->m_isHost ? m_gameManager->m_players[localSlotIndex].m_ready : true;
    for (i32 slotIndex = 0; slotIndex < 4; slotIndex++) {
        GruntzPlayer* player = &g_gameReg->m_players[slotIndex];
        if (player) {
            if (player->m_networkPlayerId != g_multiState->m_localPlayerId
                && player->m_humanControlled && player->m_active) {
                hasRemoteHumanPlayer = true;
            }
            CWnd* nameControl = GetPlayerNameControl(slotIndex);
            if ((g_multiState->m_isHost && player->m_humanControlled == false)
                || player->m_networkPlayerId == g_multiState->m_localPlayerId) {
                nameControl->EnableWindow(true);
            } else {
                nameControl->EnableWindow(false);
            }
            CWnd* typeControl = GetPlayerTypeControl(slotIndex);
            if (g_multiState->m_isHost && localReadyFlag == false
                && player->m_networkPlayerId != g_multiState->m_localPlayerId) {
                typeControl->EnableWindow(true);
            } else {
                typeControl->EnableWindow(false);
            }
            CWnd* readyControl = GetReadyControl(slotIndex);
            if (player->m_networkPlayerId == g_multiState->m_localPlayerId) {
                readyControl->EnableWindow(true);
            } else {
                readyControl->EnableWindow(false);
            }
            if (player->m_ready == false) {
                if (player->m_active) {
                    ::SendMessageA(readyControl->m_hWnd, BM_SETCHECK, 0, 0);
                    allLivePlayersReady = false;
                } else {
                    ::SendMessageA(readyControl->m_hWnd, BM_SETCHECK, 0, 0);
                }
            } else if (player->m_active) {
                ::SendMessageA(readyControl->m_hWnd, BM_SETCHECK, 1, 0);
            } else {
                ::SendMessageA(readyControl->m_hWnd, BM_SETCHECK, 0, 0);
            }
            CWnd* maxGruntzControl = GetMaxGruntzControl(slotIndex);
            maxGruntzControl->EnableWindow(
                g_multiState->m_isHost && player->m_active && localReadyFlag == false
            );
            SetMaxGruntzSelection(slotIndex, player->m_active ? player->m_maxGruntz : 0);
            if (force == 0) {
                if (this->GetLocalPlayerSlotIndex() == slotIndex) {
                    continue;
                }
                if (g_multiState->m_isHost && player->m_humanControlled == false) {
                    continue;
                }
            }
            if (player->m_active) {
                {
                    force = 0;
                    GetPlayerNameControl(slotIndex)->SetWindowTextA(player->GetName());
                }
                if (player->m_humanControlled) {
                    CWnd* typeCombo = GetPlayerTypeControl(slotIndex);
                    ::SendMessageA(typeCombo->m_hWnd, CB_SETCURSEL, 4, 0);
                } else {
                    i32 selection = IDX(player->m_difficulty);
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
    ::InvalidateRect(color0, NULL, true);
    HWND color1 = this->GetDlgItem(CTRL_PLAYER_COLOR1)->m_hWnd;
    ::InvalidateRect(color1, NULL, true);
    HWND color2 = this->GetDlgItem(CTRL_PLAYER_COLOR2)->m_hWnd;
    ::InvalidateRect(color2, NULL, true);
    HWND color3 = this->GetDlgItem(CTRL_PLAYER_COLOR3)->m_hWnd;
    ::InvalidateRect(color3, NULL, true);
    return 1;
}

// @early-stop
RVA(0x000c46b0, 0x384)
void CMultiStartDlg::Watchdog() {
    if (g_watchdogBusy != false) {
        return;
    }
    g_watchdogBusy = true;
    CNetSessionListNode* session = g_multiState->m_netMgr->m_selectedSession;
    if (session == NULL) {
        return;
    }
    g_multiState->m_netMgr->EnumerateSessionPlayers(session, 0);
    g_multiState->ResolveLocalPlayer();
    if (g_netStatsTick == 0) {
        u32 timestamp = timeGetTime();
        g_multiState->BroadcastValueMessage(NETMSG_LATENCY_PROBE, static_cast<i32>(timestamp), 0);
    }
    if (g_multiState->m_isHost == false) {
        if (g_netStatsTick == 0) {
            g_multiState->ReportMaxAckLatency();
        }
        EnableWindow(false);
        i32 verificationResult =
            g_multiState->VerifyCustomLevel(session, g_multiState->m_localPlayer);
        EnableWindow(true);
        if (verificationResult != 0) {
            EndDialog(1);
            g_watchdogBusy = false;
            return;
        }
    } else {
        g_multiState->PollSession();
        if (g_multiState->m_autoCommandDelay != false) {
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
            GruntzPlayer* player = &g_gameReg->m_players[i];
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
            if (player->m_active != false && player->m_humanControlled != false) {
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
    if (g_multiState->m_sessionTerminated != false) {
        KillTimer(1);
        g_multiState->ReportVersionMsg("The game session has been terminated.", 0);
        g_watchdogBusy = false;
        return;
    }
    if (g_multiState->m_colorSelectionRejected != false) {
        g_multiState->m_colorSelectionRejected = false;
        g_multiState->ReportVersionMsg("Someone has already selected that color.", 0);
        g_watchdogBusy = false;
        return;
    }
    char* errorMessage;
    if (g_multiState->m_removedByHost != false) {
        KillTimer(1);
        errorMessage = "You have been removed from the game by the host.";
    } else if (g_multiState->m_gameClosed != false) {
        KillTimer(1);
        errorMessage = "This game is closed.";
    } else if (g_multiState->m_gameFull != false) {
        KillTimer(1);
        errorMessage = "This game is already full.";
    } else if (g_multiState->m_versionMismatch != false) {
        KillTimer(1);
        errorMessage = "This version is not the same as the host computer's version of the game.";
    } else {
        if (g_playerRosterChanged != false) {
            RefreshPlayerControls(1);
            EnableChatControls();
            RefreshWorldControls();
            RefreshLatencyControl();
            g_playerRosterChanged = false;
        }
        if (g_multiState->m_connectAccepted != false) {
            EnableChatControls();
            RefreshWorldControls();
            RefreshLatencyControl();
            g_multiState->m_connectAccepted = false;
        }
        g_watchdogBusy = false;
        return;
    }
    g_multiState->ReportVersionMsg(errorMessage, 0);
    EndDialog(0);
    g_watchdogBusy = false;
}

RVA(0x000c4b30, 0x1f)
i32 CMultiStartDlg::GetLocalPlayerSlotIndex() {
    GruntzPlayer* slot = m_gameManager->FindPlayerByNetworkId(g_multiState->m_localPlayerId);
    if (slot == NULL) {
        return -1;
    }
    return slot->m_playerIndex;
}

RVA(0x000c4b60, 0x77)
i32 CMultiStartDlg::SetPlayerColor(i32 slot, ColorTint color) {
    GruntzPlayer* player = &m_gameManager->m_players[slot];
    if (g_multiState->m_isHost != false) {
        b32 available = IsPlayerColorAvailable(color);
        if (available == false) {
            g_multiState->ReportVersionMsg("Someone has already selected that color.", available);
            return 0;
        }
        SetPlayerColorAvailable(player->m_color, true);
        SetPlayerColorAvailable(color, false);
    }
    player->m_color = color;
    return 1;
}

RVA(0x000c4c00, 0x190)
void CMultiStartDlg::OnOK() {
    if (g_multiState->m_isHost == false) {
        return;
    }
    if (&CMulti::GetCommandDelay == NULL) {
        return;
    }
    if (&CMulti::GetResendDelay == NULL) {
        return;
    }
    g_multiState->BroadcastPlayerIdMessage(NETMSG_VERIFY_CUSTOM_LEVEL, DPSEND_GUARANTEED);
    i32 customLevel = g_multiState->m_usesCustomLevel;
    i32 verificationToken = g_gameReg->ResolveLevelChecksum(
        false,
        false,
        customLevel,
        0,
        customLevel != 0 ? g_multiState->CustomLevelName() : g_multiState->BuiltInLevelName()
    );
    g_multiState->m_levelVerifyResult = false;
    if (g_multiState->Poll(verificationToken) == 0) {
        g_multiState->m_customLevelVerificationPending = false;
        EnableWindow(false);
        g_gameReg->EnterModalUI(
            "Unable to verify custom level with other players. The game will not start."
        );
        EnableWindow(true);
    } else if (g_multiState->m_levelVerifyResult != false) {
        g_multiState->m_customLevelVerificationPending = true;
        CDialog::OnOK();
    } else {
        g_multiState->m_customLevelVerificationPending = false;
        EnableWindow(false);
        g_gameReg->EnterModalUI("Not all players have the (same) custom level.");
        EnableWindow(true);
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

RVA(0x000c4ec0, 0x3)
void CMultiStartDlg::HandlePlayerNameChange(i32 slot) {}

RVA(0x000c4ee0, 0x33)
void CMultiStartDlg::OnMaxGruntzSelection0() {
    CWnd* combo = GetMaxGruntzControl(0);
    g_gameReg->m_players[0].m_maxGruntz = combo->SendMessageA(CB_GETCURSEL, 0, 0) + 1;
    BroadcastPlayerSlotChanges();
}

RVA(0x000c4f30, 0x33)
void CMultiStartDlg::OnMaxGruntzSelection1() {
    CWnd* combo = GetMaxGruntzControl(1);
    g_gameReg->m_players[1].m_maxGruntz = combo->SendMessageA(CB_GETCURSEL, 0, 0) + 1;
    BroadcastPlayerSlotChanges();
}

RVA(0x000c4f80, 0x33)
void CMultiStartDlg::OnMaxGruntzSelection2() {
    CWnd* combo = GetMaxGruntzControl(2);
    g_gameReg->m_players[2].m_maxGruntz = combo->SendMessageA(CB_GETCURSEL, 0, 0) + 1;
    BroadcastPlayerSlotChanges();
}

RVA(0x000c4fd0, 0x33)
void CMultiStartDlg::OnMaxGruntzSelection3() {
    CWnd* combo = GetMaxGruntzControl(3);
    g_gameReg->m_players[3].m_maxGruntz = combo->SendMessageA(CB_GETCURSEL, 0, 0) + 1;
    BroadcastPlayerSlotChanges();
}

RVA(0x000c5020, 0x95)
void CMultiStartDlg::CommitLatencySelection() {
    if (g_multiState->m_isHost == false) {
        return;
    }
    i32 commandDelay, resendInterval;
    HWND dialogHwnd = GetSafeHwnd();
    m_latencyOptions
        ->GetSelItemData(dialogHwnd, IDX(IDC_MULTI_LATENCY), &commandDelay, &resendInterval);
    if (commandDelay != 0 || resendInterval != 0) {
        g_multiState->m_commandDelay = commandDelay;
        g_multiState->m_resendInterval = resendInterval;
        g_multiState->m_autoCommandDelay = false;
        g_multiState->SendGameConfig(NULL);
    } else {
        g_multiState->m_autoCommandDelay = true;
    }
}

RVA(0x000c50f0, 0x9b)
void CMultiStartDlg::CommitReadySelection(i32 slotIndex) {
    CWnd* readyControl = GetReadyControl(slotIndex);
    if (!readyControl) {
        return;
    }
    i32 checked = readyControl->SendMessageA(BM_GETCHECK, 0, 0);
    GruntzPlayer* player = &g_gameReg->m_players[slotIndex];
    if (!player) {
        return;
    }
    if (checked) {
        player->m_ready = true;
    } else {
        player->m_ready = false;
    }
    if (g_multiState->m_isHost) {
        g_multiState->BroadcastPlayerTable(NULL);
        RefreshPlayerControls(1);
        EnableChatControls();
        RefreshWorldControls();
        RefreshLatencyControl();
    } else {
        g_multiState->BroadcastPlayerUpdate(player);
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
    CKeyedList* latencyOptions = m_latencyOptions;
    if (latencyOptions) {
        delete latencyOptions;
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
