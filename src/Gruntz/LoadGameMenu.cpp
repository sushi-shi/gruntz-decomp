#include <rva.h>

#include <Gruntz/LoadGameMenu.h>

#include <Mfc.h>

#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Play.h>
#include <Gruntz/SaveSlotCtrlId.h>
#include <Io/SaveGame.h>

DATA(0x00245ca4)
CSaveGame* g_dlgLoadSink = NULL;

RVA(0x0009dff0, 0x8c)
BOOL CALLBACK GruntzLoadGameDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (wParam == IDCANCEL || wParam == IDOK) {

                CPlay* obj = g_gameReg->PickPlayOrPausedState();
                if (obj) {
                    obj->m_stepCountdown = 2;
                }
                EndDialog(hDlg, 0);
                return true;
            }
            if (LoadGameCommand(hDlg, static_cast<i32>(wParam), g_dlgLoadSink) != 0) {
                return true;
            }

        default:
            return false;
        case WM_INITDIALOG: {
            g_dlgLoadSink = static_cast<CSaveGame*>(g_gameReg->m_saveGame);
            FillGameInfoDialog(hDlg, g_dlgLoadSink);
            return true;
        }
    }
}

RVA(0x0009e0b0, 0x1a3)
void FillGameInfoDialog(HWND hWnd, CSaveGame* sg) {
    if (hWnd == NULL || sg == NULL) {
        return;
    }
    LabelGameInfoSlot(
        hWnd,
        sg->GetSlot(0),
        CTRL_SAVEDLG_SLOT0,
        CTRL_SAVESLOT_LOAD0,
        CTRL_SAVESLOT_INFO0,
        CTRL_SAVESLOT_DELETE0
    );
    LabelGameInfoSlot(
        hWnd,
        sg->GetSlot(1),
        CTRL_SAVEDLG_SLOT1,
        CTRL_SAVESLOT_LOAD1,
        CTRL_SAVESLOT_INFO1,
        CTRL_SAVESLOT_DELETE1
    );
    LabelGameInfoSlot(
        hWnd,
        sg->GetSlot(2),
        CTRL_SAVEDLG_SLOT2,
        CTRL_SAVESLOT_LOAD2,
        CTRL_SAVESLOT_INFO2,
        CTRL_SAVESLOT_DELETE2
    );
    LabelGameInfoSlot(
        hWnd,
        sg->GetSlot(3),
        CTRL_SAVEDLG_SLOT3,
        CTRL_SAVESLOT_LOAD3,
        CTRL_SAVESLOT_INFO3,
        CTRL_SAVESLOT_DELETE3
    );
    LabelGameInfoSlot(
        hWnd,
        sg->GetSlot(4),
        CTRL_SAVEDLG_SLOT4,
        CTRL_SAVESLOT_LOAD4,
        CTRL_SAVESLOT_INFO4,
        CTRL_SAVESLOT_DELETE4
    );
    LabelGameInfoSlot(
        hWnd,
        sg->GetSlot(5),
        CTRL_SAVEDLG_SLOT5,
        CTRL_SAVESLOT_LOAD5,
        CTRL_SAVESLOT_INFO5,
        CTRL_SAVESLOT_DELETE5
    );
    LabelGameInfoSlot(
        hWnd,
        sg->GetSlot(6),
        CTRL_SAVEDLG_SLOT6,
        CTRL_SAVESLOT_LOAD6,
        CTRL_SAVESLOT_INFO6,
        CTRL_SAVESLOT_DELETE6
    );
    LabelGameInfoSlot(
        hWnd,
        sg->GetSlot(7),
        CTRL_SAVEDLG_SLOT7,
        CTRL_SAVESLOT_LOAD7,
        CTRL_SAVESLOT_INFO7,
        CTRL_SAVESLOT_DELETE7
    );
    LabelGameInfoSlot(
        hWnd,
        sg->GetSlot(8),
        CTRL_SAVEDLG_SLOT8,
        CTRL_SAVESLOT_LOAD8,
        CTRL_SAVESLOT_INFO8,
        CTRL_SAVESLOT_DELETE8
    );
    LabelGameInfoSlot(
        hWnd,
        sg->GetSlot(9),
        CTRL_SAVEDLG_SLOT9,
        CTRL_SAVESLOT_LOAD9,
        CTRL_SAVESLOT_INFO9,
        CTRL_SAVESLOT_DELETE9
    );
}

RVA(0x0009e2d0, 0x84)
void LabelGameInfoSlot(
    HWND hWnd,
    SaveSlot* item,
    i32 nameControlId,
    i32 loadControlId,
    i32 infoControlId,
    i32 deleteControlId
) {
    b32 flag;
    if (TempFileExists(item)) {
        SetDlgItemTextA(hWnd, nameControlId, item->m_name);
        flag = true;
    } else {
        SetDlgItemTextA(hWnd, nameControlId, "(Empty)");
        flag = false;
    }
    EnableWindow(GetDlgItem(hWnd, nameControlId), flag);
    EnableWindow(GetDlgItem(hWnd, loadControlId), flag);
    EnableWindow(GetDlgItem(hWnd, infoControlId), flag);
    EnableWindow(GetDlgItem(hWnd, deleteControlId), flag);
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
            EnableWindow(hwnd, false);
            g_gameReg->RunModalDialog("GAME_INFO", LevelPreviewDlgProc, false);
            EnableWindow(hwnd, true);
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
            EnableWindow(hwnd, false);
            i32 r = g_gameReg->RunModalDialog("GAME_DELETE", DeleteSaveDialogProc, false);
            EnableWindow(hwnd, true);
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
            EnableWindow(hwnd, false);
            i32 r = dlg->VerifySlot(slot);
            EnableWindow(hwnd, true);
            if (r == 0) {
                return 1;
            }
            g_gameReg->m_saveInfoRec = slot;
            PostMessageA(g_gameReg->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_LOAD_SAVED_GAME), 0);
            EndDialog(hwnd, 1);
            return 1;
        }
    }
    return 0;
}
