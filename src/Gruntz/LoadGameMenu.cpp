// LoadGameMenu.cpp - the in-game "GAME_LOAD" modal dialog (C:\Proj\Gruntz), opened by
// CGruntzMgr::RunLoadGameDialog (@0x092500, which passes GruntzLoadGameDlgProc + the
// "GAME_LOAD" template to RunModalDialog). GruntzLoadGameDlgProc is the DlgProc;
// LoadGameCommand (its WM_COMMAND handler, reached through the ILT thunk 0x215d)
// decodes the per-slot info/delete/load buttons over the save-slot control-id scheme.
// The load-side sibling of DrawSaveGameMenu (SaveGame.cpp) (same 0x490/0x49a/0x4a4 id ranges +
// CSaveGame slots; only offsets + code bytes are load-bearing).
#include <Mfc.h>         // afx-first superset (EnableWindow/EndDialog/PostMessageA + CGruntzMgr)
#include <Io/SaveGame.h> // CSaveGame (GetSlot/VerifySlot) + SaveSlot/SaveInfo
#include <Gruntz/GruntzMgr.h> // CGruntzMgr (RunModalDialog/PickPlayOrPausedState/m_saveSink/m_gameWnd/m_saveInfoRec)
#include <Gruntz/Play.h> // CPlay (PickPlayOrPausedState's concrete return; m_stepCountdown @+0x510)

#include <rva.h>
#include <Win32.h>

// The 0x24556c game-manager singleton (== SaveGameMenu's g_gameReg; DATA-bound
// there, extern here).
extern "C" CGameRegistry* g_gameReg;
// The last-queried slot handle (== SaveGameMenu's g_slotState; DATA-bound there).
extern i32 g_slotState; // ?g_slotState@@3HA @0x64c864
// The active GAME_LOAD dialog's CSaveGame sink, latched at WM_INITDIALOG.
DATA(0x00245ca4)
extern CSaveGame* g_dlgLoadSink; // DAT_00645ca4

// The GAME_INFO / GAME_DELETE sub-dialog procs the load dialog runs (reloc-masked code
// ptrs) and the dialog (re)init helper (a __cdecl reached via ILT thunk 0x2ee6).
extern "C" void LoadInfoDlgProc();                  // 0x1e3d (GAME_INFO)
extern "C" void LoadDeleteDlgProc();                // 0x121c (GAME_DELETE)
void FillGameInfoDialog(HWND hDlg, CSaveGame* dlg); // 0x2ee6 (fill the slot list)

i32 LoadGameCommand(HWND hwnd, i32 cmdId, CSaveGame* dlg); // 0x9e390 (WM_COMMAND handler)

// -------------------------------------------------------------------------
// GruntzLoadGameDlgProc (0x09dff0, __stdcall CALLBACK, ret BOOL). WM_INITDIALOG
// latches the game-manager's save sink and fills the slot list; WM_COMMAND IDOK/
// IDCANCEL resumes the paused/play state (step-countdown 2) and closes, else defers
// to LoadGameCommand.
RVA(0x0009dff0, 0x8c)
i32 CALLBACK GruntzLoadGameDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND: // 0x111
            if (wParam == 2 || wParam == 1) {
                CPlay* obj = g_gameReg->PickPlayOrPausedState();
                if (obj) {
                    obj->m_stepCountdown = 2;
                }
                EndDialog(hDlg, 0);
                return 1;
            }
            if (LoadGameCommand(hDlg, (i32)wParam, g_dlgLoadSink) != 0) {
                return 1;
            }
            // falls through to the shared "return 0" default
        default:
            return 0;
        case WM_INITDIALOG: { // 0x110
            g_dlgLoadSink = (CSaveGame*)g_gameReg->m_saveSink;
            FillGameInfoDialog(hDlg, g_dlgLoadSink);
            return 1;
        }
    }
}

