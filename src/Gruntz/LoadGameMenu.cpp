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
// The 0x64556c singleton IS CGruntzMgr (RTTI-confirmed, vftable 0x5e9b64) - declared at
// the REAL class so its methods emit DEFINED symbols instead of CGameRegistry phantoms
// (?RunModalDialog@CGameRegistry@@... etc. are names no obj and no .LIB can ever define).
// extern "C" keeps ONE C symbol (_g_gameReg) whatever C++ type a TU declares it at.
DATA(0x0024556c)
extern "C" CGruntzMgr* g_gameReg;
// The last-queried slot handle (== SaveGameMenu's g_slotState; DATA-bound there).
extern i32 g_slotState; // ?g_slotState@@3HA @0x64c864
// The active GAME_LOAD dialog's CSaveGame sink, latched at WM_INITDIALOG.
DATA(0x00245ca4)
CSaveGame* g_dlgLoadSink = 0; // DAT_00645ca4  (owner-TU definition)

// The GAME_INFO / GAME_DELETE sub-dialog procs the load dialog runs (reloc-masked code
// ptrs).
extern "C" void LoadInfoDlgProc();   // 0x1e3d (GAME_INFO)
extern "C" void LoadDeleteDlgProc(); // 0x121c (GAME_DELETE)

// FillGameInfoDialog @0x9e0b0 (the slot-roster filler, reached at WM_INITDIALOG via ILT
// thunk 0x2ee6) + LabelGameInfoSlot @0x9e2d0 (its per-slot labeller) are DEFINED below in
// retail-RVA order (they sit inside this loadgamemenu .text obj, between the DlgProc and
// LoadGameCommand); forward-declared here so GruntzLoadGameDlgProc/LoadGameCommand can
// call them. IsSlotOccupied (0x2694 -> 0xe5700) is the slot-occupancy probe (reloc-masked,
// defined in savegame). REHOME package D7 (was in src/Io/SaveGame.cpp's obj).
void FillGameInfoDialog(HWND hDlg, CSaveGame* dlg);
void LabelGameInfoSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6);
// The slot-occupancy probe IS TempFileExists_e5700 (0x2694 jmp-thunk -> 0xe5700, defined
// in savegame): a SaveSlot overlaps the SaveTempRec fields it reads. Reloc-masked extern.
struct SaveTempRec;
int TempFileExists_e5700(SaveTempRec* p);

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
                // PickPlayOrPausedState (0x92990) is a CGruntzMgr method; g_gameReg is
                // the CGameRegistry facet of the same 0x24556c object - reach it through
                // the established CGruntzMgr dual-view cast (as m_saveInfoRec/m_gameWnd
                // below) so the call binds ?PickPlayOrPausedState@CGruntzMgr@@ at 0x92990.
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
// FillGameInfoDialog (0x0009e0b0): the GAME_INFO/GAME_LOAD dialog's slot roster filler -
// the twin of SaveGame.cpp's FillSaveDialog, but labelling through the 0x9e2d0 variant of
// the per-slot helper. Same ten rows / same four base control IDs (0x435 / 0x490 / 0x49a /
// 0x4a4 + slot index). __cdecl(HWND, CSaveGame*); both pointers null-checked up front.
RVA(0x0009e0b0, 0x1a3)
void FillGameInfoDialog(HWND hWnd, CSaveGame* sg) {
    if (hWnd == 0 || sg == 0) {
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

// -------------------------------------------------------------------------
// LabelGameInfoSlot (0x0009e2d0, GAME_INFO dialog variant): label one CSaveGame slot's
// short name (m_name) into id3, "(Empty)" when IsSlotOccupied fails; then set the four
// control enables (here ALL four track occupancy). __cdecl(hWnd, item, id3..id6).
RVA(0x0009e2d0, 0x84)
void LabelGameInfoSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6) {
    i32 flag;
    if (TempFileExists_e5700((SaveTempRec*)item)) {
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
                g_gameReg->m_saveInfoRec = (SaveInfo*)slot;
                PostMessageA(g_gameReg->m_gameWnd->m_hwnd, 0x111, 0x807e, 0);
                EndDialog(hwnd, 1);
            }
            return 1;
        }
    }
    return 0;
}
