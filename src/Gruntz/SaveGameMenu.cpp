// SaveGameMenu.cpp - the save-game dialog's command dispatcher (C:\Proj\Gruntz),
// re-homed from src/Stub/Backlog.cpp. __cdecl(hDlg, cmd, obj): the dialog defers its
// real command through g_savedMenuCmd, then a 4-way dense-switch jump-table cascade
// decodes the control/button id (GAME_INFO / GAME_DELETE / save-overwrite arms).
// Frameless; every callee is a reloc-masked external, the ids/strings are the
// load-bearing bytes.
#include <Win32.h> // EnableWindow / GetDlgItemTextA / EndDialog

#include <rva.h>
#include <stdio.h>  // sprintf (reloc-masked)
#include <string.h> // _strcmpi (reloc-masked)

SIZE_UNKNOWN(CSaveGameMenu);
struct CSaveGameMenu {                             // arg3: the save-game record
    i32 GetSlotState(i32 slot);                    // FUN @ 0x3bcf __thiscall
    void WriteName(i32 slot, char* nm, void* mgr); // FUN @ 0x219e __thiscall
    i32 CommitSlot(i32 a, i32 b);                  // FUN @ 0x2d97 __thiscall
};
SIZE_UNKNOWN(SaveMenuMgr);
struct SaveMenuMgr { // the *0x64556c singleton, this method's alias
    i32 Prompt(const char* name, void* proc, i32 flag); // FUN @ 0x2bb7 __thiscall
    void Notify(i32 state, char* nm);                   // FUN @ 0x24c3 __thiscall
    void Log(const char* s);                            // FUN @ 0x417e __thiscall
};
DATA(0x0024556c)
extern SaveMenuMgr* g_saveMenuMgr; // *0x64556c
DATA(0x00213a9c)
extern i32 g_savedMenuCmd; // DAT_00613a9c  pending deferred command
DATA(0x0024c864)
extern i32 g_slotState; // DAT_0064c864  last-queried slot state
// FUN_00002694 __cdecl / FUN_00002e05 __cdecl (thunks): slot-used test / delete.
i32 SlotHasSave(i32 state);
void DeleteSaveSlot(HWND hDlg, CSaveGameMenu* obj);
// The three dialog sub-proc thunks passed as callbacks (reloc-masked code ptrs).
extern "C" void SaveInfoProc();
extern "C" void SaveDeleteProc();
extern "C" void SaveOverwriteProc();

