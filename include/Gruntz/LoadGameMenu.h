// LoadGameMenu.h - the LoadGameMenu.cpp TU's exported globals/functions.
#ifndef GRUNTZ_GRUNTZ_LOADGAMEMENU_H
#define GRUNTZ_GRUNTZ_LOADGAMEMENU_H


#include <Mfc.h> // afx.h FIRST: Win32 types named below
#include <Ints.h>

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void LoadDeleteDlgProc(); // 0x121c (GAME_DELETE)
extern "C" void LoadInfoDlgProc();   // 0x1e3d (GAME_INFO)

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern i32 g_slotState; // ?g_slotState@@3HA @0x64c864


struct SaveSlot;  // <Io/SaveGame.h>
class CSaveGame;  // <Io/SaveGame.h>

// File-scope prototypes moved from the .cpp: an unqualified
// declaration at file scope has EXTERNAL linkage, so it belongs in
// the owner header.
void FillGameInfoDialog(HWND hDlg, CSaveGame* dlg);
void LabelGameInfoSlot(HWND hWnd, SaveSlot* item, i32 id3, i32 id4, i32 id5, i32 id6);

#endif // GRUNTZ_GRUNTZ_LOADGAMEMENU_H