// -------------------------------------------------------------------------
// LoadGameCommand (0x09e390, __cdecl(hDlg, cmdId, dlg), ret BOOL) - the GAME_LOAD
// dialog's WM_COMMAND handler. Three contiguous control-id ranges (info / delete /
// load buttons) each map to a 0..9 slot index; info/delete pop their modal sub-dialog
// around a disabled window, and the load range verifies the slot then hands its handle
// to the main window (WM_COMMAND 0x807e) and closes.
//
// @early-stop
// jump-table-data scoring artifact (docs/patterns/jumptable-data-overlap.md): the
// switch dispatch, all three 10-way index tables, case bodies and the two return
// epilogues are byte-exact (llvm-objdump -dr), but objdiff scores the 3 inline
// jump-table regions (~120 B) as mismatched - cl's base obj references local `$L####`
// case labels (addend 0) while the delinked target carries `?LoadGameCommand+offset`
// self-relocs, which objdiff cannot pair. ~79%.
RVA(0x0009e390, 0x243)
i32 LoadGameCommand(HWND hwnd, i32 cmdId, CSaveGame* dlg) {
    i32 idx = -1;
    switch (cmdId) {
        case 0x49a:
            idx = 0;
            break;
        case 0x49b:
            idx = 1;
            break;
        case 0x49c:
            idx = 2;
            break;
        case 0x49d:
            idx = 3;
            break;
        case 0x49e:
            idx = 4;
            break;
        case 0x49f:
            idx = 5;
            break;
        case 0x4a0:
            idx = 6;
            break;
        case 0x4a1:
            idx = 7;
            break;
        case 0x4a2:
            idx = 8;
            break;
        case 0x4a3:
            idx = 9;
            break;
    }
    if (idx != -1) {
        g_slotState = (i32)dlg->GetSlot(idx);
        if (g_slotState) {
            EnableWindow(hwnd, FALSE);
            g_gameReg->RunModalDialog("GAME_INFO", (void*)LoadInfoDlgProc, 0);
            EnableWindow(hwnd, TRUE);
        }
        return 0;
    }
    idx = -1;
    switch (cmdId) {
        case 0x4a4:
            idx = 0;
            break;
        case 0x4a5:
            idx = 1;
            break;
        case 0x4a6:
            idx = 2;
            break;
        case 0x4a7:
            idx = 3;
            break;
        case 0x4a8:
            idx = 4;
            break;
        case 0x4a9:
            idx = 5;
            break;
        case 0x4aa:
            idx = 6;
            break;
        case 0x4ab:
            idx = 7;
            break;
        case 0x4ac:
            idx = 8;
            break;
        case 0x4ad:
            idx = 9;
            break;
    }
    if (idx != -1) {
        g_slotState = (i32)dlg->GetSlot(idx);
        if (g_slotState) {
            EnableWindow(hwnd, FALSE);
            i32 r = g_gameReg->RunModalDialog("GAME_DELETE", (void*)LoadDeleteDlgProc, 0);
            EnableWindow(hwnd, TRUE);
            if (r) {
                FillGameInfoDialog(hwnd, dlg);
            }
        }
        return 0;
    }
    idx = -1;
    switch (cmdId) {
        case 0x490:
            idx = 0;
            break;
        case 0x491:
            idx = 1;
            break;
        case 0x492:
            idx = 2;
            break;
        case 0x493:
            idx = 3;
            break;
        case 0x494:
            idx = 4;
            break;
        case 0x495:
            idx = 5;
            break;
        case 0x496:
            idx = 6;
            break;
        case 0x497:
            idx = 7;
            break;
        case 0x498:
            idx = 8;
            break;
        case 0x499:
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
                ((CGruntzMgr*)g_gameReg)->m_saveInfoRec = (SaveInfo*)slot;
                PostMessageA(((CGruntzMgr*)g_gameReg)->m_gameWnd->m_hwnd, 0x111, 0x807e, 0);
                EndDialog(hwnd, 1);
            }
            return 1;
        }
    }
    return 0;
}