// @early-stop
// jump-table scoring wall: the code bytes match the retail dispatch (the four
// dense-case jump tables, the deferred-command latch, the GAME_INFO/DELETE/
// OVERWRITE/save arms, EnableWindow/GetDlgItemText/EndDialog gates and the two
// return tails), but MSVC emits each switch's jump table as its own `$Lnnn` COMDAT
// while the delinker inlines the four `switchdataD_*` tables into the function body,
// so the four `jmp [id*4+table]` base relocs + the table data don't pair (see
// docs/patterns/switch-jumptable-separate-comdat.md + jumptable-data-overlap.md).
RVA(0x000e3f40, 0x3d6)
i32 DrawSaveGameMenu(HWND hDlg, i32 cmd, CSaveGameMenu* obj) {
    i32 c;
    if (cmd == 1) {
        c = g_savedMenuCmd;
        if (c == -1) {
            return 0;
        }
    } else {
        c = cmd;
    }

    // Latch a pending command from a control notification.
    if (((u32)c >> 16) == 0x100) {
        switch (c & 0xffff) {
            case 0x435:
                g_savedMenuCmd = 0x490;
                break;
            case 0x436:
                g_savedMenuCmd = 0x491;
                break;
            case 0x437:
                g_savedMenuCmd = 0x492;
                break;
            case 0x438:
                g_savedMenuCmd = 0x493;
                break;
            case 0x439:
                g_savedMenuCmd = 0x494;
                break;
            case 0x43a:
                g_savedMenuCmd = 0x495;
                break;
            case 0x43b:
                g_savedMenuCmd = 0x496;
                break;
            case 0x43c:
                g_savedMenuCmd = 0x497;
                break;
            case 0x43d:
                g_savedMenuCmd = 0x498;
                break;
            case 0x43e:
                g_savedMenuCmd = 0x499;
                break;
        }
    }

    // GAME_INFO buttons.
    i32 info;
    switch (c) {
        case 0x49a:
            info = 0;
            break;
        case 0x49b:
            info = 1;
            break;
        case 0x49c:
            info = 2;
            break;
        case 0x49d:
            info = 3;
            break;
        case 0x49e:
            info = 4;
            break;
        case 0x49f:
            info = 5;
            break;
        case 0x4a0:
            info = 6;
            break;
        case 0x4a1:
            info = 7;
            break;
        case 0x4a2:
            info = 8;
            break;
        case 0x4a3:
            info = 9;
            break;
        default:
            info = -1;
            break;
    }
    if (info != -1) {
        g_slotState = obj->GetSlotState(info);
        if (g_slotState == 0) {
            return 0;
        }
        EnableWindow(hDlg, FALSE);
        g_saveMenuMgr->Prompt("GAME_INFO", (void*)SaveInfoProc, 0);
        EnableWindow(hDlg, TRUE);
        return 0;
    }

    // GAME_DELETE buttons.
    i32 del;
    switch (c) {
        case 0x4a4:
            del = 0;
            break;
        case 0x4a5:
            del = 1;
            break;
        case 0x4a6:
            del = 2;
            break;
        case 0x4a7:
            del = 3;
            break;
        case 0x4a8:
            del = 4;
            break;
        case 0x4a9:
            del = 5;
            break;
        case 0x4aa:
            del = 6;
            break;
        case 0x4ab:
            del = 7;
            break;
        case 0x4ac:
            del = 8;
            break;
        case 0x4ad:
            del = 9;
            break;
        default:
            del = -1;
            break;
    }
    if (del != -1) {
        g_slotState = obj->GetSlotState(del);
        if (g_slotState == 0) {
            return 0;
        }
        EnableWindow(hDlg, FALSE);
        i32 ok = g_saveMenuMgr->Prompt("GAME_DELETE", (void*)SaveDeleteProc, 0);
        EnableWindow(hDlg, TRUE);
        if (ok == 0) {
            return 0;
        }
        DeleteSaveSlot(hDlg, obj);
        return 0;
    }

    // Save / overwrite buttons.
    i32 slot = -1;
    switch (c) {
        case 0x490:
            slot = 0;
            break;
        case 0x491:
            slot = 1;
            break;
        case 0x492:
            slot = 2;
            break;
        case 0x493:
            slot = 3;
            break;
        case 0x494:
            slot = 4;
            break;
        case 0x495:
            slot = 5;
            break;
        case 0x496:
            slot = 6;
            break;
        case 0x497:
            slot = 7;
            break;
        case 0x498:
            slot = 8;
            break;
        case 0x499:
            slot = 9;
            break;
    }
    if (slot == -1) {
        return 0;
    }

    char name[0x20];
    GetDlgItemTextA(hDlg, 0x435 + slot, name, 0x20);
    if (_strcmpi(name, "(Empty)") == 0) {
        sprintf(name, "Saved Game #%i", slot + 1);
    }
    if (SlotHasSave(obj->GetSlotState(slot))) {
        g_slotState = obj->GetSlotState(slot);
        if (g_slotState != 0) {
            EnableWindow(hDlg, FALSE);
            i32 ok = g_saveMenuMgr->Prompt("GAME_OVERWRITE", (void*)SaveOverwriteProc, 0);
            EnableWindow(hDlg, TRUE);
            if (ok == 0) {
                return 1;
            }
        }
    }
    obj->WriteName(slot, name, g_saveMenuMgr);
    g_saveMenuMgr->Notify(obj->GetSlotState(slot), name);
    EndDialog(hDlg, 1);
    if (!obj->CommitSlot(obj->GetSlotState(slot) + 0x35, 0x81a6)) {
        g_saveMenuMgr->Log("ERROR - Cannot Save Game.");
    }
    return 1;
}
