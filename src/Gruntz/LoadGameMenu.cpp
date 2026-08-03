#include <rva.h>

#include <Gruntz/LoadGameMenu.h>

#include <Mfc.h>

#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/SaveSlotCtrlId.h>
#include <Io/SaveGame.h>

DATA(0x00245ca4)
CSaveGame* g_dlgLoadSink = 0;

RVA(0x0009dff0, 0x8c)
i32 CALLBACK GruntzLoadGameDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (wParam == 2 || wParam == 1) {

                CPlay* obj = g_gameReg->PickPlayOrPausedState();
                if (obj) {
                    obj->m_stepCountdown = 2;
                }
                EndDialog(hDlg, 0);
                return 1;
            }
            if (LoadGameCommand(hDlg, static_cast<i32>(wParam), g_dlgLoadSink) != 0) {
                return 1;
            }

        default:
            return 0;
        case WM_INITDIALOG: {
            g_dlgLoadSink = static_cast<CSaveGame*>(g_gameReg->m_saveSink);
            FillGameInfoDialog(hDlg, g_dlgLoadSink);
            return 1;
        }
    }
}

RVA(0x0009e0b0, 0x1a3)
void FillGameInfoDialog(HWND hWnd, CSaveGame* sg) {
    if (hWnd == NULL || sg == NULL) {
        return;
    }
    LabelGameInfoSlot(hWnd, sg->GetSlot(0), 0x435, 0x490, 0x49a, 0x4a4);
    LabelGameInfoSlot(hWnd, sg->GetSlot(1), 0x436, 0x491, 0x49b, 0x4a5);
    LabelGameInfoSlot(hWnd, sg->GetSlot(2), 0x437, 0x492, 0x49c, 0x4a6);
    LabelGameInfoSlot(hWnd, sg->GetSlot(3), 0x438, 0x493, 0x49d, 0x4a7);
    LabelGameInfoSlot(hWnd, sg->GetSlot(4), 0x439, 0x494, 0x49e, 0x4a8);
    LabelGameInfoSlot(hWnd, sg->GetSlot(5), 0x43a, 0x495, 0x49f, 0x4a9);
    LabelGameInfoSlot(hWnd, sg->GetSlot(6), 0x43b, 0x496, 0x4a0, 0x4aa);
    LabelGameInfoSlot(hWnd, sg->GetSlot(7), 0x43c, 0x497, 0x4a1, 0x4ab);
    LabelGameInfoSlot(hWnd, sg->GetSlot(8), 0x43d, 0x498, 0x4a2, 0x4ac);
    LabelGameInfoSlot(hWnd, sg->GetSlot(9), 0x43e, 0x499, 0x4a3, 0x4ad);
}

RVA(0x0009e2d0, 0x84)
void LabelGameInfoSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6) {
    i32 flag;
    if (TempFileExists(item)) {
        SetDlgItemTextA(hWnd, id3, item->m_name);
        flag = 1;
    } else {
        SetDlgItemTextA(hWnd, id3, "(Empty)");
        flag = 0;
    }
    EnableWindow(GetDlgItem(hWnd, id3), flag);
    EnableWindow(GetDlgItem(hWnd, id4), flag);
    EnableWindow(GetDlgItem(hWnd, id5), flag);
    EnableWindow(GetDlgItem(hWnd, id6), flag);
}

// @early-stop
RVA(0x0009e390, 0x2bc)
i32 LoadGameCommand(HWND hwnd, i32 cmdId, CSaveGame* dlg) {
    i32 idx = -1;
    switch (cmdId) {
        case CTRL_SAVESLOT_INFO0:
            idx = 0;
            break;
        case CTRL_SAVESLOT_INFO1:
            idx = 1;
            break;
        case CTRL_SAVESLOT_INFO2:
            idx = 2;
            break;
        case CTRL_SAVESLOT_INFO3:
            idx = 3;
            break;
        case CTRL_SAVESLOT_INFO4:
            idx = 4;
            break;
        case CTRL_SAVESLOT_INFO5:
            idx = 5;
            break;
        case CTRL_SAVESLOT_INFO6:
            idx = 6;
            break;
        case CTRL_SAVESLOT_INFO7:
            idx = 7;
            break;
        case CTRL_SAVESLOT_INFO8:
            idx = 8;
            break;
        case CTRL_SAVESLOT_INFO9:
            idx = 9;
            break;
    }
    if (idx != -1) {
        g_slotState = dlg->GetSlot(idx);
        if (g_slotState) {
            EnableWindow(hwnd, FALSE);
            g_gameReg->RunModalDialog("GAME_INFO", LevelPreviewDlgProc, 0);
            EnableWindow(hwnd, TRUE);
        }
        return 0;
    }
    idx = -1;
    switch (cmdId) {
        case CTRL_SAVESLOT_DELETE0:
            idx = 0;
            break;
        case CTRL_SAVESLOT_DELETE1:
            idx = 1;
            break;
        case CTRL_SAVESLOT_DELETE2:
            idx = 2;
            break;
        case CTRL_SAVESLOT_DELETE3:
            idx = 3;
            break;
        case CTRL_SAVESLOT_DELETE4:
            idx = 4;
            break;
        case CTRL_SAVESLOT_DELETE5:
            idx = 5;
            break;
        case CTRL_SAVESLOT_DELETE6:
            idx = 6;
            break;
        case CTRL_SAVESLOT_DELETE7:
            idx = 7;
            break;
        case CTRL_SAVESLOT_DELETE8:
            idx = 8;
            break;
        case CTRL_SAVESLOT_DELETE9:
            idx = 9;
            break;
    }
    if (idx != -1) {
        g_slotState = dlg->GetSlot(idx);
        if (g_slotState) {
            EnableWindow(hwnd, FALSE);
            i32 r = g_gameReg->RunModalDialog("GAME_DELETE", DeleteSaveDialogProc, 0);
            EnableWindow(hwnd, TRUE);
            if (r) {
                FillGameInfoDialog(hwnd, dlg);
            }
        }
        return 0;
    }
    idx = -1;
    switch (cmdId) {
        case CTRL_SAVESLOT_LOAD0:
            idx = 0;
            break;
        case CTRL_SAVESLOT_LOAD1:
            idx = 1;
            break;
        case CTRL_SAVESLOT_LOAD2:
            idx = 2;
            break;
        case CTRL_SAVESLOT_LOAD3:
            idx = 3;
            break;
        case CTRL_SAVESLOT_LOAD4:
            idx = 4;
            break;
        case CTRL_SAVESLOT_LOAD5:
            idx = 5;
            break;
        case CTRL_SAVESLOT_LOAD6:
            idx = 6;
            break;
        case CTRL_SAVESLOT_LOAD7:
            idx = 7;
            break;
        case CTRL_SAVESLOT_LOAD8:
            idx = 8;
            break;
        case CTRL_SAVESLOT_LOAD9:
            idx = 9;
            break;
    }
    if (idx != -1) {
        SaveSlot* slot = dlg->GetSlot(idx);
        if (slot) {
            EnableWindow(hwnd, FALSE);
            i32 r = dlg->VerifySlot(slot);
            EnableWindow(hwnd, TRUE);
            if (r) {
                g_gameReg->m_saveInfoRec = slot;
                PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, 0x807e, 0);
                EndDialog(hwnd, 1);
            }
            return 1;
        }
    }
    return 0;
}
